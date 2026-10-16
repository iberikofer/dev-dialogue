namespace DevDialogue.Api.Models.Dtos;

public record QuestionBankDto(
    Guid Id,
    string Direction,
    string Stage,
    string Level,
    string Topic,
    string QuestionText,
    bool IsActive,
    Guid? CreatedBy,
    DateTime CreatedAt
);

public record CreateQuestionDto(
    string Direction,
    string Stage,
    string Level,
    string Topic,
    string QuestionText
);

public record UpdateQuestionDto(
    string? Direction,
    string? Stage,
    string? Level,
    string? Topic,
    string? QuestionText,
    bool? IsActive
);
