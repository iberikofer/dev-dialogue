#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::models {

class QuestionListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    DirectionRole,
    StageRole,
    LevelRole,
    TopicRole,
    QuestionTextRole,
    IsActiveRole,
    CreatedAtRole,
  };

  explicit QuestionListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const;

  void setQuestions(const std::vector<QuestionBankItem> &questions);
  void appendQuestion(const QuestionBankItem &question);
  void replaceQuestion(const QuestionBankItem &question);
  void removeQuestion(const QString &id);
  void setActive(const QString &id, bool active);
  void clear();

signals:
  void countChanged();

private:
  std::vector<QuestionBankItem> m_questions;
};

} // namespace devdialogue::models
