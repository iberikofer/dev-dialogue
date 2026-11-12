#include "viewmodels/DashboardViewModel.hpp"

#include <QPointer>
#include <optional>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

constexpr int HistoryPageSize = 50;

// Result of the two parallel dashboard requests.
struct PendingLoad {
    std::optional<models::UserDashboardStats> stats;
    std::optional<std::vector<models::InterviewSummary>> interviews;
    int remaining{2};
    bool failed{false};
};

} // namespace

DashboardViewModel::DashboardViewModel(std::shared_ptr<services::InterviewService> interviewService,
                                       QObject* parent)
    : QObject(parent), m_interviewService(std::move(interviewService)) {}

QString DashboardViewModel::averageScoreText() const {
    if (m_stats.averageScore <= 0.0) {
        return QString();
    }
    QString text = QString::number(m_stats.averageScore, 'f', 1);
    if (text.endsWith(QLatin1String(".0"))) {
        text.chop(2);
    }
    return text;
}

void DashboardViewModel::refresh() {
    const int generation = ++m_generation;
    setLoading(true);

    if (!m_interviewService || !backendLikelyOnline()) {
        apply(demo::dashboardStats(), demo::interviews(), true);
        return;
    }

    auto pending = std::make_shared<PendingLoad>();
    QPointer<DashboardViewModel> self(this);
    auto finish = [self, pending, generation]() {
        if (--pending->remaining > 0 || !self || generation != self->m_generation) {
            return;
        }
        if (pending->failed || !pending->stats.has_value() || !pending->interviews.has_value()) {
            self->apply(demo::dashboardStats(), demo::interviews(), true);
        } else {
            self->apply(*pending->stats, std::move(*pending->interviews), false);
        }
    };

    m_interviewService->getStats(
        [pending, finish](const std::variant<models::UserDashboardStats, models::ApiError>& result) {
            if (const auto* stats = std::get_if<models::UserDashboardStats>(&result)) {
                pending->stats = *stats;
            } else {
                pending->failed = true;
            }
            finish();
        });

    m_interviewService->getInterviews(
        1, HistoryPageSize,
        [pending, finish](const std::variant<models::PagedResult<models::InterviewSummary>, models::ApiError>& result) {
            if (const auto* page = std::get_if<models::PagedResult<models::InterviewSummary>>(&result)) {
                pending->interviews = page->items;
            } else {
                pending->failed = true;
            }
            finish();
        });
}

void DashboardViewModel::setLoading(bool loading) {
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}

void DashboardViewModel::apply(const models::UserDashboardStats& stats,
                               std::vector<models::InterviewSummary> interviews, bool demo) {
    m_stats = stats;
    m_isDemoData = demo;
    m_interviews.setInterviews(std::move(interviews));
    emit statsChanged();
    emit interviewsChanged();
    setLoading(false);
}

} // namespace devdialogue::viewmodels
