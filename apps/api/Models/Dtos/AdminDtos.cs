using System.ComponentModel.DataAnnotations;

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
    [Required(ErrorMessage = "Напрямок (Direction) є обов'язковим")]
    [MaxLength(50, ErrorMessage = "Напрямок не може перевищувати 50 символів")]
    string Direction,

    [Required(ErrorMessage = "Етап (Stage) є обов'язковим")]
    [MaxLength(50, ErrorMessage = "Етап не може перевищувати 50 символів")]
    string Stage,

    [Required(ErrorMessage = "Рівень (Level) є обов'язковим")]
    [MaxLength(50, ErrorMessage = "Рівень не може перевищувати 50 символів")]
    string Level,

    [Required(ErrorMessage = "Тема (Topic) є обов'язковою")]
    [MaxLength(200, ErrorMessage = "Тема не може перевищувати 200 символів")]
    string Topic,

    [Required(ErrorMessage = "Текст питання (QuestionText) є обов'язковим")]
    [MaxLength(3000, ErrorMessage = "Текст питання не може перевищувати 3000 символів")]
    string QuestionText
);

public record UpdateQuestionDto(
    [MaxLength(50, ErrorMessage = "Напрямок не може перевищувати 50 символів")]
    string? Direction,

    [MaxLength(50, ErrorMessage = "Етап не може перевищувати 50 символів")]
    string? Stage,

    [MaxLength(50, ErrorMessage = "Рівень не може перевищувати 50 символів")]
    string? Level,

    [MaxLength(200, ErrorMessage = "Тема не може перевищувати 200 символів")]
    string? Topic,

    [MaxLength(3000, ErrorMessage = "Текст питання не може перевищувати 3000 символів")]
    string? QuestionText,

    bool? IsActive
);
