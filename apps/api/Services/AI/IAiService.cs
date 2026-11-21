using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;

namespace DevDialogue.Api.Services.AI;

public sealed record AiTurnOutcome(
    string Message,
    bool AwaitingNextAction = false,
    int? Score = null,
    List<CriterionScoreDto>? CriteriaBreakdown = null,
    List<string>? SuggestedActions = null,
    Dictionary<string, int>? StoredCriteria = null,
    bool AdvancesQuestion = true,
    string Emotion = "neutral",
    List<string>? ActionsSequence = null,
    List<AvatarUiCommandDto>? UiCommands = null
);

public interface IAiService
{
    Task<string> GenerateFirstQuestionAsync(CreateInterviewDto dto, string? topicHint, CancellationToken cancellationToken = default);
    Task<string> GenerateNextResponseAsync(Interview interview, List<InterviewMessageDto> transcript, CancellationToken cancellationToken = default);
    Task<InterviewFeedbackDto> GenerateFeedbackAsync(Interview interview, List<InterviewMessageDto> transcript, CancellationToken cancellationToken = default);

    async Task<AiTurnOutcome> HandleTurnActionAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        string? action,
        CancellationToken cancellationToken = default,
        VisualContextDto? visualContext = null)
    {
        var msg = await GenerateNextResponseAsync(interview, transcript, cancellationToken);
        return new AiTurnOutcome(msg);
    }
}
