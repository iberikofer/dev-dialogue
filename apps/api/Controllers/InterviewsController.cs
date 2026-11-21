using System.Text.Json;
using DevDialogue.Api.Data;
using DevDialogue.Api.Helpers;
using DevDialogue.Api.Models;
using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;
using DevDialogue.Api.Security;
using DevDialogue.Api.Services.AI;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Controllers;

[ApiController]
[Route("api/[controller]")]
[Authorize]
public class InterviewsController : ControllerBase
{
    private readonly AppDbContext _context;
    private readonly IAiService _aiService;
    private readonly LocalModelManager _localModelManager;
    private readonly AvatarSpeechService _avatarSpeech;
    private readonly VisualSessionAggregator _visualAggregator;
    private readonly IConfiguration _config;
    private readonly ILogger<InterviewsController> _logger;

    public InterviewsController(
        AppDbContext context,
        IAiService aiService,
        LocalModelManager localModelManager,
        AvatarSpeechService avatarSpeech,
        VisualSessionAggregator visualAggregator,
        IConfiguration config,
        ILogger<InterviewsController> logger)
    {
        _context = context;
        _aiService = aiService;
        _localModelManager = localModelManager;
        _avatarSpeech = avatarSpeech;
        _visualAggregator = visualAggregator;
        _config = config;
        _logger = logger;
    }

    [HttpPost]
    public async Task<ActionResult<CreateInterviewResponseDto>> CreateInterview([FromBody] CreateInterviewDto dto)
    {
        // Валідація мови: якщо мову не підтримується/не знайдено -> 404 Not Found
        if (!SupportedOptions.Languages.Contains(dto.Language.ToLowerInvariant()))
        {
            return NotFound(new
            {
                error = $"Language '{dto.Language}' not found. Supported languages: {string.Join(", ", SupportedOptions.Languages)}",
                code = "LANGUAGE_NOT_FOUND"
            });
        }

        // Валідація напрямку
        if (!SupportedOptions.Directions.Contains(dto.Direction.ToLowerInvariant()))
        {
            return BadRequest(new
            {
                error = $"Invalid direction '{dto.Direction}'. Supported: {string.Join(", ", SupportedOptions.Directions)}",
                code = "INVALID_DIRECTION"
            });
        }

        // Валідація рівня
        if (!SupportedOptions.Levels.Contains(dto.Level.ToLowerInvariant()))
        {
            return BadRequest(new
            {
                error = $"Invalid level '{dto.Level}'. Supported: {string.Join(", ", SupportedOptions.Levels)}",
                code = "INVALID_LEVEL"
            });
        }

        if (!await _localModelManager.EnsureReadyForInterviewAsync())
        {
            return BadRequest(new
            {
                error = $"Local AI model '{_localModelManager.ModelName}' is not downloaded. Please download it before starting an interview.",
                code = "LOCAL_MODEL_REQUIRED",
                model = _localModelManager.ModelName
            });
        }

        await _avatarSpeech.Ensure3DServersRunningAsync(forceRestart: false);

        var userId = User.GetUserId();
        var today = DateOnly.FromDateTime(DateTime.UtcNow);

        var user = await _context.Users.FirstOrDefaultAsync(u => u.Id == userId);
        if (user == null)
        {
            user = new User
            {
                Id = userId,
                Email = User.GetUserEmail(),
                Role = "user",
                DailyInterviewsCount = 0,
                LastInterviewDate = today
            };
            _context.Users.Add(user);
        }

        // Reset daily counter if it is a new day
        if (user.LastInterviewDate != today)
        {
            user.DailyInterviewsCount = 0;
            user.LastInterviewDate = today;
        }

        // Lookup starting topic from QuestionBank if available (exact level first, then direction fallback)
        var directionKey = dto.Direction.ToLowerInvariant();
        var levelKey = dto.Level.ToLowerInvariant();
        var seedQuestion = await _context.QuestionsBank
            .AsNoTracking()
            .Where(q => q.Direction == directionKey && q.Level == levelKey && q.IsActive)
            .OrderBy(_ => EF.Functions.Random())
            .FirstOrDefaultAsync()
            ?? await _context.QuestionsBank
                .AsNoTracking()
                .Where(q => q.Direction == directionKey && q.IsActive)
                .OrderBy(_ => EF.Functions.Random())
                .FirstOrDefaultAsync();

        var firstQuestion = await _aiService.GenerateFirstQuestionAsync(dto, seedQuestion?.Topic);

        var initialTranscript = new List<InterviewMessageDto>
        {
            new("assistant", firstQuestion, DateTime.UtcNow)
        };

        var interview = new Interview
        {
            Id = Guid.NewGuid(),
            UserId = userId,
            Direction = dto.Direction.ToLowerInvariant(),
            Level = dto.Level.ToLowerInvariant(),
            Language = dto.Language.ToLowerInvariant(),
            Stage = "hard_skills",
            Status = "active",
            Transcript = JsonSerializer.Serialize(initialTranscript),
            QuestionCount = 1,
            CreatedAt = DateTime.UtcNow,
            UpdatedAt = DateTime.UtcNow
        };

        _context.Interviews.Add(interview);
        user.DailyInterviewsCount++;
        user.UpdatedAt = DateTime.UtcNow;

        await _context.SaveChangesAsync();

        return Ok(new CreateInterviewResponseDto(
            interview.Id,
            firstQuestion,
            Emotion: "smile",
            ActionsSequence: ["Smile"]
        ));
    }

    [HttpGet]
    public async Task<ActionResult<PagedResult<InterviewSummaryDto>>> GetInterviews([FromQuery] int page = 1, [FromQuery] int pageSize = 20)
    {
        var userId = User.GetUserId();

        var query = _context.Interviews
            .AsNoTracking()
            .Where(i => i.UserId == userId)
            .OrderByDescending(i => i.CreatedAt)
            .ThenByDescending(i => i.Id)
            .Select(i => new InterviewSummaryDto(
                i.Id, i.UserId, i.Direction, i.Level, i.Language, 
                i.Stage, i.Status, i.Score, i.QuestionCount, 
                i.CreatedAt, i.UpdatedAt
            ));

        var pagedResult = await query.ToPagedAsync(page, pageSize);

        return Ok(pagedResult);
    }

    [HttpGet("{id:guid}")]
    public async Task<ActionResult<InterviewDetailDto>> GetInterview(Guid id)
    {
        var userId = User.GetUserId();

        var interview = await _context.Interviews
            .AsNoTracking()
            .FirstOrDefaultAsync(i => i.Id == id && i.UserId == userId);

        if (interview == null) return NotFound();

        if (string.Equals(interview.Status, "active", StringComparison.OrdinalIgnoreCase))
        {
            _ = _localModelManager.EnsureReadyForInterviewAsync();
            _ = _avatarSpeech.Ensure3DServersRunningAsync(forceRestart: false);
        }

        var transcript = JsonSerializer.Deserialize<List<InterviewMessageDto>>(interview.Transcript)
                         ?? new List<InterviewMessageDto>();

        InterviewFeedbackDto? feedback = null;
        if (!string.IsNullOrWhiteSpace(interview.Feedback))
        {
            feedback = JsonSerializer.Deserialize<InterviewFeedbackDto>(interview.Feedback);
        }

        return Ok(new InterviewDetailDto(
            interview.Id,
            interview.UserId,
            interview.Direction,
            interview.Level,
            interview.Language,
            interview.Stage,
            interview.Status,
            interview.Score,
            interview.QuestionCount,
            transcript,
            feedback,
            interview.CreatedAt,
            interview.UpdatedAt
        ));
    }

    [HttpPost("{id:guid}/message")]
    public async Task<ActionResult<SendMessageResponseDto>> SendMessage(Guid id, [FromBody] SendMessageDto dto)
    {
        var userId = User.GetUserId();

        var interview = await _context.Interviews
            .FirstOrDefaultAsync(i => i.Id == id && i.UserId == userId);

        if (interview == null) return NotFound();

        if (string.Equals(interview.Status, "active", StringComparison.OrdinalIgnoreCase))
        {
            _ = _localModelManager.EnsureReadyForInterviewAsync();
            _ = _avatarSpeech.Ensure3DServersRunningAsync(forceRestart: false);
        }
        if (interview.Status != "active")
        {
            return BadRequest(new { error = "interview_not_active", message = "This interview has already concluded." });
        }

        var transcript = JsonSerializer.Deserialize<List<InterviewMessageDto>>(interview.Transcript)
                         ?? new List<InterviewMessageDto>();

        var action = dto.Action?.Trim().ToLowerInvariant();
        var isCandidateAnswer = string.IsNullOrEmpty(action) || action == "answer";

        if (isCandidateAnswer)
        {
            transcript.Add(new InterviewMessageDto("user", dto.Content, DateTime.UtcNow));
        }

        // Resolve live camera telemetry (only used if camera is active and include_in_score is enabled)
        var visualContext = dto.VisualContext;
        if (visualContext is { CameraActive: true } && visualContext.VisualScore == 0)
        {
            var latestSample = _visualAggregator.GetLatestSample(interview.Id.ToString());
            if (latestSample is not null)
            {
                visualContext = visualContext with
                {
                    PersonPresent = latestSample.PersonPresent,
                    VisualScore = latestSample.OverallScore,
                    PresenceScore = latestSample.PresenceScore,
                    HandsScore = latestSample.HandsScore,
                    LightingScore = latestSample.LightingScore,
                    AppearanceScore = latestSample.AppearanceScore,
                    PostureScore = latestSample.PostureScore,
                    EmotionScore = latestSample.EmotionScore,
                    Emotion = latestSample.Emotion,
                    Summary = latestSample.Summary
                };
            }
        }

        // Get AI evaluation, explanation, or next question
        var outcome = await _aiService.HandleTurnActionAsync(
            interview,
            transcript,
            action,
            HttpContext.RequestAborted,
            visualContext);

        // Append AI response (preserving optional 4-criteria scores in transcript metadata)
        transcript.Add(new InterviewMessageDto("assistant", outcome.Message, DateTime.UtcNow, outcome.StoredCriteria));

        interview.Transcript = JsonSerializer.Serialize(transcript);
        interview.QuestionCount = transcript.Count(m => m.Role == "user") + (outcome.AdvancesQuestion ? 1 : 0);
        interview.UpdatedAt = DateTime.UtcNow;

        await _context.SaveChangesAsync();

        return Ok(new SendMessageResponseDto(
            outcome.Message,
            interview.QuestionCount,
            IsCompleted: false,
            AwaitingNextAction: outcome.AwaitingNextAction,
            LastScore: outcome.Score,
            CriteriaBreakdown: outcome.CriteriaBreakdown,
            SuggestedActions: outcome.SuggestedActions,
            Emotion: outcome.Emotion,
            ActionsSequence: outcome.ActionsSequence,
            UiCommands: outcome.UiCommands
        ));
    }

    [HttpPost("{id:guid}/complete")]
    public async Task<ActionResult<CompleteInterviewResponseDto>> CompleteInterview(
        Guid id,
        [FromBody(EmptyBodyBehavior = Microsoft.AspNetCore.Mvc.ModelBinding.EmptyBodyBehavior.Allow)]
        CompleteInterviewRequestDto? request = null)
    {
        var userId = User.GetUserId();

        var interview = await _context.Interviews
            .FirstOrDefaultAsync(i => i.Id == id && i.UserId == userId);

        if (interview == null) return NotFound();

        if (string.Equals(interview.Status, "active", StringComparison.OrdinalIgnoreCase))
        {
            _ = _localModelManager.EnsureReadyForInterviewAsync();
            _ = _avatarSpeech.Ensure3DServersRunningAsync(forceRestart: false);
        }
        if (interview.Status == "completed" && !string.IsNullOrWhiteSpace(interview.Feedback))
        {
            var existingFeedback = JsonSerializer.Deserialize<InterviewFeedbackDto>(interview.Feedback)!;
            return Ok(new CompleteInterviewResponseDto(interview.Score ?? existingFeedback.Score, existingFeedback));
        }

        var transcript = JsonSerializer.Deserialize<List<InterviewMessageDto>>(interview.Transcript)
                         ?? new List<InterviewMessageDto>();

        var feedback = await _aiService.GenerateFeedbackAsync(interview, transcript);
        var resolvedVisual = request?.VisualAssessment ?? _visualAggregator.GetAggregate(interview.Id.ToString());
        if (resolvedVisual is not null && resolvedVisual.SamplesAnalyzed > 0)
        {
            feedback = EnrichFeedbackWithVisual(feedback, resolvedVisual, interview.Language);
        }

        interview.Status = "completed";
        interview.Feedback = JsonSerializer.Serialize(feedback);
        interview.Score = feedback.Score;
        interview.UpdatedAt = DateTime.UtcNow;

        await _context.SaveChangesAsync();

        return Ok(new CompleteInterviewResponseDto(feedback.Score, feedback));
    }

    private static InterviewFeedbackDto EnrichFeedbackWithVisual(
        InterviewFeedbackDto feedback,
        VisualCriteriaDto visual,
        string language)
    {
        var isUk = string.Equals(language, "uk", StringComparison.OrdinalIgnoreCase);
        var strengths = new List<string>(feedback.Strengths);
        var weaknesses = new List<string>(feedback.Weaknesses);

        if (visual.PresenceScore < 45)
        {
            weaknesses.Add(isUk
                ? $"Низька присутність у кадрі під час співбесіди ({visual.PresenceScore}/100) — кандидата часто не було видно перед камерою."
                : $"Low camera presence during the interview ({visual.PresenceScore}/100) — frequently absent from the frame.");
        }
        else if (visual.PresenceScore >= 80 && visual.PostureScore >= 75)
        {
            strengths.Add(isUk
                ? $"Стабільна присутність у кадрі ({visual.PresenceScore}/100), хороша постава та зоровий контакт ({visual.PostureScore}/100)."
                : $"Consistent camera presence ({visual.PresenceScore}/100), good posture and eye contact ({visual.PostureScore}/100).");
        }

        if (visual.LightingScore < 50)
        {
            weaknesses.Add(isUk
                ? $"Слабке або нерівномірне освітлення на камері ({visual.LightingScore}/100) — варто додати фронтальне світло."
                : $"Weak or uneven camera lighting ({visual.LightingScore}/100) — consider adding front lighting.");
        }
        else if (visual.LightingScore >= 80 && visual.AppearanceScore >= 78)
        {
            strengths.Add(isUk
                ? $"Якісне освітлення ({visual.LightingScore}/100) та охайний професійний вигляд у кадрі ({visual.AppearanceScore}/100)."
                : $"Clear lighting ({visual.LightingScore}/100) and neat professional presentation ({visual.AppearanceScore}/100).");
        }

        if (visual.EmotionScore >= 78 && visual.PresenceScore >= 60)
        {
            strengths.Add(isUk
                ? $"Впевнена міміка та спокійний емоційний стан упродовж розмови ({visual.EmotionScore}/100, стан: {visual.DominantEmotion})."
                : $"Composed facial expression and steady demeanor throughout the session ({visual.EmotionScore}/100, state: {visual.DominantEmotion}).");
        }
        else if (visual.EmotionScore < 45 && visual.PresenceScore >= 45)
        {
            weaknesses.Add(isUk
                ? $"Помітна скутість або відволікання погляду в кадрі ({visual.EmotionScore}/100)."
                : $"Noticeable tension or gaze distraction on camera ({visual.EmotionScore}/100).");
        }

        // If camera scoring is enabled and individual questions didn't all have per-turn visual criteria yet,
        // factor the session visual score (15%) into the final score.
        var anyTurnHadVisual = feedback.QuestionScores.Any(q => q.Criteria != null && q.Criteria.ContainsKey("visual"));
        var finalScore = feedback.Score;
        if (visual.IncludedInScore && !anyTurnHadVisual && feedback.QuestionScores.Count > 0)
        {
            finalScore = Math.Clamp((int)Math.Round(feedback.Score * 0.85 + visual.OverallScore * 0.15), 0, 100);
        }

        var visualSummaryNote = isUk
            ? $" Візуальна оцінка по камері: {visual.OverallScore}/100 (присутність {visual.PresenceScore}%, світло {visual.LightingScore}%, емоції {visual.EmotionScore}%, постава {visual.PostureScore}%)."
            : $" Camera presentation score: {visual.OverallScore}/100 (presence {visual.PresenceScore}%, lighting {visual.LightingScore}%, emotion {visual.EmotionScore}%, posture {visual.PostureScore}%).";

        var mergedSummary = feedback.Summary.Contains("Візуальна оцінка", StringComparison.OrdinalIgnoreCase)
                            || feedback.Summary.Contains("Camera presentation", StringComparison.OrdinalIgnoreCase)
            ? feedback.Summary
            : feedback.Summary.TrimEnd() + visualSummaryNote;

        return feedback with
        {
            Score = finalScore,
            Summary = mergedSummary,
            Strengths = strengths,
            Weaknesses = weaknesses,
            VisualAssessment = visual
        };
    }

    [HttpGet("stats")]
    public async Task<ActionResult<DashboardStatsDto>> GetStats()
    {
        var userId = User.GetUserId();
        var dailyLimit = _config.GetValue<int>("DailyInterviewLimit", 10);
        var today = DateOnly.FromDateTime(DateTime.UtcNow);

        var user = await _context.Users.AsNoTracking().FirstOrDefaultAsync(u => u.Id == userId);
        var interviews = await _context.Interviews
            .AsNoTracking()
            .Where(i => i.UserId == userId && i.Status == "completed")
            .ToListAsync();

        var countToday = (user != null && user.LastInterviewDate == today) ? user.DailyInterviewsCount : 0;
        var avgScore = interviews.Count > 0 ? interviews.Average(i => i.Score ?? 0) : 0;

        return Ok(new DashboardStatsDto(
            TotalInterviews: interviews.Count,
            AverageScore: Math.Round(avgScore, 1),
            DailyInterviewsCount: countToday,
            DailyLimit: dailyLimit,
            RemainingToday: Math.Max(0, dailyLimit - countToday)
        ));
    }
}
