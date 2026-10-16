using System.Text.Json.Serialization;

namespace DevDialogue.Api.Models.Dtos;

public record CreateInterviewDto(
    string Direction,
    string Level,
    string Language
);

public record CreateInterviewResponseDto(
    Guid InterviewId,
    string FirstQuestion
);

public record SendMessageDto(
    string Content
);

public record SendMessageResponseDto(
    string Message,
    int QuestionCount,
    bool IsCompleted = false
);

public record InterviewMessageDto(
    [property: JsonPropertyName("role")] string Role,
    [property: JsonPropertyName("content")] string Content,
    [property: JsonPropertyName("timestamp")] DateTime Timestamp
);

public record QuestionScoreDto(
    [property: JsonPropertyName("question")] string Question,
    [property: JsonPropertyName("score")] int Score,
    [property: JsonPropertyName("comment")] string Comment
);

public record RecommendationDto(
    [property: JsonPropertyName("topic")] string Topic,
    [property: JsonPropertyName("resource")] string Resource
);

public record InterviewFeedbackDto(
    [property: JsonPropertyName("summary")] string Summary,
    [property: JsonPropertyName("score")] int Score,
    [property: JsonPropertyName("strengths")] List<string> Strengths,
    [property: JsonPropertyName("weaknesses")] List<string> Weaknesses,
    [property: JsonPropertyName("recommendations")] List<RecommendationDto> Recommendations,
    [property: JsonPropertyName("question_scores")] List<QuestionScoreDto> QuestionScores
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
