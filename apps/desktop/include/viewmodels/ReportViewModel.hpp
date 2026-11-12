#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <memory>

#include "models/ApiContracts.hpp"

namespace devdialogue::services {
class InterviewService;
}

namespace devdialogue::viewmodels {

class NavigationViewModel;

/// Backs the performance report: score, narrative summary, strengths,
/// weaknesses, recommendations and the per-question breakdown.
class ReportViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString interviewId READ interviewId NOTIFY reportChanged)
  Q_PROPERTY(QString direction READ direction NOTIFY reportChanged)
  Q_PROPERTY(QString level READ level NOTIFY reportChanged)
  Q_PROPERTY(QString language READ language NOTIFY reportChanged)
  Q_PROPERTY(QString status READ status NOTIFY reportChanged)
  Q_PROPERTY(int score READ score NOTIFY reportChanged)
  Q_PROPERTY(QString summary READ summary NOTIFY reportChanged)
  Q_PROPERTY(QVariantList strengths READ strengths NOTIFY reportChanged)
  Q_PROPERTY(QVariantList weaknesses READ weaknesses NOTIFY reportChanged)
  Q_PROPERTY(QVariantList recommendations READ recommendations NOTIFY
                 reportChanged)
  Q_PROPERTY(QVariantList questionScores READ questionScores NOTIFY
                 reportChanged)
  Q_PROPERTY(bool hasReport READ hasReport NOTIFY reportChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  ReportViewModel(std::shared_ptr<services::InterviewService> interviewService,
                  NavigationViewModel *navigation, QObject *parent = nullptr);

  QString interviewId() const { return m_interviewId; }
  QString direction() const { return m_direction; }
  QString level() const { return m_level; }
  QString language() const { return m_language; }
  QString status() const { return m_status; }
  int score() const { return m_score; }
  QString summary() const { return m_summary; }
  QVariantList strengths() const { return m_strengths; }
  QVariantList weaknesses() const { return m_weaknesses; }
  QVariantList recommendations() const { return m_recommendations; }
  QVariantList questionScores() const { return m_questionScores; }
  bool hasReport() const { return m_hasReport; }
  bool isBusy() const { return m_isBusy; }
  QString errorMessage() const { return m_errorMessage; }

  Q_INVOKABLE void loadReport(const QString &id);
  Q_INVOKABLE void startNewInterview();
  Q_INVOKABLE void backToDashboard();
  Q_INVOKABLE void openRecommendation(const QString &url);
  Q_INVOKABLE void clearError();

signals:
  void reportChanged();
  void busyChanged();
  void errorMessageChanged();

private:
  void setBusy(bool busy);
  void setError(const QString &error);
  void applyReport(const models::InterviewDetail &detail);

  std::shared_ptr<services::InterviewService> m_interviewService;
  NavigationViewModel *m_navigation;

  QString m_interviewId;
  QString m_direction{QStringLiteral("frontend")};
  QString m_level{QStringLiteral("junior")};
  QString m_language{QStringLiteral("uk")};
  QString m_status{QStringLiteral("completed")};
  int m_score{0};
  QString m_summary;
  QVariantList m_strengths;
  QVariantList m_weaknesses;
  QVariantList m_recommendations;
  QVariantList m_questionScores;
  bool m_hasReport{false};
  bool m_isBusy{false};
  QString m_errorMessage;
};

} // namespace devdialogue::viewmodels
