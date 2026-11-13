#pragma once

#include <QAbstractListModel>
#include <QString>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::viewmodels {

// Chat transcript of the interview room. System messages never reach the
// screen, so they are dropped when a transcript is loaded.
class MessageListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        RoleRole = Qt::UserRole + 1,
        ContentRole,
        TimeTextRole
    };

    explicit MessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setMessages(const std::vector<models::InterviewMessage>& messages);
    // Appends a message stamped with the current time.
    void append(const QString& role, const QString& content);

    int count() const { return static_cast<int>(m_messages.size()); }
    int userMessageCount() const;

private:
    std::vector<models::InterviewMessage> m_messages;
};

} // namespace devdialogue::viewmodels
