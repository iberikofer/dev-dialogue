#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "services/InterviewService.hpp"
#include "viewmodels/InterviewListModel.hpp"
#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

// Candidate dashboard: KPI statistics and the interview history
// (port of the data side of the web DashboardView.tsx).
class DashboardViewModel : public QObject, public support::BackendAware {
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(int totalInterviews READ totalInterviews NOTIFY statsChanged)
    Q_PROPERTY(double averageScore READ averageScore NOTIFY statsChanged)
    Q_PROPERTY(QString averageScoreText READ averageScoreText NOTIFY statsChanged)
    Q_PROPERTY(int dailyInterviewsCount READ dailyInterviewsCount NOTIFY statsChanged)
    Q_PROPERTY(int dailyLimit READ dailyLimit NOTIFY statsChanged)
    Q_PROPERTY(int remainingToday READ remainingToday NOTIFY statsChanged)
    Q_PROPERTY(bool isDemoData READ isDemoData NOTIFY statsChanged)
    Q_PROPERTY(QAbstractItemModel* interviews READ interviews CONSTANT)
    Q_PROPERTY(int interviewCount READ interviewCount NOTIFY interviewsChanged)

public:
    explicit DashboardViewModel(std::shared_ptr<services::InterviewService> interviewService,
                                QObject* parent = nullptr);

    bool loading() const { return m_loading; }
    int totalInterviews() const { return m_stats.totalInterviews; }
    double averageScore() const { return m_stats.averageScore; }
    // "82.5" or "77"; empty while there is no score yet.
    QString averageScoreText() const;
    int dailyInterviewsCount() const { return m_stats.dailyInterviewsCount; }
    int dailyLimit() const { return m_stats.dailyLimit; }
    int remainingToday() const { return m_stats.remainingToday; }
    bool isDemoData() const { return m_isDemoData; }
    QAbstractItemModel* interviews() { return &m_interviews; }
    int interviewCount() const { return m_interviews.count(); }

    // Loads the statistics and the history. If the backend cannot be
    // reached the screen shows the same sample data as the web client.
    Q_INVOKABLE void refresh();

signals:
    void loadingChanged();
    void statsChanged();
    void interviewsChanged();

private:
    void setLoading(bool loading);
    void apply(const models::UserDashboardStats& stats, std::vector<models::InterviewSummary> interviews,
               bool demo);

    std::shared_ptr<services::InterviewService> m_interviewService;
    InterviewListModel m_interviews;
    models::UserDashboardStats m_stats;
    bool m_loading{false};
    bool m_isDemoData{false};
    int m_generation{0};
};

} // namespace devdialogue::viewmodels
