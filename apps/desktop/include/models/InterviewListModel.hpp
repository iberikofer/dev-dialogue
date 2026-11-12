#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::models {

class InterviewListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    DirectionRole,
    LevelRole,
    LanguageRole,
    StageRole,
    StatusRole,
    ScoreRole,
    HasScoreRole,
    QuestionCountRole,
    CreatedAtRole,
    UpdatedAtRole,
  };

  explicit InterviewListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const;

  void setInterviews(const std::vector<InterviewSummary> &interviews);
  void appendInterview(const InterviewSummary &interview);
  void clear();

signals:
  void countChanged();

private:
  std::vector<InterviewSummary> m_interviews;
};

} // namespace devdialogue::models
