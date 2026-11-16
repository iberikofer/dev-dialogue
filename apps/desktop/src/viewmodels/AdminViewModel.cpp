#include "viewmodels/AdminViewModel.hpp"

#include <QPointer>
#include <QRandomGenerator>
#include <algorithm>
#include <optional>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

constexpr int PageSize = 20;
constexpr int UndoWindowMs = 5000;

// Ids of questions that only exist locally, e.g. "q-7fk2a" (same shape as the web).
QString makeLocalQuestionId() {
    static const QString alphabet = QStringLiteral("abcdefghijklmnopqrstuvwxyz0123456789");
    QString suffix;
    for (int i = 0; i < 5; ++i) {
        suffix.append(alphabet.at(QRandomGenerator::global()->bounded(static_cast<int>(alphabet.size()))));
    }
    return QStringLiteral("q-") + suffix;
}

} // namespace

AdminViewModel::AdminViewModel(std::shared_ptr<services::AdminService> adminService,
                               std::shared_ptr<services::AuthService> authService, QObject* parent)
    : QObject(parent), m_adminService(std::move(adminService)), m_authService(std::move(authService)) {
    m_undoTimer.setSingleShot(true);
    m_undoTimer.setInterval(UndoWindowMs);
    connect(&m_undoTimer, &QTimer::timeout, this, &AdminViewModel::commitPendingDelete);
}

AdminViewModel::~AdminViewModel() {
    // Last attempt for a delete still inside its undo window. The process is
    // exiting, so the request may never leave; closing the window sends it
    // earlier and waits for the answer (sendPendingDeletes()).
    commitPendingDelete();
}

void AdminViewModel::setDirectionFilter(const QString& direction) {
    if (m_directionFilter == direction) {
        return;
    }
    m_directionFilter = direction;
    emit directionFilterChanged();
    m_page = 1;
    refresh();
}

void AdminViewModel::setUiLanguage(const QString& language) {
    if (m_uiLanguage == language) {
        return;
    }
    m_uiLanguage = language;
    emit uiLanguageChanged();
    // The sample questions follow the interface language.
    if (m_isDemoData) {
        applyDemoQuestions();
    }
}

void AdminViewModel::refresh() {
    // A new list has no place to put an undone question back.
    commitPendingDelete();
    const int generation = ++m_generation;
    setErrorMessage(QString());
    setLoading(true);

    if (!m_adminService || !backendLikelyOnline()) {
        applyDemoQuestions();
        return;
    }

    std::optional<models::InterviewDirection> direction;
    if (!m_directionFilter.isEmpty()) {
        direction = support::enumFromString<models::InterviewDirection>(m_directionFilter);
    }

    QPointer<AdminViewModel> self(this);
    m_adminService->getQuestions(
        direction, std::nullopt, std::nullopt, std::nullopt, m_page, PageSize,
        [self, generation](const std::variant<models::PagedResult<models::QuestionBankItem>, models::ApiError>& result) {
            if (!self || generation != self->m_generation) {
                return;
            }
            const auto* page = std::get_if<models::PagedResult<models::QuestionBankItem>>(&result);
            if (page == nullptr) {
                self->applyDemoQuestions();
                return;
            }
            const int pageSize = std::max(1, page->pageSize);
            const int pages = page->totalPages.value_or((page->totalCount + pageSize - 1) / pageSize);
            std::vector<models::QuestionBankItem> items = page->items;
            items.erase(std::remove_if(items.begin(), items.end(),
                                       [&self](const models::QuestionBankItem& item) {
                                           return self->m_deletingIds.contains(QString::fromStdString(item.id));
                                       }),
                        items.end());
            self->m_isDemoData = false;
            self->m_questions.setQuestions(std::move(items));
            self->setPaging(page->page, std::max(1, pages), page->totalCount);
            emit self->questionsChanged();
            self->setLoading(false);
        });
}

void AdminViewModel::nextPage() {
    if (m_page < m_totalPages) {
        ++m_page;
        emit pageChanged();
        refresh();
    }
}

void AdminViewModel::previousPage() {
    if (m_page > 1) {
        --m_page;
        emit pageChanged();
        refresh();
    }
}

void AdminViewModel::createQuestion(const QString& direction, const QString& level, const QString& topic,
                                    const QString& questionText) {
    const QString trimmedTopic = topic.trimmed();
    const QString trimmedText = questionText.trimmed();
    if (trimmedTopic.isEmpty() || trimmedText.isEmpty() || m_isSubmitting) {
        return;
    }

    models::CreateQuestionRequest request;
    request.direction = support::enumFromString<models::InterviewDirection>(direction);
    request.level = support::enumFromString<models::InterviewLevel>(level);
    request.stage = models::InterviewStage::HardSkills;
    request.topic = trimmedTopic.toStdString();
    request.questionText = trimmedText.toStdString();

    // The web client keeps a question that could not be saved in the local
    // list, so the admin flow can be demonstrated without a backend.
    auto addLocally = [this, request]() {
        models::QuestionBankItem item;
        item.id = makeLocalQuestionId().toStdString();
        item.direction = request.direction;
        item.level = request.level;
        item.stage = request.stage;
        item.topic = request.topic;
        item.questionText = request.questionText;
        item.isActive = true;
        item.createdAt = support::nowIso();
        if (m_authService) {
            const auto user = m_authService->currentUser();
            if (user.has_value()) {
                item.createdBy = user->id;
            }
        }
        m_questions.prepend(std::move(item));
        adjustTotalCount(1);
        emit questionsChanged();
        emit questionCreated();
    };

    if (!m_adminService || m_isDemoData) {
        addLocally();
        return;
    }

    setSubmitting(true);
    QPointer<AdminViewModel> self(this);
    m_adminService->createQuestion(
        request, [self, addLocally](const std::variant<models::QuestionBankItem, models::ApiError>& result) {
            if (!self) {
                return;
            }
            self->setSubmitting(false);
            if (const auto* created = std::get_if<models::QuestionBankItem>(&result)) {
                self->m_questions.prepend(*created);
                self->adjustTotalCount(1);
                emit self->questionsChanged();
                emit self->questionCreated();
                return;
            }
            addLocally();
        });
}

void AdminViewModel::deleteQuestion(const QString& id) {
    // Only one delete waits at a time: the previous one goes through now.
    commitPendingDelete();
    auto taken = m_questions.takeById(id);
    if (!taken.has_value()) {
        return;
    }
    const QString topic = QString::fromStdString(taken->second.topic);
    m_pendingDelete = PendingDelete{taken->first, std::move(taken->second)};
    adjustTotalCount(-1);
    emit questionsChanged();
    emit questionDeleted(topic);
    m_undoTimer.start();
}

void AdminViewModel::undoDelete() {
    if (!m_pendingDelete.has_value()) {
        return;
    }
    m_undoTimer.stop();
    PendingDelete pending = std::move(*m_pendingDelete);
    m_pendingDelete.reset();
    m_questions.insertAt(pending.row, std::move(pending.item));
    adjustTotalCount(1);
    emit questionsChanged();
}

bool AdminViewModel::sendPendingDeletes() {
    commitPendingDelete();
    return !m_deletingIds.isEmpty();
}

void AdminViewModel::commitPendingDelete() {
    m_undoTimer.stop();
    if (!m_pendingDelete.has_value()) {
        return;
    }
    PendingDelete pending = std::move(*m_pendingDelete);
    m_pendingDelete.reset();
    // Sample questions only live in this window.
    if (!m_adminService || m_isDemoData) {
        return;
    }

    const QString id = QString::fromStdString(pending.item.id);
    m_deletingIds.insert(id);
    QPointer<AdminViewModel> self(this);
    m_adminService->deleteQuestion(id, [self, id, pending](bool success, const models::ApiError& error) {
        if (!self) {
            return;
        }
        self->m_deletingIds.remove(id);
        if (self->m_deletingIds.isEmpty()) {
            emit self->deletesSettled();
        }
        if (success || !self) {
            return;
        }
        // The question is still on the server: show it again, unless a page
        // loaded since already has it.
        if (!self->m_questions.contains(id)) {
            self->m_questions.insertAt(pending.row, pending.item);
            self->adjustTotalCount(1);
            emit self->questionsChanged();
        }
        emit self->questionDeleteFailed(QString::fromStdString(pending.item.topic),
                                        error.isNetworkDown ? QString() : QString::fromStdString(error.message));
    });
}

void AdminViewModel::setQuestionActive(const QString& id, bool active) {
    const std::optional<bool> previous = m_questions.setActive(id, active);
    if (!previous.has_value() || *previous == active || !m_adminService || m_isDemoData) {
        return;
    }

    models::UpdateQuestionRequest request;
    request.isActive = active;

    QPointer<AdminViewModel> self(this);
    const bool before = *previous;
    m_adminService->updateQuestion(
        id, request, [self, id, before](const std::variant<models::QuestionBankItem, models::ApiError>& result) {
            if (!self) {
                return;
            }
            if (const auto* updated = std::get_if<models::QuestionBankItem>(&result)) {
                self->m_questions.replace(*updated);
                return;
            }
            const models::ApiError& error = std::get<models::ApiError>(result);
            // The server refused the change: put the switch back and say why.
            // Without a connection the local state is kept, like other edits.
            if (!error.isNetworkDown) {
                self->m_questions.setActive(id, before);
                self->setErrorMessage(QString::fromStdString(error.message));
            }
        });
}

void AdminViewModel::applyDemoQuestions() {
    commitPendingDelete();
    std::vector<models::QuestionBankItem> items = demo::questions(m_uiLanguage);
    if (!m_directionFilter.isEmpty()) {
        const auto wanted = support::enumFromString<models::InterviewDirection>(m_directionFilter);
        items.erase(std::remove_if(items.begin(), items.end(),
                                   [wanted](const models::QuestionBankItem& item) { return item.direction != wanted; }),
                    items.end());
    }
    m_isDemoData = true;
    const int count = static_cast<int>(items.size());
    m_questions.setQuestions(std::move(items));
    setPaging(1, 1, count);
    emit questionsChanged();
    setLoading(false);
}

void AdminViewModel::setLoading(bool value) {
    if (m_loading != value) {
        m_loading = value;
        emit loadingChanged();
    }
}

void AdminViewModel::setSubmitting(bool submitting) {
    if (m_isSubmitting != submitting) {
        m_isSubmitting = submitting;
        emit submittingChanged();
    }
}

void AdminViewModel::setErrorMessage(const QString& message) {
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

void AdminViewModel::setPaging(int page, int totalPages, int totalCount) {
    if (m_page != page || m_totalPages != totalPages || m_totalCount != totalCount) {
        m_page = page;
        m_totalPages = totalPages;
        m_totalCount = totalCount;
        emit pageChanged();
    }
}

void AdminViewModel::adjustTotalCount(int delta) {
    setPaging(m_page, m_totalPages, std::max(0, m_totalCount + delta));
}

} // namespace devdialogue::viewmodels
