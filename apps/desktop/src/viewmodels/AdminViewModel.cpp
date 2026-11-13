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
    : QObject(parent), m_adminService(std::move(adminService)), m_authService(std::move(authService)) {}

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
            self->m_isDemoData = false;
            self->m_questions.setQuestions(page->items);
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
                emit self->questionsChanged();
                emit self->questionCreated();
                return;
            }
            addLocally();
        });
}

void AdminViewModel::deleteQuestion(const QString& id) {
    // Removed from the list right away, as on the web; the server call is
    // best-effort.
    if (m_questions.removeById(id)) {
        emit questionsChanged();
    }
    if (m_adminService && !m_isDemoData) {
        m_adminService->deleteQuestion(id, [](bool, const models::ApiError&) {});
    }
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

} // namespace devdialogue::viewmodels
