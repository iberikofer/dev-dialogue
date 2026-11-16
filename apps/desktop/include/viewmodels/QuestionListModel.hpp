#pragma once

#include <QAbstractListModel>
#include <QString>
#include <optional>
#include <utility>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::viewmodels {

// Rows of the admin question bank table.
class QuestionListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        QuestionIdRole = Qt::UserRole + 1,
        DirectionRole,
        LevelRole,
        TopicRole,
        QuestionTextRole,
        IsActiveRole
    };

    explicit QuestionListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setQuestions(std::vector<models::QuestionBankItem> questions);
    void prepend(models::QuestionBankItem question);
    bool removeById(const QString& id);
    // Removes the question and hands it back with the row it was in.
    std::optional<std::pair<int, models::QuestionBankItem>> takeById(const QString& id);
    // Puts a question back at the row (clamped to the list).
    void insertAt(int row, models::QuestionBankItem question);
    bool contains(const QString& id) const { return indexOfId(id) >= 0; }
    // Returns the previous state, or nothing if the question is not in the list.
    std::optional<bool> setActive(const QString& id, bool active);
    // Replaces the row with the same id (after the server confirmed an edit).
    void replace(const models::QuestionBankItem& question);

    int count() const { return static_cast<int>(m_questions.size()); }

private:
    int indexOfId(const QString& id) const;

    std::vector<models::QuestionBankItem> m_questions;
};

} // namespace devdialogue::viewmodels
