#include "models/QuestionListModel.hpp"

#include <algorithm>

namespace devdialogue::models {

QuestionListModel::QuestionListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int QuestionListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(m_questions.size());
}

QVariant QuestionListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_questions.size()))
    return {};

  const QuestionBankItem &item =
      m_questions[static_cast<std::size_t>(index.row())];

  switch (role) {
  case IdRole:
    return QString::fromStdString(item.id);
  case DirectionRole:
    return QString::fromStdString(
        nlohmann::json(item.direction).get<std::string>());
  case StageRole:
    return QString::fromStdString(nlohmann::json(item.stage).get<std::string>());
  case LevelRole:
    return QString::fromStdString(nlohmann::json(item.level).get<std::string>());
  case TopicRole:
    return QString::fromStdString(item.topic);
  case QuestionTextRole:
    return QString::fromStdString(item.questionText);
  case IsActiveRole:
    return item.isActive;
  case CreatedAtRole:
    return QString::fromStdString(item.createdAt);
  case Qt::DisplayRole:
    return QString::fromStdString(item.questionText);
  default:
    return {};
  }
}

QHash<int, QByteArray> QuestionListModel::roleNames() const {
  return {{IdRole, "questionId"},
          {DirectionRole, "direction"},
          {StageRole, "stage"},
          {LevelRole, "level"},
          {TopicRole, "topic"},
          {QuestionTextRole, "questionText"},
          {IsActiveRole, "isActive"},
          {CreatedAtRole, "createdAt"}};
}

int QuestionListModel::count() const {
  return static_cast<int>(m_questions.size());
}

void QuestionListModel::setQuestions(
    const std::vector<QuestionBankItem> &questions) {
  beginResetModel();
  m_questions = questions;
  endResetModel();
  emit countChanged();
}

void QuestionListModel::appendQuestion(const QuestionBankItem &question) {
  const int row = static_cast<int>(m_questions.size());
  beginInsertRows(QModelIndex(), row, row);
  m_questions.push_back(question);
  endInsertRows();
  emit countChanged();
}

void QuestionListModel::replaceQuestion(const QuestionBankItem &question) {
  auto it = std::find_if(m_questions.begin(), m_questions.end(),
                         [&question](const QuestionBankItem &item) {
                           return item.id == question.id;
                         });
  if (it == m_questions.end())
    return;

  const int row = static_cast<int>(std::distance(m_questions.begin(), it));
  *it = question;
  emit dataChanged(index(row), index(row));
}

void QuestionListModel::removeQuestion(const QString &id) {
  const std::string target = id.toStdString();
  auto it = std::find_if(m_questions.begin(), m_questions.end(),
                         [&target](const QuestionBankItem &item) {
                           return item.id == target;
                         });
  if (it == m_questions.end())
    return;

  const int row = static_cast<int>(std::distance(m_questions.begin(), it));
  beginRemoveRows(QModelIndex(), row, row);
  m_questions.erase(it);
  endRemoveRows();
  emit countChanged();
}

void QuestionListModel::setActive(const QString &id, bool active) {
  const std::string target = id.toStdString();
  auto it = std::find_if(m_questions.begin(), m_questions.end(),
                         [&target](const QuestionBankItem &item) {
                           return item.id == target;
                         });
  if (it == m_questions.end() || it->isActive == active)
    return;

  it->isActive = active;
  const int row = static_cast<int>(std::distance(m_questions.begin(), it));
  emit dataChanged(index(row), index(row), {IsActiveRole});
}

void QuestionListModel::clear() {
  if (m_questions.empty())
    return;
  beginResetModel();
  m_questions.clear();
  endResetModel();
  emit countChanged();
}

} // namespace devdialogue::models
