#include <gtest/gtest.h>
#include "models/ApiContracts.hpp"

using namespace devdialogue::models;

// ==========================================
// Enum Unknown Fallback Tests
// ==========================================

TEST(ApiContractsTest, EnumUnknownFallback) {
    // Unrecognized values should deserialize to Unknown (0)
    nlohmann::json jDirection = "non_existent_direction";
    InterviewDirection dir = jDirection.get<InterviewDirection>();
    EXPECT_EQ(dir, InterviewDirection::Unknown);

    nlohmann::json jLevel = "staff_principal";
    InterviewLevel lvl = jLevel.get<InterviewLevel>();
    EXPECT_EQ(lvl, InterviewLevel::Unknown);

    nlohmann::json jLang = "klingon";
    InterviewLanguage lang = jLang.get<InterviewLanguage>();
    EXPECT_EQ(lang, InterviewLanguage::Unknown);

    nlohmann::json jRole = "superadmin";
    UserRole role = jRole.get<UserRole>();
    EXPECT_EQ(role, UserRole::Unknown);

    nlohmann::json jStage = "behavioral_round_4";
    InterviewStage stage = jStage.get<InterviewStage>();
    EXPECT_EQ(stage, InterviewStage::Unknown);

    nlohmann::json jStatus = "paused";
    InterviewStatus status = jStatus.get<InterviewStatus>();
    EXPECT_EQ(status, InterviewStatus::Unknown);
}

TEST(ApiContractsTest, EnumValidValuesRoundtrip) {
    nlohmann::json jDir = InterviewDirection::Frontend;
    EXPECT_EQ(jDir.get<std::string>(), "frontend");
    EXPECT_EQ(jDir.get<InterviewDirection>(), InterviewDirection::Frontend);

    nlohmann::json jLvl = InterviewLevel::Senior;
    EXPECT_EQ(jLvl.get<std::string>(), "senior");
    EXPECT_EQ(jLvl.get<InterviewLevel>(), InterviewLevel::Senior);

    nlohmann::json jLang = InterviewLanguage::Uk;
    EXPECT_EQ(jLang.get<std::string>(), "uk");
    EXPECT_EQ(jLang.get<InterviewLanguage>(), InterviewLanguage::Uk);
}

// ==========================================
// DTO Roundtrip Tests
// ==========================================

TEST(ApiContractsTest, UserProfileRoundtrip) {
    UserProfile original;
    original.id = "usr-123";
    original.email = "test@example.com";
    original.fullName = "Test User";
    original.avatarUrl = "https://example.com/avatar.png";
    original.role = UserRole::User;
    original.dailyInterviewsCount = 3;
    original.lastInterviewDate = "2026-11-06T10:00:00Z";
    original.createdAt = "2026-01-01T00:00:00Z";

    nlohmann::json j = original;
    UserProfile restored = j.get<UserProfile>();

    EXPECT_EQ(restored.id, original.id);
    EXPECT_EQ(restored.email, original.email);
    ASSERT_TRUE(restored.fullName.has_value());
    EXPECT_EQ(*restored.fullName, "Test User");
    ASSERT_TRUE(restored.avatarUrl.has_value());
    EXPECT_EQ(*restored.avatarUrl, "https://example.com/avatar.png");
    EXPECT_EQ(restored.role, UserRole::User);
    EXPECT_EQ(restored.dailyInterviewsCount, 3);
}

TEST(ApiContractsTest, AuthMeResponseRoundtrip) {
    nlohmann::json j = {
        {"id", "me-456"},
        {"email", "admin@devdialogue.local"},
        {"fullName", "Admin Guy"},
        {"avatarUrl", nullptr},
        {"role", "admin"}
    };

    AuthMeResponse me = j.get<AuthMeResponse>();
    EXPECT_EQ(me.id, "me-456");
    EXPECT_EQ(me.email, "admin@devdialogue.local");
    ASSERT_TRUE(me.fullName.has_value());
    EXPECT_EQ(*me.fullName, "Admin Guy");
    EXPECT_FALSE(me.avatarUrl.has_value());
    EXPECT_EQ(me.role, UserRole::Admin);
}

TEST(ApiContractsTest, InterviewSummaryRoundtrip) {
    InterviewSummary summary;
    summary.id = "int-789";
    summary.userId = "usr-123";
    summary.direction = InterviewDirection::Backend;
    summary.level = InterviewLevel::Middle;
    summary.language = InterviewLanguage::En;
    summary.stage = InterviewStage::HardSkills;
    summary.status = InterviewStatus::Active;
    summary.score = 85;
    summary.questionCount = 5;
    summary.createdAt = "2026-11-06T11:00:00Z";
    summary.updatedAt = "2026-11-06T11:30:00Z";

    nlohmann::json j = summary;
    InterviewSummary restored = j.get<InterviewSummary>();

    EXPECT_EQ(restored.id, summary.id);
    EXPECT_EQ(restored.direction, InterviewDirection::Backend);
    EXPECT_EQ(restored.level, InterviewLevel::Middle);
    EXPECT_EQ(restored.language, InterviewLanguage::En);
    ASSERT_TRUE(restored.score.has_value());
    EXPECT_EQ(*restored.score, 85);
    EXPECT_EQ(restored.questionCount, 5);
}

TEST(ApiContractsTest, InterviewFeedbackReportDualFieldNaming) {
    // 1. camelCase: "questionScores"
    nlohmann::json jCamel = {
        {"summary", "Great performance!"},
        {"score", 90},
        {"strengths", {"C++ knowledge", "Algorithms"}},
        {"weaknesses", {"Docker optimization"}},
        {"recommendations", {{{"topic", "Docker"}, {"resource", "docker docs"}}}},
        {"questionScores", {{{"question", "What is RAII?"}, {"score", 95}, {"comment", "Flawless"}}}}
    };

    InterviewFeedbackReport reportCamel = jCamel.get<InterviewFeedbackReport>();
    EXPECT_EQ(reportCamel.score, 90);
    ASSERT_EQ(reportCamel.questionScores.size(), 1u);
    EXPECT_EQ(reportCamel.questionScores[0].question, "What is RAII?");
    EXPECT_EQ(reportCamel.questionScores[0].score, 95);

    // 2. snake_case: "question_scores" (from apps/api InterviewFeedbackDto)
    nlohmann::json jSnake = {
        {"summary", "Solid interview"},
        {"score", 80},
        {"strengths", {"Networking"}},
        {"weaknesses", {}},
        {"recommendations", {}},
        {"question_scores", {{{"question", "Explain TCP handshake"}, {"score", 85}, {"comment", "Good"}}}}
    };

    InterviewFeedbackReport reportSnake = jSnake.get<InterviewFeedbackReport>();
    EXPECT_EQ(reportSnake.score, 80);
    ASSERT_EQ(reportSnake.questionScores.size(), 1u);
    EXPECT_EQ(reportSnake.questionScores[0].question, "Explain TCP handshake");
    EXPECT_EQ(reportSnake.questionScores[0].score, 85);
}

TEST(ApiContractsTest, OptionsResponseRoundtrip) {
    nlohmann::json j = {
        {"tracks", {
            {{"id", "frontend"}, {"title", "Frontend Development"}},
            {{"id", "backend"}, {"title", "Backend Development"}}
        }},
        {"levels", {
            {{"id", "junior"}, {"title", "Junior"}},
            {{"id", "middle"}, {"title", "Middle"}},
            {{"id", "senior"}, {"title", "Senior"}}
        }},
        {"languages", {
            {{"id", "uk"}, {"title", "Українська"}},
            {{"id", "en"}, {"title", "English"}}
        }}
    };

    OptionsResponse options = j.get<OptionsResponse>();
    ASSERT_EQ(options.tracks.size(), 2u);
    EXPECT_EQ(options.tracks[0].id, "frontend");
    ASSERT_EQ(options.levels.size(), 3u);
    EXPECT_EQ(options.levels[2].id, "senior");
    ASSERT_EQ(options.languages.size(), 2u);
    EXPECT_EQ(options.languages[0].title, "Українська");
}

TEST(ApiContractsTest, PagedResultRoundtrip) {
    nlohmann::json j = {
        {"items", {
            {
                {"id", "q-1"},
                {"direction", "backend"},
                {"stage", "hard_skills"},
                {"level", "junior"},
                {"topic", "C#"},
                {"questionText", "What is garbage collection?"},
                {"isActive", true},
                {"createdBy", nullptr},
                {"createdAt", "2026-10-01T00:00:00Z"}
            }
        }},
        {"page", 1},
        {"pageSize", 10},
        {"totalCount", 42},
        {"totalPages", 5}
    };

    PagedResult<QuestionBankItem> paged = j.get<PagedResult<QuestionBankItem>>();
    EXPECT_EQ(paged.page, 1);
    EXPECT_EQ(paged.pageSize, 10);
    EXPECT_EQ(paged.totalCount, 42);
    ASSERT_TRUE(paged.totalPages.has_value());
    EXPECT_EQ(*paged.totalPages, 5);
    ASSERT_EQ(paged.items.size(), 1u);
    EXPECT_EQ(paged.items[0].topic, "C#");
    EXPECT_EQ(paged.items[0].level, InterviewLevel::Junior);
}

// ==========================================
// ApiError Parsing Tests
// ==========================================

TEST(ApiContractsTest, ParseErrorAndCodeFormat) {
    nlohmann::json j = {
        {"error", "Invalid direction provided"},
        {"code", "INVALID_ARGUMENT"}
    };

    ApiError err = ApiError::fromJson(j, 400);
    EXPECT_EQ(err.statusCode, 400);
    EXPECT_EQ(err.message, "Invalid direction provided");
    EXPECT_EQ(err.code, "INVALID_ARGUMENT");
}

TEST(ApiContractsTest, ParseProblemDetailsFormat) {
    nlohmann::json j = {
        {"type", "https://tools.ietf.org/html/rfc7231#section-6.5.1"},
        {"title", "One or more validation errors occurred."},
        {"status", 400},
        {"errors", {
            {"Direction", {"The Direction field is required."}}
        }}
    };

    ApiError err = ApiError::fromJson(j, 400);
    EXPECT_EQ(err.statusCode, 400);
    EXPECT_EQ(err.message, "One or more validation errors occurred.");
    EXPECT_TRUE(err.validationErrors.contains("Direction"));
}

TEST(ApiContractsTest, ParseSupabaseAuthErrorDescription) {
    nlohmann::json j = {
        {"error", "invalid_grant"},
        {"error_description", "Invalid login credentials"}
    };

    ApiError err = ApiError::fromJson(j, 400);
    EXPECT_EQ(err.statusCode, 400);
    EXPECT_EQ(err.message, "Invalid login credentials");
    EXPECT_EQ(err.code, "invalid_grant");
}

TEST(ApiContractsTest, ParseSupabaseAuthMsg) {
    nlohmann::json j = {
        {"msg", "User already registered"}
    };

    ApiError err = ApiError::fromJson(j, 422);
    EXPECT_EQ(err.statusCode, 422);
    EXPECT_EQ(err.message, "User already registered");
}

TEST(ApiContractsTest, ParseSupabaseAuthMessageAndErrorCode) {
    nlohmann::json j = {
        {"message", "Email rate limit exceeded"},
        {"error_code", "over_email_send_rate_limit"}
    };

    ApiError err = ApiError::fromJson(j, 429);
    EXPECT_EQ(err.statusCode, 429);
    EXPECT_EQ(err.message, "Email rate limit exceeded");
    EXPECT_EQ(err.code, "over_email_send_rate_limit");
}
