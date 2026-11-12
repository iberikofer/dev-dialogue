#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "services/MockAiEngine.hpp"

using devdialogue::models::InterviewDirection;
using devdialogue::models::InterviewLanguage;
using devdialogue::models::InterviewLevel;
using devdialogue::models::InterviewMessage;

namespace {

const std::vector<InterviewDirection> kDirections = {
    InterviewDirection::Frontend, InterviewDirection::Backend,
    InterviewDirection::DevOps, InterviewDirection::Qa, InterviewDirection::Pm};

const std::vector<InterviewLevel> kLevels = {InterviewLevel::Trainee,
                                             InterviewLevel::Junior,
                                             InterviewLevel::Middle};

bool containsCyrillic(const std::string &text) {
  for (const char character : text) {
    const auto code = static_cast<unsigned char>(character);
    if (code >= 0xD0 && code <= 0xD1)
      return true;
  }
  return false;
}

} // namespace

TEST(MockAiEngineTest, GeneratesAQuestionForEveryTrackAndLevel) {
  const devdialogue::services::MockAiEngine engine;

  for (const InterviewDirection direction : kDirections) {
    for (const InterviewLevel level : kLevels) {
      const std::string question = engine.generateFirstQuestion(
          direction, level, InterviewLanguage::En);

      EXPECT_FALSE(question.empty())
          << "empty question for direction " << static_cast<int>(direction)
          << " level " << static_cast<int>(level);
    }
  }
}

TEST(MockAiEngineTest, GeneratesDeterministicFirstQuestion) {
  const devdialogue::services::MockAiEngine engine;

  const std::string first = engine.generateFirstQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En);
  const std::string second = engine.generateFirstQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En);

  EXPECT_EQ(first, second);
  EXPECT_NE(first.find("Frontend"), std::string::npos);
  EXPECT_NE(first.find("Junior"), std::string::npos);
}

TEST(MockAiEngineTest, UkrainianAndEnglishPoolsDiffer) {
  const devdialogue::services::MockAiEngine engine;

  const std::string ukrainian = engine.generateFirstQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::Uk);
  const std::string english = engine.generateFirstQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En);

  EXPECT_TRUE(containsCyrillic(ukrainian));
  EXPECT_FALSE(containsCyrillic(english));
  EXPECT_NE(ukrainian, english);
}

TEST(MockAiEngineTest, NextQuestionWalksThroughThePool) {
  const devdialogue::services::MockAiEngine engine;

  const std::string first = engine.generateFirstQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En);
  const std::string second = engine.generateNextQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En, 1, "my answer");

  EXPECT_NE(first, second);
  EXPECT_FALSE(second.empty());
}

TEST(MockAiEngineTest, PoolExhaustionReturnsClosingMessage) {
  const devdialogue::services::MockAiEngine engine;

  const std::string closing = engine.generateNextQuestion(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En, 99, "my answer");

  EXPECT_NE(closing.find("Thank you"), std::string::npos);
}

TEST(MockAiEngineTest, EvaluatesTranscriptIntoQuestionScores) {
  const devdialogue::services::MockAiEngine engine;

  std::vector<InterviewMessage> transcript;
  transcript.push_back({"assistant", "What is the event loop?", "2026-01-01"});
  transcript.push_back(
      {"user", "Microtasks run after the current task completes.", "2026-01-01"});
  transcript.push_back({"assistant", "Explain closures.", "2026-01-01"});
  transcript.push_back({"user", "A closure keeps access to its lexical scope.",
                        "2026-01-01"});

  const auto report = engine.evaluateInterview(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En, transcript);

  EXPECT_EQ(report.questionScores.size(), 2);
  EXPECT_EQ(report.questionScores[0].question, "What is the event loop?");
  EXPECT_GE(report.score, 0);
  EXPECT_LE(report.score, 100);
  EXPECT_FALSE(report.summary.empty());
  EXPECT_FALSE(report.strengths.empty());
  EXPECT_FALSE(report.weaknesses.empty());
  EXPECT_FALSE(report.recommendations.empty());
}

TEST(MockAiEngineTest, DetailedAnswersScoreHigherThanShortOnes) {
  const devdialogue::services::MockAiEngine engine;

  std::vector<InterviewMessage> shortTranscript;
  shortTranscript.push_back({"assistant", "Question?", "2026-01-01"});
  shortTranscript.push_back({"user", "Yes.", "2026-01-01"});

  std::vector<InterviewMessage> detailedTranscript;
  detailedTranscript.push_back({"assistant", "Question?", "2026-01-01"});
  detailedTranscript.push_back(
      {"user",
       "I would use a queue because it keeps the ordering predictable, for "
       "example when the backend replays events, and the trade-off is extra "
       "latency which we can amortise with batching.",
       "2026-01-01"});

  const auto shortReport = engine.evaluateInterview(
      InterviewDirection::Backend, InterviewLevel::Middle,
      InterviewLanguage::En, shortTranscript);
  const auto detailedReport = engine.evaluateInterview(
      InterviewDirection::Backend, InterviewLevel::Middle,
      InterviewLanguage::En, detailedTranscript);

  ASSERT_EQ(shortReport.questionScores.size(), 1);
  ASSERT_EQ(detailedReport.questionScores.size(), 1);
  EXPECT_GT(detailedReport.questionScores[0].score,
            shortReport.questionScores[0].score);
}

TEST(MockAiEngineTest, UkrainianReportUsesUkrainianCopy) {
  const devdialogue::services::MockAiEngine engine;

  std::vector<InterviewMessage> transcript;
  transcript.push_back({"assistant", "Question?", "2026-01-01"});
  transcript.push_back({"user", "Answer.", "2026-01-01"});

  const auto report = engine.evaluateInterview(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::Uk, transcript);

  EXPECT_TRUE(containsCyrillic(report.summary));
  EXPECT_TRUE(containsCyrillic(report.strengths.front()));
  EXPECT_TRUE(containsCyrillic(report.questionScores.front().comment));
}

TEST(MockAiEngineTest, EmptyTranscriptStillProducesAReport) {
  const devdialogue::services::MockAiEngine engine;

  const auto report = engine.evaluateInterview(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En, {});

  EXPECT_TRUE(report.questionScores.empty());
  EXPECT_GE(report.score, 0);
  EXPECT_LE(report.score, 100);
}
