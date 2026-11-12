#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "models/ApiContracts.hpp"

namespace devdialogue::models {
class InterviewListModel;
}

namespace devdialogue::services {
class InterviewService;
}

namespace devdialogue::viewmodels {

class NavigationViewModel;

/// Backs the candidate dashboard: KPI tiles plus the session history table.
class DashboardViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(int totalInterviews READ totalInterviews NOTIFY statsChanged)
  Q_PROPERTY(double averageScore READ averageScore NOTIFY statsChanged)
  Q_PROPERTY(int dailyInterviewsCount READ dailyInterviewsCount NOTIFY
                 statsChanged)
  Q_PROPERTY(int dailyLimit READ dailyLimit NOTIFY statsChanged)
  Q_PROPERTY(int remainingToday READ remainingToday NOTIFY statsChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
  Q_PROPERTY(models::InterviewListModel *interviewsModel READ interviewsModel
                 CONSTANT)

public:
  DashboardViewModel(std::shared_ptr<services::InterviewService> interviewService,
                     NavigationViewModel *navigation,
                     QObject *parent = nullptr);

  int totalInterviews() const { return m_totalInterviews; }
  double averageScore() const { return m_averageScore; }
  int dailyInterviewsCount() const { return m_dailyInterviewsCount; }
  int dailyLimit() const { return m_dailyLimit; }
  int remainingToday() const { return m_remainingToday; }

  bool isBusy() const { return m_isBusy; }
  QString errorMessage() const { return m_errorMessage; }

  models::InterviewListModel *interviewsModel() const {
    return m_interviewsModel;
  }

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void openInterview(const QString &id, const QString &status);
  Q_INVOKABLE void startNewInterview();
  Q_INVOKABLE void openAdmin();
  Q_INVOKABLE void clearError();

signals:
  void statsChanged();
  void busyChanged();
  void errorMessageChanged();

private:
  void setBusy(bool busy);
  void setError(const QString &error);
  void applyStats(const models::UserDashboardStats &stats);

  std::shared_ptr<services::InterviewService> m_interviewService;
  NavigationViewModel *m_navigation;
  models::InterviewListModel *m_interviewsModel;

  int m_totalInterviews{0};
  double m_averageScore{0.0};
  int m_dailyInterviewsCount{0};
  int m_dailyLimit{10};
  int m_remainingToday{10};
  bool m_isBusy{false};
  QString m_errorMessage;
};

} // namespace devdialogue::viewmodels
