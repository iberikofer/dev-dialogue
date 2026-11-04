# Document 02: Data Models, Serialization & Networking

This document details the transition from TypeScript interfaces in `packages/api-contracts` and `apps/web/src/lib/api-client.ts` to type-safe C++20 structs, `nlohmann::json` serialization, and the asynchronous `QNetworkAccessManager` networking architecture.

---

## 1. C++20 Data Contracts (`include/models/ApiContracts.hpp`)

All types from `packages/api-contracts/src/index.ts` are mapped directly to idiomatic C++20 structs with string-enum converters and non-intrusive JSON serialization:

```cpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace devdialogue::models {

// ==========================================
// Strongly-Typed Enums
// ==========================================

enum class UserRole {
    User,
    Admin
};

enum class InterviewDirection {
    Frontend,
    Backend,
    DevOps,
    Pm,
    Qa
};

enum class InterviewLevel {
    Trainee,
    Junior,
    Middle
};

enum class InterviewLanguage {
    Uk,
    En
};

enum class InterviewStage {
    HardSkills,
    SoftSkills,
    English
};

enum class InterviewStatus {
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
    InterviewDirection direction;
    InterviewStage stage;
    InterviewLevel level;
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

template<typename T>
struct PagedResult {
    std::vector<T> items;
    int page{1};
    int pageSize{20};
    int totalCount{0};
    std::optional<int> totalPages;
};

// ==========================================
// JSON Enum Conversion Helpers
// ==========================================

NLOHMANN_JSON_SERIALIZE_ENUM(UserRole, {
    {UserRole::User, "user"},
    {UserRole::Admin, "admin"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewDirection, {
    {InterviewDirection::Frontend, "frontend"},
    {InterviewDirection::Backend, "backend"},
    {InterviewDirection::DevOps, "devops"},
    {InterviewDirection::Pm, "pm"},
    {InterviewDirection::Qa, "qa"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewLevel, {
    {InterviewLevel::Trainee, "trainee"},
    {InterviewLevel::Junior, "junior"},
    {InterviewLevel::Middle, "middle"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewLanguage, {
    {InterviewLanguage::Uk, "uk"},
    {InterviewLanguage::En, "en"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewStage, {
    {InterviewStage::HardSkills, "hard_skills"},
    {InterviewStage::SoftSkills, "soft_skills"},
    {InterviewStage::English, "english"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(InterviewStatus, {
    {InterviewStatus::Active, "active"},
    {InterviewStatus::Completed, "completed"},
    {InterviewStatus::Abandoned, "abandoned"}
})

// ==========================================
// Macro Definitions for Non-Intrusive JSON
// ==========================================

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InterviewMessage, role, content, timestamp)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QuestionScoreItem, question, score, comment)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RecommendationItem, topic, resource)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InterviewFeedbackReport, summary, score, strengths, weaknesses, recommendations, questionScores)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthMeResponse, id, email, fullName, avatarUrl, role)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateInterviewRequest, direction, level, language)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateInterviewResponse, interviewId, firstQuestion)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageRequest, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SendMessageResponse, message, questionCount, isCompleted)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompleteInterviewResponse, score, feedback)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDashboardStats, totalInterviews, averageScore, dailyInterviewsCount, dailyLimit, remainingToday)

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
    j.at("id").get_to(s.id);
    j.at("userId").get_to(s.userId);
    j.at("direction").get_to(s.direction);
    j.at("level").get_to(s.level);
    j.at("language").get_to(s.language);
    j.at("stage").get_to(s.stage);
    j.at("status").get_to(s.status);
    if (j.contains("score") && !j["score"].is_null()) {
        s.score = j["score"].get<int>();
    }
    j.at("questionCount").get_to(s.questionCount);
    j.at("createdAt").get_to(s.createdAt);
    j.at("updatedAt").get_to(s.updatedAt);
}

} // namespace devdialogue::models
```

---

## 2. Asynchronous API Client (`ApiClient.hpp` & `ApiClient.cpp`)

Replacing `apps/web/src/lib/api-client.ts`, the C++ `ApiClient` uses `QNetworkAccessManager` with SSL/TLS verification, Bearer token injection, automatic heartbeat, and error categorization.

### Header: `include/network/ApiClient.hpp`

```cpp
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "core/AppConfig.hpp"
#include "services/SecureStorage.hpp"

namespace devdialogue::network {

struct ApiError {
    int statusCode{0};
    std::string message;
    std::string rawPayload;
    bool isNetworkDown{false};
};

template<typename T>
using ApiCallback = std::function<void(const std::variant<T, ApiError>&)>;

class ApiClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineStatusChanged)
    Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY errorOccurred)

public:
    explicit ApiClient(
        std::shared_ptr<core::AppConfig> config,
        std::shared_ptr<services::SecureStorage> storage,
        QObject* parent = nullptr
    );

    bool isOnline() const { return m_isOnline; }
    QString lastErrorMessage() const { return m_lastError; }

    // HTTP Verbs with generic typing
    template<typename TReq, typename TResp>
    void post(const QString& endpoint, const TReq& body, ApiCallback<TResp> callback);

    template<typename TResp>
    void get(const QString& endpoint, ApiCallback<TResp> callback);

    template<typename TReq, typename TResp>
    void put(const QString& endpoint, const TReq& body, ApiCallback<TResp> callback);

    void del(const QString& endpoint, std::function<void(bool success, const ApiError&)> callback);

    void setBearerToken(const std::string& token);
    void clearBearerToken();
    void performHealthCheck();

signals:
    void onlineStatusChanged(bool isOnline);
    void errorOccurred(const QString& message);
    void unauthorizedDetected(); // Triggers 401 logout

private:
    std::shared_ptr<core::AppConfig> m_config;
    std::shared_ptr<services::SecureStorage> m_storage;
    QNetworkAccessManager m_networkManager;
    std::string m_cachedToken;
    bool m_isOnline{true};
    QString m_lastError;

    QNetworkRequest createRequest(const QString& endpoint);
    void handleNetworkReply(QNetworkReply* reply, std::function<void(const QByteArray&, const ApiError*)> handler);
};

} // namespace devdialogue::network
```

### Implementation: `src/network/ApiClient.cpp`

```cpp
#include "network/ApiClient.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QTimer>

namespace devdialogue::network {

ApiClient::ApiClient(
    std::shared_ptr<core::AppConfig> config,
    std::shared_ptr<services::SecureStorage> storage,
    QObject* parent
) : QObject(parent), m_config(std::move(config)), m_storage(std::move(storage)) {
    // Read persisted token on boot
    m_cachedToken = m_storage->readCredential("jwt_token").value_or("");

    // Set up periodic health check (every 30 seconds)
    auto* healthTimer = new QTimer(this);
    connect(healthTimer, &QTimer::timeout, this, &ApiClient::performHealthCheck);
    healthTimer->start(30000);
}

QNetworkRequest ApiClient::createRequest(const QString& endpoint) {
    const QString fullUrl = QString::fromStdString(m_config->apiBaseUrl()) + endpoint;
    QNetworkRequest req{QUrl(fullUrl)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_cachedToken.empty()) {
        const QString bearer = QString("Bearer %1").arg(QString::fromStdString(m_cachedToken));
        req.setRawHeader("Authorization", bearer.toUtf8());
    }

    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    return req;
}

void ApiClient::performHealthCheck() {
    QNetworkRequest req = createRequest("/api/health");
    QNetworkReply* reply = m_networkManager.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        bool newStatus = (reply->error() == QNetworkReply::NoError);
        if (newStatus != m_isOnline) {
            m_isOnline = newStatus;
            emit onlineStatusChanged(m_isOnline);
        }
    });
}

void ApiClient::setBearerToken(const std::string& token) {
    m_cachedToken = token;
    m_storage->writeCredential("jwt_token", token);
}

void ApiClient::clearBearerToken() {
    m_cachedToken.clear();
    m_storage->deleteCredential("jwt_token");
}

} // namespace devdialogue::network
```

