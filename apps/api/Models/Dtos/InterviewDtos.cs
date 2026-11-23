using System.Text.Json.Serialization;
using System.ComponentModel.DataAnnotations;
namespace DevDialogue.Api.Models.Dtos;

public record CreateInterviewDto(
    [Required(ErrorMessage = "Напрямок (Direction) є обов'язковим")]
    string Direction,

    [Required(ErrorMessage = "Рівень (Level) є обов'язковим")]
    string Level,

    [Required(ErrorMessage = "Мова (Language) є обов'язковою")]
    string Language
);

public record AvatarUiCommandDto(
    [property: JsonPropertyName("action")] string Action,
    [property: JsonPropertyName("target")] string Target
);

public record CreateInterviewResponseDto(
    Guid InterviewId,
    string FirstQuestion,
    [property: JsonPropertyName("emotion")] string Emotion = "smile",
    [property: JsonPropertyName("actionsSequence")] List<string>? ActionsSequence = null,
    [property: JsonPropertyName("uiCommands")] List<AvatarUiCommandDto>? UiCommands = null
);

public record VisualContextDto(
    [property: JsonPropertyName("camera_active")] bool CameraActive = false,
    [property: JsonPropertyName("include_in_score")] bool IncludeInScore = true,
    [property: JsonPropertyName("person_present")] bool PersonPresent = false,
    [property: JsonPropertyName("visual_score")] int VisualScore = 0,
    [property: JsonPropertyName("presence_score")] int PresenceScore = 0,
    [property: JsonPropertyName("hands_score")] int HandsScore = 0,
    [property: JsonPropertyName("lighting_score")] int LightingScore = 0,
    [property: JsonPropertyName("appearance_score")] int AppearanceScore = 0,
    [property: JsonPropertyName("posture_score")] int PostureScore = 0,
    [property: JsonPropertyName("emotion_score")] int EmotionScore = 0,
    [property: JsonPropertyName("emotion")] string Emotion = "neutral",
    [property: JsonPropertyName("summary")] string Summary = ""
);

public record SendMessageDto(
    [Required(ErrorMessage = "Повідомлення не може бути порожнім")]
    [MaxLength(3000, ErrorMessage = "Відповідь перевищує ліміт у 3000 символів")]
    string Content,

    [property: JsonPropertyName("action")] string? Action = null,
    [property: JsonPropertyName("visual_context")] VisualContextDto? VisualContext = null
);

public record CriterionScoreDto(
    [property: JsonPropertyName("name")] string Name,
    [property: JsonPropertyName("score")] int Score,
    [property: JsonPropertyName("weight")] double Weight
);

public record SendMessageResponseDto(
    string Message,
    int QuestionCount,
    bool IsCompleted = false,
    [property: JsonPropertyName("awaitingNextAction")] bool AwaitingNextAction = false,
    [property: JsonPropertyName("lastScore")] int? LastScore = null,
    [property: JsonPropertyName("criteriaBreakdown")] List<CriterionScoreDto>? CriteriaBreakdown = null,
    [property: JsonPropertyName("suggestedActions")] List<string>? SuggestedActions = null,
    [property: JsonPropertyName("emotion")] string Emotion = "neutral",
    [property: JsonPropertyName("actionsSequence")] List<string>? ActionsSequence = null,
    [property: JsonPropertyName("uiCommands")] List<AvatarUiCommandDto>? UiCommands = null
);

public record InterviewMessageDto(
    [property: JsonPropertyName("role")] string Role,
    [property: JsonPropertyName("content")] string Content,
    [property: JsonPropertyName("timestamp")] DateTime Timestamp,
    [property: JsonPropertyName("criteria")] Dictionary<string, int>? Criteria = null
);

public record QuestionScoreDto(
    [property: JsonPropertyName("question")] string Question,
    [property: JsonPropertyName("score")] int Score,
    [property: JsonPropertyName("comment")] string Comment,
    [property: JsonPropertyName("response_seconds")] int ResponseSeconds = 0,
    [property: JsonPropertyName("user_answer")] string UserAnswer = "",
    [property: JsonPropertyName("criteria")] Dictionary<string, int>? Criteria = null
);

public record RecommendationDto(
    [property: JsonPropertyName("topic")] string Topic,
    [property: JsonPropertyName("resource")] string Resource
);

public record VisualCriteriaDto(
    [property: JsonPropertyName("overall_score")] int OverallScore,
    [property: JsonPropertyName("presence_score")] int PresenceScore,
    [property: JsonPropertyName("hands_score")] int HandsScore,
    [property: JsonPropertyName("lighting_score")] int LightingScore,
    [property: JsonPropertyName("appearance_score")] int AppearanceScore,
    [property: JsonPropertyName("posture_score")] int PostureScore,
    [property: JsonPropertyName("emotion_score")] int EmotionScore = 75,
    [property: JsonPropertyName("dominant_emotion")] string DominantEmotion = "neutral",
    [property: JsonPropertyName("included_in_score")] bool IncludedInScore = true,
    [property: JsonPropertyName("summary")] string Summary = "",
    [property: JsonPropertyName("model")] string Model = "gemma4:e4b + 30 FPS CV",
    [property: JsonPropertyName("samples_analyzed")] int SamplesAnalyzed = 0
);

public record InterviewFeedbackDto(
    [property: JsonPropertyName("summary")] string Summary,
    [property: JsonPropertyName("score")] int Score,
    [property: JsonPropertyName("strengths")] List<string> Strengths,
    [property: JsonPropertyName("weaknesses")] List<string> Weaknesses,
    [property: JsonPropertyName("recommendations")] List<RecommendationDto> Recommendations,
    [property: JsonPropertyName("question_scores")] List<QuestionScoreDto> QuestionScores,
    [property: JsonPropertyName("average_response_seconds")] int AverageResponseSeconds = 0,
    [property: JsonPropertyName("total_duration_seconds")] int TotalDurationSeconds = 0,
    [property: JsonPropertyName("timing_analysis")] string TimingAnalysis = "",
    [property: JsonPropertyName("visual_assessment")] VisualCriteriaDto? VisualAssessment = null
);

public record CompleteInterviewRequestDto(
    [property: JsonPropertyName("visual_assessment")] VisualCriteriaDto? VisualAssessment = null
);

public record CompleteInterviewResponseDto(
    int Score,
    InterviewFeedbackDto Feedback
);

public record InterviewSummaryDto(
    Guid Id,
    Guid UserId,
    string Direction,
    string Level,
    string Language,
    string Stage,
    string Status,
    int? Score,
    int QuestionCount,
    DateTime CreatedAt,
    DateTime UpdatedAt
);

public record InterviewDetailDto(
    Guid Id,
    Guid UserId,
    string Direction,
    string Level,
    string Language,
    string Stage,
    string Status,
    int? Score,
    int QuestionCount,
    List<InterviewMessageDto> Transcript,
    InterviewFeedbackDto? Feedback,
    DateTime CreatedAt,
    DateTime UpdatedAt
);

public record DashboardStatsDto(
    int TotalInterviews,
    double AverageScore,
    int DailyInterviewsCount,
    int DailyLimit,
    int RemainingToday
);
