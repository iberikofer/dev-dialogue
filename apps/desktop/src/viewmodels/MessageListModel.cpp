#include "viewmodels/MessageListModel.hpp"

#include <algorithm>
#include <iterator>

#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

MessageListModel::MessageListModel(QObject* parent) : QAbstractListModel(parent) {}

int MessageListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : count();
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= count()) {
        return {};
    }
    const models::InterviewMessage& message = m_messages[static_cast<std::size_t>(index.row())];

    switch (role) {
    case RoleRole:
        return QString::fromStdString(message.role);
    case ContentRole:
        return QString::fromStdString(message.content);
    case TimeTextRole:
        return support::formatTime(message.timestamp);
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const {
    return {
        {RoleRole, "role"},
        {ContentRole, "content"},
        {TimeTextRole, "timeText"},
    };
}

void MessageListModel::clear() {
    if (m_messages.empty()) {
        return;
    }
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

void MessageListModel::setMessages(const std::vector<models::InterviewMessage>& messages) {
    beginResetModel();
    m_messages.clear();
    std::copy_if(messages.begin(), messages.end(), std::back_inserter(m_messages),
                 [](const models::InterviewMessage& message) { return message.role != "system"; });
    endResetModel();
}

void MessageListModel::append(const QString& role, const QString& content) {
    models::InterviewMessage message;
    message.role = role.toStdString();
    message.content = content.toStdString();
    message.timestamp = support::nowIso();

    beginInsertRows(QModelIndex(), count(), count());
    m_messages.push_back(std::move(message));
    endInsertRows();
}

int MessageListModel::userMessageCount() const {
    return static_cast<int>(std::count_if(m_messages.begin(), m_messages.end(),
                                          [](const models::InterviewMessage& message) { return message.role == "user"; }));
}

} // namespace devdialogue::viewmodels
