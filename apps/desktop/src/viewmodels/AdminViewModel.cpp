#include "viewmodels/AdminViewModel.hpp"

#include <utility>
#include <variant>

#include "core/EnumUtils.hpp"
#include "models/QuestionListModel.hpp"
#include "services/AdminService.hpp"

namespace devdialogue::viewmodels {

AdminViewModel::AdminViewModel(
    std::shared_ptr<services::AdminService> adminService, QObject *parent)
    : QObject(parent), m_adminService(std::move(adminService)),
      m_questionsModel(new models::QuestionListModel(this)) {}

bool AdminViewModel::parseActiveFilter(const QString &filter,
                                       std::optional<bool> &result) {
  if (filter == QLatin1String("active")) {
    result = true;
    return true;
  }
  if (filter == QLatin1String("inactive")) {
    result = false;
    return true;
  }
  result.reset();
  return false;
}

void AdminViewModel::setBusy(bool busy) {
  if (m_isBusy == busy)
    return;
  m_isBusy = busy;
  emit busyChanged();
}

void AdminViewModel::setError(const QString &error) {
  m_errorMessage = error;
  emit errorMessageChanged();
}

void AdminViewModel::setSuccess(const QString &message) {
  m_successMessage = message;
  emit successMessageChanged();
}

void AdminViewModel::setDirection(const QString &direction) {
  if (m_direction == direction)
    return;
  m_direction = direction;
  m_page = 1;
  emit filtersChanged();
  refresh();
}

void AdminViewModel::setLevel(const QString &level) {
  if (m_level == level)
    return;
  m_level = level;
  m_page = 1;
  emit filtersChanged();
  refresh();
}

void AdminViewModel::setStage(const QString &stage) {
  if (m_stage == stage)
    return;
  m_stage = stage;
  m_page = 1;
  emit filtersChanged();
  refresh();
}

void AdminViewModel::setActiveFilter(const QString &filter) {
  if (m_activeFilter == filter)
    return;
  m_activeFilter = filter;
  m_page = 1;
  emit filtersChanged();
  refresh();
}

void AdminViewModel::refresh() {
  setBusy(true);
  setError(QString());

  std::optional<bool> isActive;
  parseActiveFilter(m_activeFilter, isActive);

  m_adminService->listQuestions(
      m_direction, m_level, m_stage, isActive, m_page, m_pageSize,
      [this](const std::variant<models::PagedResult<models::QuestionBankItem>,
                                network::ApiError> &result) {
        setBusy(false);

        if (const auto *page =
                std::get_if<models::PagedResult<models::QuestionBankItem>>(
                    &result)) {
          m_page = page->page;
          m_pageSize = page->pageSize;
          m_totalCount = page->totalCount;
          m_totalPages = page->totalPages.value_or(0);
          m_questionsModel->setQuestions(page->items);
          emit questionsChanged();
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void AdminViewModel::applyFilters(const QString &direction,
                                  const QString &level,
                                  const QString &stage,
                                  const QString &activeFilter) {
  m_direction = direction;
  m_level = level;
  m_stage = stage;
  m_activeFilter = activeFilter;
  m_page = 1;
  emit filtersChanged();
  refresh();
}

void AdminViewModel::clearFilters() {
  applyFilters(QString(), QString(), QString(), QStringLiteral("all"));
}

void AdminViewModel::nextPage() {
  if (m_page >= m_totalPages)
    return;
  ++m_page;
  refresh();
}

void AdminViewModel::previousPage() {
  if (m_page <= 1)
    return;
  --m_page;
  refresh();
}

void AdminViewModel::createQuestion(const QString &direction,
                                    const QString &stage,
                                    const QString &level,
                                    const QString &topic,
                                    const QString &questionText) {
  if (isBusy())
    return;

  models::CreateQuestionRequest request;
  request.direction = core::enums::directionFrom(direction);
  request.stage = core::enums::stageFrom(stage);
  request.level = core::enums::levelFrom(level);
  request.topic = topic.toStdString();
  request.questionText = questionText.toStdString();

  setBusy(true);
  setError(QString());
  setSuccess(QString());

  m_adminService->createQuestion(
      request,
      [this](const std::variant<models::QuestionBankItem,
                                network::ApiError> &result) {
        setBusy(false);

        if (std::holds_alternative<models::QuestionBankItem>(result)) {
          setSuccess(tr("Question added to the bank."));
          refresh();
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void AdminViewModel::setQuestionActive(const QString &id, bool active) {
  if (id.isEmpty() || isBusy())
    return;

  models::UpdateQuestionRequest request;
  request.isActive = active;

  setBusy(true);
  setError(QString());
  setSuccess(QString());

  m_adminService->updateQuestion(
      id, request,
      [this, active](const std::variant<models::QuestionBankItem,
                                        network::ApiError> &result) {
        setBusy(false);

        if (const auto *item =
                std::get_if<models::QuestionBankItem>(&result)) {
          m_questionsModel->replaceQuestion(*item);
          m_questionsModel->setActive(QString::fromStdString(item->id),
                                      item->isActive);
          setSuccess(active ? tr("Question activated.")
                            : tr("Question deactivated."));
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void AdminViewModel::deleteQuestion(const QString &id) {
  if (id.isEmpty() || isBusy())
    return;

  setBusy(true);
  setError(QString());
  setSuccess(QString());

  m_adminService->deleteQuestion(
      id, [this, id](bool success, const QString &errorMessage) {
        setBusy(false);

        if (success) {
          m_questionsModel->removeQuestion(id);
          setSuccess(tr("Question deleted."));
          refresh();
          return;
        }

        setError(errorMessage.isEmpty() ? tr("Could not delete the question.")
                                        : errorMessage);
      });
}

void AdminViewModel::clearError() { setError(QString()); }

void AdminViewModel::clearSuccess() { setSuccess(QString()); }

} // namespace devdialogue::viewmodels
