namespace DevDialogue.Api.Models.Dtos;

public record AuthMeDto(
    Guid Id,
    string Email,
    string? FullName,
    string? AvatarUrl,
    string Role
);

public record UserProfileDto(
    Guid Id,
    string Email,
    string? FullName,
    string? AvatarUrl,
    string Role,
    int DailyInterviewsCount,
    DateOnly? LastInterviewDate,
    DateTime CreatedAt
);
