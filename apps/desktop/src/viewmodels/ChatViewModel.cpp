#include "viewmodels/ChatViewModel.hpp"

#include <algorithm>
#include <utility>
#include <variant>

#include "core/EnumUtils.hpp"
#include "models/MessageListModel.hpp"
#include "services/InterviewService.hpp"
#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

namespace {

int countUserAnswers(const std::vector<models::InterviewMessage> &messages) {
  return static_cast<int>(
      std::count_if(messages.begin(), messages.end(),
                    [](const models::InterviewMessage &message) {
                      return message.role == "user";
                    }));
}

} // namespace

ChatViewModel::ChatViewModel(
    std::shared_ptr<services::InterviewService> interviewService,
    NavigationViewModel *navigation, QObject *parent)
    : QObject(parent), m_interviewService(std::move(interviewService)),
      m_navigation(navigation),
      m_messagesModel(new models::MessageListModel(this)) {
  connect(m_messagesModel, &models::MessageListModel::countChanged, this,
          &ChatViewModel::conversationChanged);
}

void ChatViewModel::setBusy(bool busy) {
  if (m_isBusy == busy)
    return;
  m_isBusy = busy;
  emit busyChanged();
}

void ChatViewModel::setError(const QString &error) {
  m_errorMessage = error;
  emit errorMessageChanged();
}

void ChatViewModel::setSessionMetadata(const models::InterviewDetail &detail) {
  m_currentDirection = core::enums::toString(detail.direction);
  m_currentLevel = core::enums::toString(detail.level);
  m_currentLanguage = core::enums::toString(detail.language);
  m_questionCount = detail.questionCount;
  emit sessionChanged();
}

void ChatViewModel::loadInterview(const QString &id) {
  if (id.isEmpty())
    return;

  m_interviewId = id;
  m_isCompleted = false;
  m_candidateAnswersCount = 0;
  m_questionCount = 0;
  m_messagesModel->clear();
  setError(QString());
  emit sessionChanged();
  emit completedChanged();

  setBusy(true);
  m_interviewService->getInterview(
      id, [this](const std::variant<models::InterviewDetail, network::ApiError>
                     &result) {
        setBusy(false);

        if (const auto *detail =
                std::get_if<models::InterviewDetail>(&result)) {
          setSessionMetadata(*detail);
          m_messagesModel->setMessages(detail->transcript);
          m_candidateAnswersCount = countUserAnswers(detail->transcript);
          m_isCompleted = detail->status == models::InterviewStatus::Completed;
          emit conversationChanged();
          emit completedChanged();
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void ChatViewModel::sendAnswer(const QString &content) {
  const QString trimmed = content.trimmed();
  if (trimmed.isEmpty() || m_isBusy || m_interviewId.isEmpty())
    return;

  setBusy(true);
  setError(QString());

  models::InterviewMessage userMessage;
  userMessage.role = "user";
  userMessage.content = trimmed.toStdString();
  m_messagesModel->appendMessage(userMessage);
  ++m_candidateAnswersCount;
  emit conversationChanged();

  m_interviewService->sendMessage(
      m_interviewId, trimmed,
      [this](const std::variant<models::SendMessageResponse,
                                network::ApiError> &result) {
        setBusy(false);

        if (const auto *response =
                std::get_if<models::SendMessageResponse>(&result)) {
          models::InterviewMessage assistantMessage;
          assistantMessage.role = "assistant";
          assistantMessage.content = response->message;
          m_messagesModel->appendMessage(assistantMessage);

          if (response->questionCount > 0)
            m_questionCount = response->questionCount;
          if (response->isCompleted.value_or(false))
            m_isCompleted = true;

          emit conversationChanged();
          emit completedChanged();
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void ChatViewModel::completeInterview() {
  if (m_isBusy || m_interviewId.isEmpty())
    return;

  setBusy(true);
  setError(QString());

  m_interviewService->completeInterview(
      m_interviewId,
      [this](const std::variant<models::CompleteInterviewResponse,
                                network::ApiError> &result) {
        setBusy(false);

        if (std::holds_alternative<models::CompleteInterviewResponse>(
                result)) {
          m_isCompleted = true;
          emit completedChanged();
          if (m_navigation)
            m_navigation->navigateToReport(m_interviewId);
          return;
        }

        setError(QString::fromStdString(
            std::get<network::ApiError>(result).message));
      });
}

void ChatViewModel::leaveSession() {
  if (m_navigation)
    m_navigation->navigateToDashboard();
}

void ChatViewModel::clearError() { setError(QString()); }

} // namespace devdialogue::viewmodels
