#include "viewmodels/SetupViewModel.hpp"

#include <utility>
#include <variant>

#include "core/EnumUtils.hpp"
#include "services/InterviewService.hpp"
#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

SetupViewModel::SetupViewModel(
    std::shared_ptr<services::InterviewService> interviewService,
    NavigationViewModel *navigation, QObject *parent)
    : QObject(parent), m_interviewService(std::move(interviewService)),
      m_navigation(navigation) {}

void SetupViewModel::setDirection(const QString &direction) {
  if (m_direction == direction)
    return;
  m_direction = direction;
  emit selectionChanged();
}

void SetupViewModel::setLevel(const QString &level) {
  if (m_level == level)
    return;
  m_level = level;
  emit selectionChanged();
}

void SetupViewModel::setLanguage(const QString &language) {
  if (m_language == language)
    return;
  m_language = language;
  emit selectionChanged();
}

void SetupViewModel::setBusy(bool busy) {
  if (m_isBusy == busy)
    return;
  m_isBusy = busy;
  emit busyChanged();
}

void SetupViewModel::setError(const QString &error) {
  m_errorMessage = error;
  emit errorMessageChanged();
}

void SetupViewModel::startInterview() {
  if (m_isBusy)
    return;

  setBusy(true);
  setError(QString());

  using core::enums::directionFrom;
  using core::enums::languageFrom;
  using core::enums::levelFrom;

  m_interviewService->createInterview(
      directionFrom(m_direction), levelFrom(m_level), languageFrom(m_language),
      [this](const std::variant<models::InterviewDetail, network::ApiError>
                 &result) {
        setBusy(false);

        if (const auto *detail =
                std::get_if<models::InterviewDetail>(&result)) {
          const QString id = QString::fromStdString(detail->id);
          emit interviewCreated(id);
          if (m_navigation)
            m_navigation->navigateToChat(id);
          return;
        }

        setError(
            QString::fromStdString(std::get<network::ApiError>(result).message));
      });
}

void SetupViewModel::resetSelection() {
  setDirection(QStringLiteral("frontend"));
  setLevel(QStringLiteral("junior"));
  setLanguage(QStringLiteral("uk"));
  setError(QString());
}

void SetupViewModel::clearError() { setError(QString()); }

} // namespace devdialogue::viewmodels
