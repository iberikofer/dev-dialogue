#include "viewmodels/InterviewListModel.hpp"

#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

InterviewListModel::InterviewListModel(QObject* parent) : QAbstractListModel(parent) {}

int InterviewListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : count();
}

QVariant InterviewListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= count()) {
        return {};
    }
    const models::InterviewSummary& item = m_interviews[static_cast<std::size_t>(index.row())];

    switch (role) {
    case InterviewIdRole:
        return QString::fromStdString(item.id);
    case DateTextRole:
        return support::formatDateTime(item.createdAt);
    case DirectionRole:
        return support::enumToString(item.direction);
    case LevelRole:
        return support::enumToString(item.level);
    case StatusRole:
        return support::enumToString(item.status);
    case IsCompletedRole:
        return item.status == models::InterviewStatus::Completed;
    case HasScoreRole:
        return item.score.has_value();
    case ScoreRole:
        return item.score.value_or(0);
    default:
        return {};
    }
}

QHash<int, QByteArray> InterviewListModel::roleNames() const {
    return {
        {InterviewIdRole, "interviewId"},
        {DateTextRole, "dateText"},
        {DirectionRole, "direction"},
        {LevelRole, "level"},
        {StatusRole, "status"},
        {IsCompletedRole, "isCompleted"},
        {HasScoreRole, "hasScore"},
        {ScoreRole, "score"},
    };
}

void InterviewListModel::setInterviews(std::vector<models::InterviewSummary> interviews) {
    beginResetModel();
    m_interviews = std::move(interviews);
    endResetModel();
}

} // namespace devdialogue::viewmodels
