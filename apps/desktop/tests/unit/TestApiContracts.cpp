#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "models/ApiContracts.hpp"

using namespace devdialogue::models;

namespace {

nlohmann::json backendSummaryPayload() {
  return nlohmann::json{
      {"id", "interview-123"},
      {"userId", "user-abc"},
      {"direction", "backend"},
      {"level", "middle"},
      {"language", "en"},
      {"stage", "soft_skills"},
      {"status", "completed"},
      {"score", 85},
      {"questionCount", 5},
      {"createdAt", "2026-11-04T12:00:00Z"},
      {"updatedAt", "2026-11-04T12:30:00Z"}};
}

} // namespace

// ---------------------------------------------------------------------------
// Enum mapping
// ---------------------------------------------------------------------------

TEST(ApiContractsTest, EnumsSerializeToBackendIdentifiers) {
  EXPECT_EQ(nlohmann::json(InterviewDirection::Frontend), "frontend");
  EXPECT_EQ(nlohmann::json(InterviewDirection::Backend), "backend");
  EXPECT_EQ(nlohmann::json(InterviewDirection::DevOps), "devops");
  EXPECT_EQ(nlohmann::json(InterviewDirection::Qa), "qa");
  EXPECT_EQ(nlohmann::json(InterviewDirection::Pm), "pm");

  EXPECT_EQ(nlohmann::json(InterviewLevel::Trainee), "trainee");
  EXPECT_EQ(nlohmann::json(InterviewLevel::Junior), "junior");
  EXPECT_EQ(nlohmann::json(InterviewLevel::Middle), "middle");

  EXPECT_EQ(nlohmann::json(InterviewLanguage::Uk), "uk");
  EXPECT_EQ(nlohmann::json(InterviewLanguage::En), "en");

  EXPECT_EQ(nlohmann::json(InterviewStage::HardSkills), "hard_skills");
  EXPECT_EQ(nlohmann::json(InterviewStage::SoftSkills), "soft_skills");
  EXPECT_EQ(nlohmann::json(InterviewStage::English), "english");

  EXPECT_EQ(nlohmann::json(InterviewStatus::Active), "active");
  EXPECT_EQ(nlohmann::json(InterviewStatus::Completed), "completed");
  EXPECT_EQ(nlohmann::json(InterviewStatus::Abandoned), "abandoned");

  EXPECT_EQ(nlohmann::json(UserRole::User), "user");
  EXPECT_EQ(nlohmann::json(UserRole::Admin), "admin");
}

// ---------------------------------------------------------------------------
// Interview summary
// ---------------------------------------------------------------------------

TEST(ApiContractsTest, SerializeInterviewSummary) {
  InterviewSummary summary;
  summary.id = "interview-123";
  summary.userId = "user-abc";
  summary.direction = InterviewDirection::Frontend;
  summary.level = InterviewLevel::Junior;
  summary.language = InterviewLanguage::Uk;
  summary.stage = InterviewStage::HardSkills;
  summary.status = InterviewStatus::Completed;
  summary.score = 85;
  summary.questionCount = 5;
  summary.createdAt = "2026-11-04T12:00:00Z";
  summary.updatedAt = "2026-11-04T12:30:00Z";

  const nlohmann::json document = summary;

  EXPECT_EQ(document.at("id"), "interview-123");
  EXPECT_EQ(document.at("userId"), "user-abc");
  EXPECT_EQ(document.at("direction"), "frontend");
  EXPECT_EQ(document.at("level"), "junior");
  EXPECT_EQ(document.at("language"), "uk");
  EXPECT_EQ(document.at("stage"), "hard_skills");
  EXPECT_EQ(document.at("status"), "completed");
  EXPECT_EQ(document.at("score"), 85);
  EXPECT_EQ(document.at("questionCount"), 5);
}

TEST(ApiContractsTest, MissingScoreSerializesAsNull) {
  InterviewSummary summary;
  summary.id = "interview-empty";

  const nlohmann::json document = summary;
  EXPECT_TRUE(document.at("score").is_null());
}

TEST(ApiContractsTest, DeserializeInterviewSummary) {
  const InterviewSummary summary =
      backendSummaryPayload().get<InterviewSummary>();

  EXPECT_EQ(summary.id, "interview-123");
  EXPECT_EQ(summary.userId, "user-abc");
  EXPECT_EQ(summary.direction, InterviewDirection::Backend);
  EXPECT_EQ(summary.level, InterviewLevel::Middle);
  EXPECT_EQ(summary.language, InterviewLanguage::En);
  EXPECT_EQ(summary.stage, InterviewStage::SoftSkills);
  EXPECT_EQ(summary.status, InterviewStatus::Completed);
  ASSERT_TRUE(summary.score.has_value());
  EXPECT_EQ(*summary.score, 85);
  EXPECT_EQ(summary.questionCount, 5);
}

TEST(ApiContractsTest, InterviewSummaryRoundTrip) {
  const nlohmann::json document = backendSummaryPayload();
  const InterviewSummary summary = document.get<InterviewSummary>();
  const nlohmann::json roundTripped = summary;

  EXPECT_EQ(roundTripped, document);
}

TEST(ApiContractsTest, DeserializeInterviewSummaryWithoutOptionalFields) {
  const nlohmann::json document = nlohmann::json{
      {"id", "interview-456"}, {"direction", "qa"}, {"status", "active"}};

  const InterviewSummary summary = document.get<InterviewSummary>();

  EXPECT_EQ(summary.id, "interview-456");
  EXPECT_EQ(summary.direction, InterviewDirection::Qa);
  EXPECT_EQ(summary.status, InterviewStatus::Active);
  EXPECT_FALSE(summary.score.has_value());
}

// ---------------------------------------------------------------------------
// Interview detail (transcript + feedback)
// ---------------------------------------------------------------------------

TEST(ApiContractsTest, InterviewDetailRoundTripKeepsTranscript) {
  InterviewDetail detail;
  detail.id = "interview-detail";
  detail.direction = InterviewDirection::DevOps;
  detail.status = InterviewStatus::Completed;
  detail.score = 72;

  InterviewMessage question;
  question.role = "assistant";
  question.content = "How does the event loop handle microtasks?";
  question.timestamp = "2026-11-04T12:00:00Z";

  InterviewMessage answer;
  answer.role = "user";
  answer.content = "Microtasks are drained after the current task.";
  answer.timestamp = "2026-11-04T12:01:00Z";

  detail.transcript = {question, answer};

  InterviewFeedbackReport feedback;
  feedback.summary = "Solid understanding.";
  feedback.score = 72;
  feedback.strengths = {"Clear reasoning"};
  feedback.weaknesses = {"Missing trade-offs"};
  feedback.recommendations = {{"System Design Primer",
                               "https://github.com/donnemartin/system-design-primer"}};
  feedback.questionScores = {{"How does the event loop handle microtasks?", 72,
                              "Logical answer."}};
  detail.feedback = feedback;

  const nlohmann::json document = detail;
  EXPECT_EQ(document.at("transcript").size(), 2);
  EXPECT_EQ(document.at("feedback").at("question_scores").size(), 1);

  const InterviewDetail restored = document.get<InterviewDetail>();
  EXPECT_EQ(restored.transcript.size(), 2);
  EXPECT_EQ(restored.transcript[0].role, "assistant");
  EXPECT_EQ(restored.transcript[1].content,
            "Microtasks are drained after the current task.");
  ASSERT_TRUE(restored.feedback.has_value());
  EXPECT_EQ(restored.feedback->score, 72);
  EXPECT_EQ(restored.feedback->strengths.size(), 1);
  EXPECT_EQ(restored.feedback->recommendations.size(), 1);
  EXPECT_EQ(restored.feedback->questionScores.size(), 1);
}

TEST(ApiContractsTest, FeedbackAcceptsSnakeCaseAndCamelCaseQuestionScores) {
  const nlohmann::json snakeCase = nlohmann::json{
      {"summary", "ok"},
      {"score", 80},
      {"question_scores",
       nlohmann::json::array({nlohmann::json{{"question", "Q?"},
                                              {"score", 80},
                                              {"comment", "fine"}}})}};

  const nlohmann::json camelCase = nlohmann::json{
      {"summary", "ok"},
      {"score", 80},
      {"questionScores",
       nlohmann::json::array({nlohmann::json{{"question", "Q?"},
                                              {"score", 80},
                                              {"comment", "fine"}}})}};

  const InterviewFeedbackReport fromSnake =
      snakeCase.get<InterviewFeedbackReport>();
  const InterviewFeedbackReport fromCamel =
      camelCase.get<InterviewFeedbackReport>();

  EXPECT_EQ(fromSnake.questionScores.size(), 1);
  EXPECT_EQ(fromCamel.questionScores.size(), 1);
  EXPECT_EQ(fromSnake.questionScores[0].question, "Q?");
  EXPECT_EQ(fromCamel.questionScores[0].comment, "fine");
}

// ---------------------------------------------------------------------------
// Paged results
// ---------------------------------------------------------------------------

TEST(ApiContractsTest, PagedResultComputesTotalPagesWhenMissing) {
  const nlohmann::json document = nlohmann::json{
      {"items", nlohmann::json::array()},
      {"page", 1},
      {"pageSize", 20},
      {"totalCount", 45}};

  const PagedResult<QuestionBankItem> page =
      document.get<PagedResult<QuestionBankItem>>();

  EXPECT_EQ(page.page, 1);
  EXPECT_EQ(page.pageSize, 20);
  EXPECT_EQ(page.totalCount, 45);
  ASSERT_TRUE(page.totalPages.has_value());
  EXPECT_EQ(*page.totalPages, 3);
}

TEST(ApiContractsTest, PagedResultRoundTrip) {
  PagedResult<QuestionBankItem> page;
  page.page = 2;
  page.pageSize = 10;
  page.totalCount = 12;
  page.totalPages = 2;

  QuestionBankItem item;
  item.id = "question-1";
  item.direction = InterviewDirection::Frontend;
  item.stage = InterviewStage::HardSkills;
  item.level = InterviewLevel::Junior;
  item.topic = "JavaScript";
  item.questionText = "Explain the event loop.";
  item.isActive = true;
  page.items = {item};

  const nlohmann::json document = page;
  const PagedResult<QuestionBankItem> restored =
      document.get<PagedResult<QuestionBankItem>>();

  EXPECT_EQ(restored.items.size(), 1);
  EXPECT_EQ(restored.items[0].id, "question-1");
  EXPECT_EQ(restored.items[0].isActive, true);
  EXPECT_EQ(restored.page, 2);
  EXPECT_EQ(restored.totalCount, 12);
}

// ---------------------------------------------------------------------------
// Question bank payloads
// ---------------------------------------------------------------------------

TEST(ApiContractsTest, UpdateQuestionRequestOnlySerializesProvidedFields) {
  UpdateQuestionRequest request;
  request.isActive = false;

  const nlohmann::json document = request;

  EXPECT_EQ(document.size(), 1);
  EXPECT_EQ(document.at("isActive"), false);
}

TEST(ApiContractsTest, QuestionBankItemRoundTripKeepsOptionalAuthor) {
  QuestionBankItem item;
  item.id = "question-7";
  item.direction = InterviewDirection::Pm;
  item.stage = InterviewStage::SoftSkills;
  item.level = InterviewLevel::Middle;
  item.topic = "Discovery";
  item.questionText = "Tell me about a time you disagreed with a teammate.";
  item.isActive = false;
  item.createdBy = "admin-1";

  const nlohmann::json document = item;
  const QuestionBankItem restored = document.get<QuestionBankItem>();

  EXPECT_EQ(restored.id, "question-7");
  EXPECT_EQ(restored.direction, InterviewDirection::Pm);
  EXPECT_EQ(restored.stage, InterviewStage::SoftSkills);
  EXPECT_EQ(restored.level, InterviewLevel::Middle);
  EXPECT_EQ(restored.isActive, false);
  ASSERT_TRUE(restored.createdBy.has_value());
  EXPECT_EQ(*restored.createdBy, "admin-1");
}

// ---------------------------------------------------------------------------
// Authentication payloads
// ---------------------------------------------------------------------------

TEST(ApiContractsTest, AuthMeResponseRoundTrip) {
  AuthMeResponse response;
  response.id = "user-1";
  response.email = "user@example.com";
  response.fullName = "Ada Lovelace";
  response.role = UserRole::Admin;

  const nlohmann::json document = response;
  EXPECT_EQ(document.at("role"), "admin");

  const AuthMeResponse restored = document.get<AuthMeResponse>();
  EXPECT_EQ(restored.id, "user-1");
  EXPECT_EQ(restored.email, "user@example.com");
  ASSERT_TRUE(restored.fullName.has_value());
  EXPECT_EQ(*restored.fullName, "Ada Lovelace");
  EXPECT_EQ(restored.role, UserRole::Admin);
}

TEST(ApiContractsTest, SendMessageResponseKeepsOptionalCompletionFlag) {
  SendMessageResponse response;
  response.message = "Next question";
  response.questionCount = 2;

  const nlohmann::json document = response;
  EXPECT_TRUE(document.at("isCompleted").is_null());

  response.isCompleted = true;
  const nlohmann::json withFlag = response;
  EXPECT_EQ(withFlag.at("isCompleted"), true);
}
