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

struct InterviewDetail : public InterviewSummary {
  std::vector<InterviewMessage> transcript;
  std::optional<InterviewFeedbackReport> feedback;
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

struct QuestionBankItem {
  std::string id;
  InterviewDirection direction{InterviewDirection::Frontend};
  InterviewStage stage{InterviewStage::HardSkills};
  InterviewLevel level{InterviewLevel::Junior};
  std::string topic;
  std::string questionText;
  bool isActive{true};
  std::optional<std::string> createdBy;
  std::string createdAt;
};

struct CreateQuestionRequest {
  InterviewDirection direction;
  InterviewStage stage;
  InterviewLevel level;
  std::string topic;
  std::string questionText;
};

struct UpdateQuestionRequest {
  std::optional<InterviewDirection> direction;
  std::optional<InterviewStage> stage;
  std::optional<InterviewLevel> level;
  std::optional<std::string> topic;
  std::optional<std::string> questionText;
  std::optional<bool> isActive;
};

struct UserDashboardStats {
  int totalInterviews{0};
  double averageScore{0.0};
  int dailyInterviewsCount{0};
  int dailyLimit{10};
  int remainingToday{10};
};

template <typename T> struct PagedResult {
  std::vector<T> items;
  int page{1};
  int pageSize{20};
  int totalCount{0};
  std::optional<int> totalPages;
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

// The backend serializes question scores as `question_scores` (snake_case),
// unlike the rest of the camelCase payload, so this type needs an explicit
// serializer that accepts both spellings.
inline void to_json(nlohmann::json &j, const InterviewFeedbackReport &r) {
  j = nlohmann::json{{"summary", r.summary},
                     {"score", r.score},
                     {"strengths", r.strengths},
                     {"weaknesses", r.weaknesses},
                     {"recommendations", r.recommendations},
                     {"question_scores", r.questionScores}};
}

inline void from_json(const nlohmann::json &j, InterviewFeedbackReport &r) {
  if (j.contains("summary") && !j["summary"].is_null())
    j.at("summary").get_to(r.summary);
  if (j.contains("score") && !j["score"].is_null())
    j.at("score").get_to(r.score);
  if (j.contains("strengths") && j["strengths"].is_array())
    j.at("strengths").get_to(r.strengths);
  if (j.contains("weaknesses") && j["weaknesses"].is_array())
    j.at("weaknesses").get_to(r.weaknesses);
  if (j.contains("recommendations") && j["recommendations"].is_array())
    j.at("recommendations").get_to(r.recommendations);

  const nlohmann::json *scores = nullptr;
  if (j.contains("question_scores"))
    scores = &j.at("question_scores");
  else if (j.contains("questionScores"))
    scores = &j.at("questionScores");

  if (scores != nullptr && scores->is_array())
    r.questionScores = scores->get<std::vector<QuestionScoreItem>>();
}

inline void to_json(nlohmann::json &j, const InterviewSummary &s) {
  j = nlohmann::json{{"id", s.id},
                     {"userId", s.userId},
                     {"direction", s.direction},
                     {"level", s.level},
                     {"language", s.language},
                     {"stage", s.stage},
                     {"status", s.status},
                     {"score", s.score ? nlohmann::json(*s.score) : nlohmann::json(nullptr)},
                     {"questionCount", s.questionCount},
                     {"createdAt", s.createdAt},
                     {"updatedAt", s.updatedAt}};
}

inline void from_json(const nlohmann::json &j, InterviewSummary &s) {
  j.at("id").get_to(s.id);
  if (j.contains("userId") && !j["userId"].is_null())
    j.at("userId").get_to(s.userId);
  if (j.contains("direction") && !j["direction"].is_null())
    j.at("direction").get_to(s.direction);
  if (j.contains("level") && !j["level"].is_null())
    j.at("level").get_to(s.level);
  if (j.contains("language") && !j["language"].is_null())
    j.at("language").get_to(s.language);
  if (j.contains("stage") && !j["stage"].is_null())
    j.at("stage").get_to(s.stage);
  if (j.contains("status") && !j["status"].is_null())
    j.at("status").get_to(s.status);
  if (j.contains("score") && !j["score"].is_null())
    s.score = j["score"].get<int>();
  if (j.contains("questionCount") && !j["questionCount"].is_null())
    j.at("questionCount").get_to(s.questionCount);
  if (j.contains("createdAt") && !j["createdAt"].is_null())
    j.at("createdAt").get_to(s.createdAt);
  if (j.contains("updatedAt") && !j["updatedAt"].is_null())
    j.at("updatedAt").get_to(s.updatedAt);
}

inline void to_json(nlohmann::json &j, const InterviewDetail &d) {
  to_json(j, static_cast<const InterviewSummary &>(d));
  j["transcript"] = d.transcript;
  j["feedback"] =
      d.feedback ? nlohmann::json(*d.feedback) : nlohmann::json(nullptr);
}

inline void from_json(const nlohmann::json &j, InterviewDetail &d) {
  from_json(j, static_cast<InterviewSummary &>(d));
  if (j.contains("transcript") && j["transcript"].is_array())
    j.at("transcript").get_to(d.transcript);
  if (j.contains("feedback") && !j["feedback"].is_null())
    d.feedback = j.at("feedback").get<InterviewFeedbackReport>();
}

inline void to_json(nlohmann::json &j, const AuthMeResponse &r) {
  j = nlohmann::json{
      {"id", r.id},
      {"email", r.email},
      {"fullName", r.fullName ? nlohmann::json(*r.fullName)
                              : nlohmann::json(nullptr)},
      {"avatarUrl", r.avatarUrl ? nlohmann::json(*r.avatarUrl)
                                : nlohmann::json(nullptr)},
      {"role", r.role}};
}

inline void from_json(const nlohmann::json &j, AuthMeResponse &r) {
  j.at("id").get_to(r.id);
  if (j.contains("email") && !j["email"].is_null())
    j.at("email").get_to(r.email);
  if (j.contains("fullName") && !j["fullName"].is_null())
    r.fullName = j.at("fullName").get<std::string>();
  if (j.contains("avatarUrl") && !j["avatarUrl"].is_null())
    r.avatarUrl = j.at("avatarUrl").get<std::string>();
  if (j.contains("role") && !j["role"].is_null())
    j.at("role").get_to(r.role);
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateInterviewRequest, direction, level,
                                   language)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateInterviewResponse, interviewId,
                                   firstQuestion)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageRequest, content)

inline void to_json(nlohmann::json &j, const SendMessageResponse &r) {
  j = nlohmann::json{
      {"message", r.message},
      {"questionCount", r.questionCount},
      {"isCompleted", r.isCompleted ? nlohmann::json(*r.isCompleted)
                                    : nlohmann::json(nullptr)}};
}

inline void from_json(const nlohmann::json &j, SendMessageResponse &r) {
  if (j.contains("message") && !j["message"].is_null())
    j.at("message").get_to(r.message);
  if (j.contains("questionCount") && !j["questionCount"].is_null())
    j.at("questionCount").get_to(r.questionCount);
  if (j.contains("isCompleted") && !j["isCompleted"].is_null())
    r.isCompleted = j.at("isCompleted").get<bool>();
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompleteInterviewResponse, score, feedback)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDashboardStats, totalInterviews,
                                   averageScore, dailyInterviewsCount,
                                   dailyLimit, remainingToday)

inline void to_json(nlohmann::json &j, const QuestionBankItem &q) {
  j = nlohmann::json{{"id", q.id},
                     {"direction", q.direction},
                     {"stage", q.stage},
                     {"level", q.level},
                     {"topic", q.topic},
                     {"questionText", q.questionText},
                     {"isActive", q.isActive},
                     {"createdBy", q.createdBy ? nlohmann::json(*q.createdBy)
                                               : nlohmann::json(nullptr)},
                     {"createdAt", q.createdAt}};
}

inline void from_json(const nlohmann::json &j, QuestionBankItem &q) {
  j.at("id").get_to(q.id);
  if (j.contains("direction") && !j["direction"].is_null())
    j.at("direction").get_to(q.direction);
  if (j.contains("stage") && !j["stage"].is_null())
    j.at("stage").get_to(q.stage);
  if (j.contains("level") && !j["level"].is_null())
    j.at("level").get_to(q.level);
  if (j.contains("topic") && !j["topic"].is_null())
    j.at("topic").get_to(q.topic);
  if (j.contains("questionText") && !j["questionText"].is_null())
    j.at("questionText").get_to(q.questionText);
  if (j.contains("isActive") && !j["isActive"].is_null())
    j.at("isActive").get_to(q.isActive);
  if (j.contains("createdBy") && !j["createdBy"].is_null())
    q.createdBy = j.at("createdBy").get<std::string>();
  if (j.contains("createdAt") && !j["createdAt"].is_null())
    j.at("createdAt").get_to(q.createdAt);
}

inline void to_json(nlohmann::json &j, const CreateQuestionRequest &q) {
  j = nlohmann::json{{"direction", q.direction},
                     {"stage", q.stage},
                     {"level", q.level},
                     {"topic", q.topic},
                     {"questionText", q.questionText}};
}

inline void to_json(nlohmann::json &j, const UpdateQuestionRequest &q) {
  j = nlohmann::json::object();
  if (q.direction)
    j["direction"] = *q.direction;
  if (q.stage)
    j["stage"] = *q.stage;
  if (q.level)
    j["level"] = *q.level;
  if (q.topic)
    j["topic"] = *q.topic;
  if (q.questionText)
    j["questionText"] = *q.questionText;
  if (q.isActive)
    j["isActive"] = *q.isActive;
}

template <typename T>
inline void to_json(nlohmann::json &j, const PagedResult<T> &p) {
  j = nlohmann::json{{"items", p.items},
                     {"page", p.page},
                     {"pageSize", p.pageSize},
                     {"totalCount", p.totalCount},
                     {"totalPages", p.totalPages ? nlohmann::json(*p.totalPages)
                                                 : nlohmann::json(nullptr)}};
}

template <typename T>
inline void from_json(const nlohmann::json &j, PagedResult<T> &p) {
  if (j.contains("items") && j["items"].is_array())
    j.at("items").get_to(p.items);
  if (j.contains("page") && !j["page"].is_null())
    j.at("page").get_to(p.page);
  if (j.contains("pageSize") && !j["pageSize"].is_null())
    j.at("pageSize").get_to(p.pageSize);
  if (j.contains("totalCount") && !j["totalCount"].is_null())
    j.at("totalCount").get_to(p.totalCount);
  if (j.contains("totalPages") && !j["totalPages"].is_null())
    p.totalPages = j.at("totalPages").get<int>();
  else if (p.pageSize > 0)
    p.totalPages = (p.totalCount + p.pageSize - 1) / p.pageSize;
}

} // namespace devdialogue::models
