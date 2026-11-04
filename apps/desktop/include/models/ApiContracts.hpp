#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace devdialogue::models {

enum class UserRole { User, Admin };

enum class InterviewDirection { Frontend, Backend, DevOps, Pm, Qa };

enum class InterviewLevel { Trainee, Junior, Middle };

enum class InterviewLanguage { Uk, En };

enum class InterviewStage { HardSkills, SoftSkills, English };

enum class InterviewStatus { Active, Completed, Abandoned };

struct UserProfile {
  std::string id;
  std::string email;
  std::optional<std::string> fullName;
  std::optional<std::string> avatarUrl;
  UserRole role{UserRole::User};
  int dailyInterviewsCount{0};
  std::optional<std::string> lastInterviewDate;
  std::string createdAt;
};

struct AuthMeResponse {
  std::string id;
  std::string email;
  std::optional<std::string> fullName;
  std::optional<std::string> avatarUrl;
  UserRole role{UserRole::User};
};

struct InterviewMessage {
  std::string role; // "system" | "assistant" | "user"
  std::string content;
  std::string timestamp;
};

struct QuestionScoreItem {
  std::string question;
  int score{0};
  std::string comment;
};

struct RecommendationItem {
  std::string topic;
  std::string resource;
};

struct InterviewFeedbackReport {
  std::string summary;
  int score{0};
  std::vector<std::string> strengths;
  std::vector<std::string> weaknesses;
  std::vector<RecommendationItem> recommendations;
  std::vector<QuestionScoreItem> questionScores;
};

struct InterviewSummary {
  std::string id;
  std::string userId;
  InterviewDirection direction{InterviewDirection::Frontend};
  InterviewLevel level{InterviewLevel::Junior};
  InterviewLanguage language{InterviewLanguage::Uk};
  InterviewStage stage{InterviewStage::HardSkills};
  InterviewStatus status{InterviewStatus::Active};
  std::optional<int> score;
  int questionCount{0};
  std::string createdAt;
  std::string updatedAt;
};

struct CreateInterviewRequest {
  InterviewDirection direction;
  InterviewLevel level;
  InterviewLanguage language;
};

struct CreateInterviewResponse {
  std::string interviewId;
  std::string firstQuestion;
};

struct SendMessageRequest {
  std::string content;
};

struct SendMessageResponse {
  std::string message;
  int questionCount{0};
  std::optional<bool> isCompleted;
};

struct CompleteInterviewResponse {
  int score{0};
  InterviewFeedbackReport feedback;
};

struct UserDashboardStats {
  int totalInterviews{0};
  double averageScore{0.0};
  int dailyInterviewsCount{0};
  int dailyLimit{10};
  int remainingToday{10};
};

// ==========================================
// Enums Serialization
// ==========================================

NLOHMANN_JSON_SERIALIZE_ENUM(UserRole, {{UserRole::User, "user"},
                                        {UserRole::Admin, "admin"}})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewDirection,
                             {{InterviewDirection::Frontend, "frontend"},
                              {InterviewDirection::Backend, "backend"},
                              {InterviewDirection::DevOps, "devops"},
                              {InterviewDirection::Pm, "pm"},
                              {InterviewDirection::Qa, "qa"}})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewLevel,
                             {{InterviewLevel::Trainee, "trainee"},
                              {InterviewLevel::Junior, "junior"},
                              {InterviewLevel::Middle, "middle"}})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewLanguage, {{InterviewLanguage::Uk, "uk"},
                                                 {InterviewLanguage::En, "en"}})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewStage,
                             {{InterviewStage::HardSkills, "hard_skills"},
                              {InterviewStage::SoftSkills, "soft_skills"},
                              {InterviewStage::English, "english"}})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewStatus,
                             {{InterviewStatus::Active, "active"},
                              {InterviewStatus::Completed, "completed"},
                              {InterviewStatus::Abandoned, "abandoned"}})

// ==========================================
// Structs Serialization
// ==========================================

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InterviewMessage, role, content, timestamp)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QuestionScoreItem, question, score, comment)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RecommendationItem, topic, resource)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InterviewFeedbackReport, summary, score,
                                   strengths, weaknesses, recommendations,
                                   questionScores)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthMeResponse, id, email, fullName,
                                   avatarUrl, role)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateInterviewRequest, direction, level,
                                   language)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateInterviewResponse, interviewId,
                                   firstQuestion)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageRequest, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageResponse, message, questionCount,
                                   isCompleted)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompleteInterviewResponse, score, feedback)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDashboardStats, totalInterviews,
                                   averageScore, dailyInterviewsCount,
                                   dailyLimit, remainingToday)

} // namespace devdialogue::models
