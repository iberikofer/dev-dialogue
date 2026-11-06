#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace devdialogue::models {

// ==========================================
// Strongly-Typed Enums with Unknown = 0
// ==========================================

enum class UserRole {
    Unknown = 0,
    User,
    Admin
};

enum class InterviewDirection {
    Unknown = 0,
    Frontend,
    Backend,
    DevOps,
    Pm,
    Qa
};

enum class InterviewLevel {
    Unknown = 0,
    Trainee,
    Junior,
    Middle,
    Senior
};

enum class InterviewLanguage {
    Unknown = 0,
    Uk,
    En
};

enum class InterviewStage {
    Unknown = 0,
    HardSkills,
    SoftSkills,
    English
};

enum class InterviewStatus {
    Unknown = 0,
    Active,
    Completed,
    Abandoned
};

// ==========================================
// DTOs & Domain Entities
// ==========================================

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
    InterviewDirection direction{InterviewDirection::Frontend};
    InterviewLevel level{InterviewLevel::Junior};
    InterviewLanguage language{InterviewLanguage::Uk};
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
    InterviewDirection direction{InterviewDirection::Frontend};
    InterviewStage stage{InterviewStage::HardSkills};
    InterviewLevel level{InterviewLevel::Junior};
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

template<typename T>
struct PagedResult {
    std::vector<T> items;
    int page{1};
    int pageSize{20};
    int totalCount{0};
    std::optional<int> totalPages;
};

struct SupportedLanguageOption {
    std::string id;
    std::string code;
    std::string name;
};

struct OptionsResponse {
    std::vector<std::string> directions;
    std::vector<std::string> levels;
    std::vector<std::string> stages;
    std::vector<SupportedLanguageOption> languages;
};

// ==========================================
// Comprehensive Unified ApiError Model
// ==========================================

struct ApiError {
    int statusCode{0};
    std::string code;
    std::string message;
    std::string rawPayload;
    bool isNetworkDown{false};
    std::unordered_map<std::string, std::vector<std::string>> validationErrors;

    static ApiError fromJson(int statusCode, const std::string& rawPayload) {
        ApiError err;
        err.statusCode = statusCode;
        err.rawPayload = rawPayload;

        if (rawPayload.empty()) {
            err.message = "Empty response from server";
            return err;
        }

        try {
            auto j = nlohmann::json::parse(rawPayload);

            // 1. Supabase Auth errors: { "error": "...", "error_description": "..." }
            if (j.contains("error_description") && j["error_description"].is_string()) {
                err.message = j["error_description"].get<std::string>();
                if (j.contains("error") && j["error"].is_string()) {
                    err.code = j["error"].get<std::string>();
                }
                return err;
            }

            // 2. Supabase Auth alternative: { "msg": "..." } or { "error_code": "...", "message": "..." }
            if (j.contains("msg") && j["msg"].is_string()) {
                err.message = j["msg"].get<std::string>();
                if (j.contains("code")) {
                    err.code = j["code"].is_number() ? std::to_string(j["code"].get<int>()) : j["code"].get<std::string>();
                }
                return err;
            }

            // 3. ProblemDetails: { "title": "...", "detail": "...", "errors": { "field": ["..."] } }
            if (j.contains("errors") && j["errors"].is_object()) {
                for (auto& [field, msgs] : j["errors"].items()) {
                    if (msgs.is_array()) {
                        std::vector<std::string> list;
                        for (auto& m : msgs) {
                            if (m.is_string()) list.push_back(m.get<std::string>());
                        }
                        err.validationErrors[field] = list;
                    }
                }
            }

            if (j.contains("title") && j["title"].is_string()) {
                err.message = j["title"].get<std::string>();
                if (j.contains("detail") && j["detail"].is_string()) {
                    err.message += ": " + j["detail"].get<std::string>();
                }
                if (j.contains("status") && j["status"].is_number()) {
                    err.code = std::to_string(j["status"].get<int>());
                }
                return err;
            }

            // 4. Custom ASP.NET error: { "error": "...", "code": "...", "message": "..." }
            if (j.contains("code") && j["code"].is_string()) {
                err.code = j["code"].get<std::string>();
            }

            if (j.contains("message") && j["message"].is_string()) {
                err.message = j["message"].get<std::string>();
            } else if (j.contains("error") && j["error"].is_string()) {
                err.message = j["error"].get<std::string>();
            }

        } catch (...) {
            err.message = rawPayload;
        }

        if (err.message.empty()) {
            err.message = "HTTP Error " + std::to_string(statusCode);
        }

        return err;
    }
};

// ==========================================
// Robust Enums Serialization (Fallback to Unknown)
// ==========================================

inline void to_json(nlohmann::json& j, const UserRole& r) {
    switch (r) {
        case UserRole::Admin: j = "admin"; break;
        case UserRole::User: j = "user"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, UserRole& r) {
    if (j.is_string()) {
        std::string s = j.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (s == "admin") { r = UserRole::Admin; return; }
        if (s == "user") { r = UserRole::User; return; }
    }
    r = UserRole::Unknown;
}

inline void to_json(nlohmann::json& j, const InterviewDirection& d) {
    switch (d) {
        case InterviewDirection::Frontend: j = "frontend"; break;
        case InterviewDirection::Backend: j = "backend"; break;
        case InterviewDirection::DevOps: j = "devops"; break;
        case InterviewDirection::Pm: j = "pm"; break;
        case InterviewDirection::Qa: j = "qa"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, InterviewDirection& d) {
    if (j.is_string()) {
        std::string s = j.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (s == "frontend") { d = InterviewDirection::Frontend; return; }
        if (s == "backend") { d = InterviewDirection::Backend; return; }
        if (s == "devops") { d = InterviewDirection::DevOps; return; }
        if (s == "pm") { d = InterviewDirection::Pm; return; }
        if (s == "qa") { d = InterviewDirection::Qa; return; }
    }
    d = InterviewDirection::Unknown;
}

inline void to_json(nlohmann::json& j, const InterviewLevel& l) {
    switch (l) {
        case InterviewLevel::Trainee: j = "trainee"; break;
        case InterviewLevel::Junior: j = "junior"; break;
        case InterviewLevel::Middle: j = "middle"; break;
        case InterviewLevel::Senior: j = "senior"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, InterviewLevel& l) {
    if (j.is_string()) {
        std::string s = j.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (s == "trainee") { l = InterviewLevel::Trainee; return; }
        if (s == "junior") { l = InterviewLevel::Junior; return; }
        if (s == "middle") { l = InterviewLevel::Middle; return; }
        if (s == "senior") { l = InterviewLevel::Senior; return; }
    }
    l = InterviewLevel::Unknown;
}

inline void to_json(nlohmann::json& j, const InterviewLanguage& l) {
    switch (l) {
        case InterviewLanguage::Uk: j = "uk"; break;
        case InterviewLanguage::En: j = "en"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, InterviewLanguage& l) {
    if (j.is_string()) {
        std::string s = j.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (s == "uk") { l = InterviewLanguage::Uk; return; }
        if (s == "en") { l = InterviewLanguage::En; return; }
    }
    l = InterviewLanguage::Unknown;
}

inline void to_json(nlohmann::json& j, const InterviewStage& s) {
    switch (s) {
        case InterviewStage::HardSkills: j = "hard_skills"; break;
        case InterviewStage::SoftSkills: j = "soft_skills"; break;
        case InterviewStage::English: j = "english"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, InterviewStage& st) {
    if (j.is_string()) {
        std::string s = j.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (s == "hard_skills" || s == "hardskills") { st = InterviewStage::HardSkills; return; }
        if (s == "soft_skills" || s == "softskills") { st = InterviewStage::SoftSkills; return; }
        if (s == "english") { st = InterviewStage::English; return; }
    }
    st = InterviewStage::Unknown;
}

inline void to_json(nlohmann::json& j, const InterviewStatus& s) {
    switch (s) {
        case InterviewStatus::Active: j = "active"; break;
        case InterviewStatus::Completed: j = "completed"; break;
        case InterviewStatus::Abandoned: j = "abandoned"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, InterviewStatus& s) {
    if (j.is_string()) {
        std::string str = j.get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (str == "active") { s = InterviewStatus::Active; return; }
        if (str == "completed") { s = InterviewStatus::Completed; return; }
        if (str == "abandoned") { s = InterviewStatus::Abandoned; return; }
    }
    s = InterviewStatus::Unknown;
}

} // namespace devdialogue::models

// ==========================================
// Custom nlohmann::adl_serializer for std::optional
// ==========================================

namespace nlohmann {

template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt.has_value()) {
            j = *opt;
        } else {
            j = nullptr;
        }
    }

    static void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();
        }
    }
};

} // namespace nlohmann

namespace devdialogue::models {

// ==========================================
// Struct Serialization (to_json & from_json)
// ==========================================

inline void to_json(nlohmann::json& j, const UserProfile& u) {
    j = nlohmann::json{
        {"id", u.id},
        {"email", u.email},
        {"fullName", u.fullName},
        {"avatarUrl", u.avatarUrl},
        {"role", u.role},
        {"dailyInterviewsCount", u.dailyInterviewsCount},
        {"lastInterviewDate", u.lastInterviewDate},
        {"createdAt", u.createdAt}
    };
}

inline void from_json(const nlohmann::json& j, UserProfile& u) {
    if (j.contains("id")) j.at("id").get_to(u.id);
    if (j.contains("email")) j.at("email").get_to(u.email);
    if (j.contains("fullName")) j.at("fullName").get_to(u.fullName);
    if (j.contains("avatarUrl")) j.at("avatarUrl").get_to(u.avatarUrl);
    if (j.contains("role")) j.at("role").get_to(u.role);
    if (j.contains("dailyInterviewsCount")) j.at("dailyInterviewsCount").get_to(u.dailyInterviewsCount);
    if (j.contains("lastInterviewDate")) j.at("lastInterviewDate").get_to(u.lastInterviewDate);
    if (j.contains("createdAt")) j.at("createdAt").get_to(u.createdAt);
}

inline void to_json(nlohmann::json& j, const AuthMeResponse& a) {
    j = nlohmann::json{
        {"id", a.id},
        {"email", a.email},
        {"fullName", a.fullName},
        {"avatarUrl", a.avatarUrl},
        {"role", a.role}
    };
}

inline void from_json(const nlohmann::json& j, AuthMeResponse& a) {
    if (j.contains("id")) j.at("id").get_to(a.id);
    if (j.contains("email")) j.at("email").get_to(a.email);
    if (j.contains("fullName")) j.at("fullName").get_to(a.fullName);
    if (j.contains("avatarUrl")) j.at("avatarUrl").get_to(a.avatarUrl);
    if (j.contains("role")) j.at("role").get_to(a.role);
}

inline void to_json(nlohmann::json& j, const InterviewMessage& m) {
    j = nlohmann::json{
        {"role", m.role},
        {"content", m.content},
        {"timestamp", m.timestamp}
    };
}

inline void from_json(const nlohmann::json& j, InterviewMessage& m) {
    if (j.contains("role")) j.at("role").get_to(m.role);
    if (j.contains("content")) j.at("content").get_to(m.content);
    if (j.contains("timestamp")) j.at("timestamp").get_to(m.timestamp);
}

inline void to_json(nlohmann::json& j, const QuestionScoreItem& q) {
    j = nlohmann::json{
        {"question", q.question},
        {"score", q.score},
        {"comment", q.comment}
    };
}

inline void from_json(const nlohmann::json& j, QuestionScoreItem& q) {
    if (j.contains("question")) j.at("question").get_to(q.question);
    if (j.contains("score")) j.at("score").get_to(q.score);
    if (j.contains("comment")) j.at("comment").get_to(q.comment);
}

inline void to_json(nlohmann::json& j, const RecommendationItem& r) {
    j = nlohmann::json{
        {"topic", r.topic},
        {"resource", r.resource}
    };
}

inline void from_json(const nlohmann::json& j, RecommendationItem& r) {
    if (j.contains("topic")) j.at("topic").get_to(r.topic);
    if (j.contains("resource")) j.at("resource").get_to(r.resource);
}

inline void to_json(nlohmann::json& j, const InterviewFeedbackReport& r) {
    j = nlohmann::json{
        {"summary", r.summary},
        {"score", r.score},
        {"strengths", r.strengths},
        {"weaknesses", r.weaknesses},
        {"recommendations", r.recommendations},
        {"questionScores", r.questionScores}
    };
}

inline void from_json(const nlohmann::json& j, InterviewFeedbackReport& r) {
    if (j.contains("summary")) j.at("summary").get_to(r.summary);
    if (j.contains("score")) j.at("score").get_to(r.score);
    if (j.contains("strengths")) j.at("strengths").get_to(r.strengths);
    if (j.contains("weaknesses")) j.at("weaknesses").get_to(r.weaknesses);
    if (j.contains("recommendations")) j.at("recommendations").get_to(r.recommendations);

    // Support both camelCase ("questionScores") and ASP.NET Core snake_case ("question_scores")
    if (j.contains("questionScores")) {
        j.at("questionScores").get_to(r.questionScores);
    } else if (j.contains("question_scores")) {
        j.at("question_scores").get_to(r.questionScores);
    }
}

inline void to_json(nlohmann::json& j, const InterviewSummary& s) {
    j = nlohmann::json{
        {"id", s.id},
        {"userId", s.userId},
        {"direction", s.direction},
        {"level", s.level},
        {"language", s.language},
        {"stage", s.stage},
        {"status", s.status},
        {"score", s.score},
        {"questionCount", s.questionCount},
        {"createdAt", s.createdAt},
        {"updatedAt", s.updatedAt}
    };
}

inline void from_json(const nlohmann::json& j, InterviewSummary& s) {
    if (j.contains("id")) j.at("id").get_to(s.id);
    if (j.contains("userId")) j.at("userId").get_to(s.userId);
    if (j.contains("direction")) j.at("direction").get_to(s.direction);
    if (j.contains("level")) j.at("level").get_to(s.level);
    if (j.contains("language")) j.at("language").get_to(s.language);
    if (j.contains("stage")) j.at("stage").get_to(s.stage);
    if (j.contains("status")) j.at("status").get_to(s.status);
    if (j.contains("score")) j.at("score").get_to(s.score);
    if (j.contains("questionCount")) j.at("questionCount").get_to(s.questionCount);
    if (j.contains("createdAt")) j.at("createdAt").get_to(s.createdAt);
    if (j.contains("updatedAt")) j.at("updatedAt").get_to(s.updatedAt);
}

inline void to_json(nlohmann::json& j, const InterviewDetail& d) {
    to_json(j, static_cast<const InterviewSummary&>(d));
    j["transcript"] = d.transcript;
    j["feedback"] = d.feedback;
}

inline void from_json(const nlohmann::json& j, InterviewDetail& d) {
    from_json(j, static_cast<InterviewSummary&>(d));
    if (j.contains("transcript")) j.at("transcript").get_to(d.transcript);
    if (j.contains("feedback")) j.at("feedback").get_to(d.feedback);
}

inline void to_json(nlohmann::json& j, const CreateInterviewRequest& r) {
    j = nlohmann::json{
        {"direction", r.direction},
        {"level", r.level},
        {"language", r.language}
    };
}

inline void from_json(const nlohmann::json& j, CreateInterviewRequest& r) {
    if (j.contains("direction")) j.at("direction").get_to(r.direction);
    if (j.contains("level")) j.at("level").get_to(r.level);
    if (j.contains("language")) j.at("language").get_to(r.language);
}

inline void to_json(nlohmann::json& j, const CreateInterviewResponse& r) {
    j = nlohmann::json{
        {"interviewId", r.interviewId},
        {"firstQuestion", r.firstQuestion}
    };
}

inline void from_json(const nlohmann::json& j, CreateInterviewResponse& r) {
    if (j.contains("interviewId")) j.at("interviewId").get_to(r.interviewId);
    if (j.contains("firstQuestion")) j.at("firstQuestion").get_to(r.firstQuestion);
}

inline void to_json(nlohmann::json& j, const SendMessageRequest& r) {
    j = nlohmann::json{{"content", r.content}};
}

inline void from_json(const nlohmann::json& j, SendMessageRequest& r) {
    if (j.contains("content")) j.at("content").get_to(r.content);
}

inline void to_json(nlohmann::json& j, const SendMessageResponse& r) {
    j = nlohmann::json{
        {"message", r.message},
        {"questionCount", r.questionCount},
        {"isCompleted", r.isCompleted}
    };
}

inline void from_json(const nlohmann::json& j, SendMessageResponse& r) {
    if (j.contains("message")) j.at("message").get_to(r.message);
    if (j.contains("questionCount")) j.at("questionCount").get_to(r.questionCount);
    if (j.contains("isCompleted")) j.at("isCompleted").get_to(r.isCompleted);
}

inline void to_json(nlohmann::json& j, const CompleteInterviewResponse& r) {
    j = nlohmann::json{
        {"score", r.score},
        {"feedback", r.feedback}
    };
}

inline void from_json(const nlohmann::json& j, CompleteInterviewResponse& r) {
    if (j.contains("score")) j.at("score").get_to(r.score);
    if (j.contains("feedback")) j.at("feedback").get_to(r.feedback);
}

inline void to_json(nlohmann::json& j, const QuestionBankItem& q) {
    j = nlohmann::json{
        {"id", q.id},
        {"direction", q.direction},
        {"stage", q.stage},
        {"level", q.level},
        {"topic", q.topic},
        {"questionText", q.questionText},
        {"isActive", q.isActive},
        {"createdBy", q.createdBy},
        {"createdAt", q.createdAt}
    };
}

inline void from_json(const nlohmann::json& j, QuestionBankItem& q) {
    if (j.contains("id")) j.at("id").get_to(q.id);
    if (j.contains("direction")) j.at("direction").get_to(q.direction);
    if (j.contains("stage")) j.at("stage").get_to(q.stage);
    if (j.contains("level")) j.at("level").get_to(q.level);
    if (j.contains("topic")) j.at("topic").get_to(q.topic);
    if (j.contains("questionText")) j.at("questionText").get_to(q.questionText);
    if (j.contains("isActive")) j.at("isActive").get_to(q.isActive);
    if (j.contains("createdBy")) j.at("createdBy").get_to(q.createdBy);
    if (j.contains("createdAt")) j.at("createdAt").get_to(q.createdAt);
}

inline void to_json(nlohmann::json& j, const CreateQuestionRequest& r) {
    j = nlohmann::json{
        {"direction", r.direction},
        {"stage", r.stage},
        {"level", r.level},
        {"topic", r.topic},
        {"questionText", r.questionText}
    };
}

inline void from_json(const nlohmann::json& j, CreateQuestionRequest& r) {
    if (j.contains("direction")) j.at("direction").get_to(r.direction);
    if (j.contains("stage")) j.at("stage").get_to(r.stage);
    if (j.contains("level")) j.at("level").get_to(r.level);
    if (j.contains("topic")) j.at("topic").get_to(r.topic);
    if (j.contains("questionText")) j.at("questionText").get_to(r.questionText);
}

inline void to_json(nlohmann::json& j, const UpdateQuestionRequest& r) {
    j = nlohmann::json::object();
    if (r.direction.has_value()) j["direction"] = r.direction.value();
    if (r.stage.has_value()) j["stage"] = r.stage.value();
    if (r.level.has_value()) j["level"] = r.level.value();
    if (r.topic.has_value()) j["topic"] = r.topic.value();
    if (r.questionText.has_value()) j["questionText"] = r.questionText.value();
    if (r.isActive.has_value()) j["isActive"] = r.isActive.value();
}

inline void from_json(const nlohmann::json& j, UpdateQuestionRequest& r) {
    if (j.contains("direction")) j.at("direction").get_to(r.direction);
    if (j.contains("stage")) j.at("stage").get_to(r.stage);
    if (j.contains("level")) j.at("level").get_to(r.level);
    if (j.contains("topic")) j.at("topic").get_to(r.topic);
    if (j.contains("questionText")) j.at("questionText").get_to(r.questionText);
    if (j.contains("isActive")) j.at("isActive").get_to(r.isActive);
}

inline void to_json(nlohmann::json& j, const UserDashboardStats& s) {
    j = nlohmann::json{
        {"totalInterviews", s.totalInterviews},
        {"averageScore", s.averageScore},
        {"dailyInterviewsCount", s.dailyInterviewsCount},
        {"dailyLimit", s.dailyLimit},
        {"remainingToday", s.remainingToday}
    };
}

inline void from_json(const nlohmann::json& j, UserDashboardStats& s) {
    if (j.contains("totalInterviews")) j.at("totalInterviews").get_to(s.totalInterviews);
    if (j.contains("averageScore")) j.at("averageScore").get_to(s.averageScore);
    if (j.contains("dailyInterviewsCount")) j.at("dailyInterviewsCount").get_to(s.dailyInterviewsCount);
    if (j.contains("dailyLimit")) j.at("dailyLimit").get_to(s.dailyLimit);
    if (j.contains("remainingToday")) j.at("remainingToday").get_to(s.remainingToday);
}

template<typename T>
void to_json(nlohmann::json& j, const PagedResult<T>& p) {
    j = nlohmann::json{
        {"items", p.items},
        {"page", p.page},
        {"pageSize", p.pageSize},
        {"totalCount", p.totalCount},
        {"totalPages", p.totalPages}
    };
}

template<typename T>
void from_json(const nlohmann::json& j, PagedResult<T>& p) {
    if (j.contains("items")) j.at("items").get_to(p.items);
    if (j.contains("page")) j.at("page").get_to(p.page);
    if (j.contains("pageSize")) j.at("pageSize").get_to(p.pageSize);
    if (j.contains("totalCount")) j.at("totalCount").get_to(p.totalCount);
    if (j.contains("totalPages")) j.at("totalPages").get_to(p.totalPages);
}

inline void to_json(nlohmann::json& j, const SupportedLanguageOption& l) {
    j = nlohmann::json{
        {"id", l.id},
        {"code", l.code},
        {"name", l.name}
    };
}

inline void from_json(const nlohmann::json& j, SupportedLanguageOption& l) {
    if (j.contains("id")) j.at("id").get_to(l.id);
    if (j.contains("code")) j.at("code").get_to(l.code);
    if (j.contains("name")) j.at("name").get_to(l.name);
}

inline void to_json(nlohmann::json& j, const OptionsResponse& o) {
    j = nlohmann::json{
        {"directions", o.directions},
        {"levels", o.levels},
        {"stages", o.stages},
        {"languages", o.languages}
    };
}

inline void from_json(const nlohmann::json& j, OptionsResponse& o) {
    if (j.contains("directions")) j.at("directions").get_to(o.directions);
    if (j.contains("levels")) j.at("levels").get_to(o.levels);
    if (j.contains("stages")) j.at("stages").get_to(o.stages);
    if (j.contains("languages")) j.at("languages").get_to(o.languages);
}

} // namespace devdialogue::models
