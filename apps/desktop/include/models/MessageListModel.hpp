#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::models {

class MessageListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  enum Roles {
    RoleRole = Qt::UserRole + 1,
    ContentRole,
    TimestampRole,
  };

  explicit MessageListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const;

  void setMessages(const std::vector<InterviewMessage> &messages);
  void appendMessage(const InterviewMessage &message);
  void clear();

signals:
  void countChanged();

private:
  std::vector<InterviewMessage> m_messages;
};

} // namespace devdialogue::models
