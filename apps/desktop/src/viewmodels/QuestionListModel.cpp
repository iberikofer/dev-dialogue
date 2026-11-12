#include "viewmodels/QuestionListModel.hpp"

#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

QuestionListModel::QuestionListModel(QObject* parent) : QAbstractListModel(parent) {}

int QuestionListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : count();
}

QVariant QuestionListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= count()) {
        return {};
    }
    const models::QuestionBankItem& item = m_questions[static_cast<std::size_t>(index.row())];

    switch (role) {
    case QuestionIdRole:
        return QString::fromStdString(item.id);
    case DirectionRole:
        return support::enumToString(item.direction);
    case LevelRole:
        return support::enumToString(item.level);
    case TopicRole:
        return QString::fromStdString(item.topic);
    case QuestionTextRole:
        return QString::fromStdString(item.questionText);
    case IsActiveRole:
        return item.isActive;
    default:
        return {};
    }
}

QHash<int, QByteArray> QuestionListModel::roleNames() const {
    return {
        {QuestionIdRole, "questionId"},
        {DirectionRole, "direction"},
        {LevelRole, "level"},
        {TopicRole, "topic"},
        {QuestionTextRole, "questionText"},
        {IsActiveRole, "isActive"},
    };
}

void QuestionListModel::setQuestions(std::vector<models::QuestionBankItem> questions) {
    beginResetModel();
    m_questions = std::move(questions);
    endResetModel();
}

void QuestionListModel::prepend(models::QuestionBankItem question) {
    beginInsertRows(QModelIndex(), 0, 0);
    m_questions.insert(m_questions.begin(), std::move(question));
    endInsertRows();
}

bool QuestionListModel::removeById(const QString& id) {
    const int row = indexOfId(id);
    if (row < 0) {
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_questions.erase(m_questions.begin() + row);
    endRemoveRows();
    return true;
}

std::optional<bool> QuestionListModel::setActive(const QString& id, bool active) {
    const int row = indexOfId(id);
    if (row < 0) {
        return std::nullopt;
    }
    models::QuestionBankItem& item = m_questions[static_cast<std::size_t>(row)];
    const bool previous = item.isActive;
    if (previous != active) {
        item.isActive = active;
        const QModelIndex changed = index(row);
        emit dataChanged(changed, changed, {IsActiveRole});
    }
    return previous;
}

void QuestionListModel::replace(const models::QuestionBankItem& question) {
    const int row = indexOfId(QString::fromStdString(question.id));
    if (row < 0) {
        return;
    }
    m_questions[static_cast<std::size_t>(row)] = question;
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed);
}

int QuestionListModel::indexOfId(const QString& id) const {
    const std::string needle = id.toStdString();
    for (std::size_t i = 0; i < m_questions.size(); ++i) {
        if (m_questions[i].id == needle) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace devdialogue::viewmodels
