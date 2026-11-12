#include "models/MessageListModel.hpp"

namespace devdialogue::models {

MessageListModel::MessageListModel(QObject *parent) : QAbstractListModel(parent) {}

int MessageListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(m_messages.size());
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_messages.size()))
    return {};

  const InterviewMessage &message =
      m_messages[static_cast<std::size_t>(index.row())];

  switch (role) {
  case RoleRole:
    return QString::fromStdString(message.role);
  case ContentRole:
    return QString::fromStdString(message.content);
  case TimestampRole:
    return QString::fromStdString(message.timestamp);
  case Qt::DisplayRole:
    return QString::fromStdString(message.content);
  default:
    return {};
  }
}

QHash<int, QByteArray> MessageListModel::roleNames() const {
  return {{RoleRole, "role"},
          {ContentRole, "content"},
          {TimestampRole, "timestamp"}};
}

int MessageListModel::count() const {
  return static_cast<int>(m_messages.size());
}

void MessageListModel::setMessages(
    const std::vector<InterviewMessage> &messages) {
  beginResetModel();
  m_messages = messages;
  endResetModel();
  emit countChanged();
}

void MessageListModel::appendMessage(const InterviewMessage &message) {
  const int row = static_cast<int>(m_messages.size());
  beginInsertRows(QModelIndex(), row, row);
  m_messages.push_back(message);
  endInsertRows();
  emit countChanged();
}

void MessageListModel::clear() {
  if (m_messages.empty())
    return;
  beginResetModel();
  m_messages.clear();
  endResetModel();
  emit countChanged();
}

} // namespace devdialogue::models
