#include "models/InterviewListModel.hpp"

namespace devdialogue::models {

InterviewListModel::InterviewListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int InterviewListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(m_interviews.size());
}

QVariant InterviewListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_interviews.size()))
    return {};

  const InterviewSummary &item =
      m_interviews[static_cast<std::size_t>(index.row())];

  switch (role) {
  case IdRole:
    return QString::fromStdString(item.id);
  case DirectionRole:
    return QString::fromStdString(nlohmann::json(item.direction).get<std::string>());
  case LevelRole:
    return QString::fromStdString(nlohmann::json(item.level).get<std::string>());
  case LanguageRole:
    return QString::fromStdString(nlohmann::json(item.language).get<std::string>());
  case StageRole:
    return QString::fromStdString(nlohmann::json(item.stage).get<std::string>());
  case StatusRole:
    return QString::fromStdString(nlohmann::json(item.status).get<std::string>());
  case ScoreRole:
    return item.score ? QVariant(*item.score) : QVariant();
  case HasScoreRole:
    return item.score.has_value();
  case QuestionCountRole:
    return item.questionCount;
  case CreatedAtRole:
    return QString::fromStdString(item.createdAt);
  case UpdatedAtRole:
    return QString::fromStdString(item.updatedAt);
  default:
    return {};
  }
}

QHash<int, QByteArray> InterviewListModel::roleNames() const {
  return {{IdRole, "interviewId"},
          {DirectionRole, "direction"},
          {LevelRole, "level"},
          {LanguageRole, "language"},
          {StageRole, "stage"},
          {StatusRole, "status"},
          {ScoreRole, "score"},
          {HasScoreRole, "hasScore"},
          {QuestionCountRole, "questionCount"},
          {CreatedAtRole, "createdAt"},
          {UpdatedAtRole, "updatedAt"}};
}

int InterviewListModel::count() const {
  return static_cast<int>(m_interviews.size());
}

void InterviewListModel::setInterviews(
    const std::vector<InterviewSummary> &interviews) {
  beginResetModel();
  m_interviews = interviews;
  endResetModel();
  emit countChanged();
}

void InterviewListModel::appendInterview(const InterviewSummary &interview) {
  const int row = static_cast<int>(m_interviews.size());
  beginInsertRows(QModelIndex(), row, row);
  m_interviews.push_back(interview);
  endInsertRows();
  emit countChanged();
}

void InterviewListModel::clear() {
  if (m_interviews.empty())
    return;
  beginResetModel();
  m_interviews.clear();
  endResetModel();
  emit countChanged();
}

} // namespace devdialogue::models
