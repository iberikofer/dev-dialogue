#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "models/ApiContracts.hpp"

namespace devdialogue::services {

/// Fully offline interview simulator. Mirrors the behaviour of the backend's
/// MockAiService so the desktop UI remains usable without a running API.
class MockAiEngine {
public:
  MockAiEngine();

  std::string generateFirstQuestion(models::InterviewDirection direction,
                                    models::InterviewLevel level,
                                    models::InterviewLanguage language) const;

  std::string generateNextQuestion(models::InterviewDirection direction,
                                   models::InterviewLevel level,
                                   models::InterviewLanguage language,
                                   int currentQuestionCount,
                                   const std::string &candidateAnswer) const;

  models::InterviewFeedbackReport
  evaluateInterview(models::InterviewDirection direction,
                    models::InterviewLevel level,
                    models::InterviewLanguage language,
                    const std::vector<models::InterviewMessage> &transcript) const;

private:
  struct QuestionPool {
    std::vector<std::string> questionsUk;
    std::vector<std::string> questionsEn;
  };

  static int directionKey(models::InterviewDirection direction);
  static int levelKey(models::InterviewLevel level);

  const QuestionPool &poolFor(models::InterviewDirection direction,
                              models::InterviewLevel level) const;

  void initializeQuestionBank();
  void addPool(models::InterviewDirection direction,
               models::InterviewLevel level, std::vector<std::string> uk,
               std::vector<std::string> en);

  static int calculateAnswerScore(const std::string &answer);

  std::unordered_map<int, std::unordered_map<int, QuestionPool>> m_questionBank;
};

} // namespace devdialogue::services
