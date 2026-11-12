#pragma once

#include <QAbstractListModel>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::viewmodels {

// Rows of the dashboard history table.
class InterviewListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        InterviewIdRole = Qt::UserRole + 1,
        DateTextRole,
        DirectionRole,
        LevelRole,
        StatusRole,
        IsCompletedRole,
        HasScoreRole,
        ScoreRole
    };

    explicit InterviewListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setInterviews(std::vector<models::InterviewSummary> interviews);
    int count() const { return static_cast<int>(m_interviews.size()); }

private:
    std::vector<models::InterviewSummary> m_interviews;
};

} // namespace devdialogue::viewmodels
