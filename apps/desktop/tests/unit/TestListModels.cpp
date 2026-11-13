#include <gtest/gtest.h>

#include <QObject>
#include <QString>

#include "viewmodels/InterviewListModel.hpp"
#include "viewmodels/MessageListModel.hpp"
#include "viewmodels/QuestionListModel.hpp"

using namespace devdialogue;
using namespace devdialogue::viewmodels;

namespace {

models::InterviewSummary makeInterview(const std::string& id, models::InterviewStatus status,
                                       std::optional<int> score) {
    models::InterviewSummary item;
    item.id = id;
    item.direction = models::InterviewDirection::Backend;
    item.level = models::InterviewLevel::Middle;
    item.status = status;
    item.score = score;
    item.createdAt = "2026-11-04T12:30:00Z";
    return item;
}

models::QuestionBankItem makeQuestion(const std::string& id, bool active = true) {
    models::QuestionBankItem item;
    item.id = id;
    item.direction = models::InterviewDirection::DevOps;
    item.level = models::InterviewLevel::Junior;
    item.topic = "Topic " + id;
    item.questionText = "Question " + id;
    item.isActive = active;
    return item;
}

QString text(const QAbstractItemModel& model, int row, int role) {
    return model.data(model.index(row, 0), role).toString();
}

} // namespace

// ---------------------------------------------------------------------------
// InterviewListModel
// ---------------------------------------------------------------------------

TEST(InterviewListModelTest, ExposesTheRoleNamesTheDashboardTableBindsTo) {
    InterviewListModel model;
    const QHash<int, QByteArray> roles = model.roleNames();

    EXPECT_EQ(roles.value(InterviewListModel::InterviewIdRole), "interviewId");
    EXPECT_EQ(roles.value(InterviewListModel::DateTextRole), "dateText");
    EXPECT_EQ(roles.value(InterviewListModel::DirectionRole), "direction");
    EXPECT_EQ(roles.value(InterviewListModel::LevelRole), "level");
    EXPECT_EQ(roles.value(InterviewListModel::IsCompletedRole), "isCompleted");
    EXPECT_EQ(roles.value(InterviewListModel::HasScoreRole), "hasScore");
    EXPECT_EQ(roles.value(InterviewListModel::ScoreRole), "score");
}

TEST(InterviewListModelTest, MapsContractValuesToDisplayData) {
    InterviewListModel model;
    model.setInterviews({
        makeInterview("done", models::InterviewStatus::Completed, 88),
        makeInterview("running", models::InterviewStatus::Active, std::nullopt),
    });

    ASSERT_EQ(model.rowCount(), 2);
    EXPECT_EQ(text(model, 0, InterviewListModel::InterviewIdRole).toStdString(), "done");
    EXPECT_EQ(text(model, 0, InterviewListModel::DirectionRole).toStdString(), "backend");
    EXPECT_EQ(text(model, 0, InterviewListModel::LevelRole).toStdString(), "middle");
    EXPECT_FALSE(text(model, 0, InterviewListModel::DateTextRole).isEmpty());
    EXPECT_TRUE(model.data(model.index(0, 0), InterviewListModel::IsCompletedRole).toBool());
    EXPECT_TRUE(model.data(model.index(0, 0), InterviewListModel::HasScoreRole).toBool());
    EXPECT_EQ(model.data(model.index(0, 0), InterviewListModel::ScoreRole).toInt(), 88);

    // An interview still in progress has no score yet.
    EXPECT_FALSE(model.data(model.index(1, 0), InterviewListModel::IsCompletedRole).toBool());
    EXPECT_FALSE(model.data(model.index(1, 0), InterviewListModel::HasScoreRole).toBool());
    EXPECT_EQ(model.data(model.index(1, 0), InterviewListModel::ScoreRole).toInt(), 0);
}

TEST(InterviewListModelTest, OutOfRangeRowsYieldNoData) {
    InterviewListModel model;
    model.setInterviews({makeInterview("only", models::InterviewStatus::Completed, 70)});

    EXPECT_FALSE(model.data(model.index(5, 0), InterviewListModel::InterviewIdRole).isValid());
    EXPECT_FALSE(model.data(QModelIndex(), InterviewListModel::InterviewIdRole).isValid());
}

// ---------------------------------------------------------------------------
// MessageListModel
// ---------------------------------------------------------------------------

TEST(MessageListModelTest, LoadingATranscriptDropsSystemMessages) {
    MessageListModel model;
    model.setMessages({
        {"system", "You are an interviewer.", "2026-11-04T12:00:00Z"},
        {"assistant", "Tell me about yourself.", "2026-11-04T12:00:01Z"},
        {"user", "I build web apps.", "2026-11-04T12:00:30Z"},
    });

    ASSERT_EQ(model.rowCount(), 2);
    EXPECT_EQ(text(model, 0, MessageListModel::RoleRole).toStdString(), "assistant");
    EXPECT_EQ(text(model, 1, MessageListModel::ContentRole).toStdString(), "I build web apps.");
    EXPECT_EQ(model.userMessageCount(), 1);
}

TEST(MessageListModelTest, AppendAddsOneRowAndStampsTheTime) {
    MessageListModel model;
    int insertedRows = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted, &model,
                     [&insertedRows](const QModelIndex&, int first, int last) { insertedRows += last - first + 1; });

    model.append("assistant", "First question");
    model.append("user", "First answer");

    EXPECT_EQ(insertedRows, 2);
    ASSERT_EQ(model.rowCount(), 2);
    EXPECT_EQ(text(model, 1, MessageListModel::ContentRole).toStdString(), "First answer");
    // "HH:mm"
    EXPECT_EQ(text(model, 1, MessageListModel::TimeTextRole).size(), 5);
    EXPECT_EQ(model.userMessageCount(), 1);
}

TEST(MessageListModelTest, ClearEmptiesTheTranscript) {
    MessageListModel model;
    model.append("user", "Something");
    model.clear();

    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.userMessageCount(), 0);
}

// ---------------------------------------------------------------------------
// QuestionListModel
// ---------------------------------------------------------------------------

TEST(QuestionListModelTest, PrependPutsTheNewQuestionFirst) {
    QuestionListModel model;
    model.setQuestions({makeQuestion("q-1"), makeQuestion("q-2")});
    model.prepend(makeQuestion("q-new"));

    ASSERT_EQ(model.rowCount(), 3);
    EXPECT_EQ(text(model, 0, QuestionListModel::QuestionIdRole).toStdString(), "q-new");
    EXPECT_EQ(text(model, 0, QuestionListModel::DirectionRole).toStdString(), "devops");
    EXPECT_EQ(text(model, 1, QuestionListModel::QuestionIdRole).toStdString(), "q-1");
}

TEST(QuestionListModelTest, RemoveByIdDeletesOnlyThatRow) {
    QuestionListModel model;
    model.setQuestions({makeQuestion("q-1"), makeQuestion("q-2"), makeQuestion("q-3")});

    EXPECT_TRUE(model.removeById("q-2"));
    EXPECT_FALSE(model.removeById("q-missing"));

    ASSERT_EQ(model.rowCount(), 2);
    EXPECT_EQ(text(model, 0, QuestionListModel::QuestionIdRole).toStdString(), "q-1");
    EXPECT_EQ(text(model, 1, QuestionListModel::QuestionIdRole).toStdString(), "q-3");
}

TEST(QuestionListModelTest, SetActiveReportsThePreviousStateAndNotifiesTheView) {
    QuestionListModel model;
    model.setQuestions({makeQuestion("q-1", true)});
    int changes = 0;
    QObject::connect(&model, &QAbstractItemModel::dataChanged, &model, [&changes]() { ++changes; });

    const std::optional<bool> previous = model.setActive("q-1", false);

    ASSERT_TRUE(previous.has_value());
    EXPECT_TRUE(*previous);
    EXPECT_FALSE(model.data(model.index(0, 0), QuestionListModel::IsActiveRole).toBool());
    EXPECT_EQ(changes, 1);

    // Setting the same value again changes nothing, and unknown ids are reported.
    EXPECT_EQ(model.setActive("q-1", false), std::optional<bool>(false));
    EXPECT_EQ(changes, 1);
    EXPECT_FALSE(model.setActive("q-missing", true).has_value());
}

TEST(QuestionListModelTest, ReplaceUpdatesTheRowWithTheSameId) {
    QuestionListModel model;
    model.setQuestions({makeQuestion("q-1"), makeQuestion("q-2")});

    models::QuestionBankItem edited = makeQuestion("q-2", false);
    edited.topic = "Edited topic";
    model.replace(edited);

    EXPECT_EQ(text(model, 1, QuestionListModel::TopicRole).toStdString(), "Edited topic");
    EXPECT_FALSE(model.data(model.index(1, 0), QuestionListModel::IsActiveRole).toBool());
    EXPECT_EQ(text(model, 0, QuestionListModel::TopicRole).toStdString(), "Topic q-1");
}
