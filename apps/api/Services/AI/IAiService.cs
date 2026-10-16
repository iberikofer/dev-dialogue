using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;

namespace DevDialogue.Api.Services.AI;

public interface IAiService
{
    Task<string> GenerateFirstQuestionAsync(CreateInterviewDto dto, string? topicHint, CancellationToken cancellationToken = default);
    Task<string> GenerateNextResponseAsync(Interview interview, List<InterviewMessageDto> transcript, CancellationToken cancellationToken = default);
    Task<InterviewFeedbackDto> GenerateFeedbackAsync(Interview interview, List<InterviewMessageDto> transcript, CancellationToken cancellationToken = default);
}
