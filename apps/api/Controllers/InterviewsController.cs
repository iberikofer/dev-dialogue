using System.Text.Json;
using DevDialogue.Api.Data;
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
    private readonly IConfiguration _config;
    private readonly ILogger<InterviewsController> _logger;

    public InterviewsController(
        AppDbContext context,
        IAiService aiService,
        IConfiguration config,
        ILogger<InterviewsController> logger)
    {
        _context = context;
        _aiService = aiService;
        _config = config;
        _logger = logger;
    }

    [HttpPost]
    public async Task<ActionResult<CreateInterviewResponseDto>> CreateInterview([FromBody] CreateInterviewDto dto)
    {
        var userId = User.GetUserId();
        var dailyLimit = _config.GetValue<int>("DailyInterviewLimit", 10);
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

        // Reset limit if it is a new day
        if (user.LastInterviewDate != today)
        {
            user.DailyInterviewsCount = 0;
            user.LastInterviewDate = today;
        }

        // Check daily interview limit
        if (user.DailyInterviewsCount >= dailyLimit)
        {
            return BadRequest(new
            {
                error = "daily_limit_reached",
                message = $"You have reached the daily limit of {dailyLimit} interviews. Please try again tomorrow.",
                limit = dailyLimit
            });
        }

        // Lookup starting topic from QuestionBank if available
        var seedQuestion = await _context.QuestionsBank
            .AsNoTracking()
            .Where(q => q.Direction == dto.Direction && q.Level == dto.Level && q.IsActive)
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
            Direction = dto.Direction,
            Level = dto.Level,
            Language = dto.Language,
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

        return Ok(new CreateInterviewResponseDto(interview.Id, firstQuestion));
    }

    [HttpGet]
    public async Task<ActionResult<List<InterviewSummaryDto>>> GetInterviews([FromQuery] int page = 1, [FromQuery] int pageSize = 20)
    {
        var userId = User.GetUserId();

        var query = _context.Interviews
            .AsNoTracking()
            .Where(i => i.UserId == userId)
            .OrderByDescending(i => i.CreatedAt);

        var items = await query
            .Skip((page - 1) * pageSize)
            .Take(pageSize)
            .Select(i => new InterviewSummaryDto(
                i.Id,
                i.UserId,
                i.Direction,
                i.Level,
                i.Language,
                i.Stage,
                i.Status,
                i.Score,
                i.QuestionCount,
                i.CreatedAt,
                i.UpdatedAt
            ))
            .ToListAsync();

        return Ok(items);
    }

    [HttpGet("{id:guid}")]
    public async Task<ActionResult<InterviewDetailDto>> GetInterview(Guid id)
    {
        var userId = User.GetUserId();

        var interview = await _context.Interviews
            .AsNoTracking()
            .FirstOrDefaultAsync(i => i.Id == id && i.UserId == userId);

        if (interview == null) return NotFound();

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
        if (interview.Status != "active")
        {
            return BadRequest(new { error = "interview_not_active", message = "This interview has already concluded." });
        }

        var transcript = JsonSerializer.Deserialize<List<InterviewMessageDto>>(interview.Transcript)
                         ?? new List<InterviewMessageDto>();

        // Append candidate message
        transcript.Add(new InterviewMessageDto("user", dto.Content, DateTime.UtcNow));

        // Get AI response
        var aiResponse = await _aiService.GenerateNextResponseAsync(interview, transcript);

        // Append AI response
        transcript.Add(new InterviewMessageDto("assistant", aiResponse, DateTime.UtcNow));

        interview.Transcript = JsonSerializer.Serialize(transcript);
        interview.QuestionCount = transcript.Count(m => m.Role == "user") + 1;
        interview.UpdatedAt = DateTime.UtcNow;

        await _context.SaveChangesAsync();

        return Ok(new SendMessageResponseDto(aiResponse, interview.QuestionCount));
    }

    [HttpPost("{id:guid}/complete")]
    public async Task<ActionResult<CompleteInterviewResponseDto>> CompleteInterview(Guid id)
    {
        var userId = User.GetUserId();

        var interview = await _context.Interviews
            .FirstOrDefaultAsync(i => i.Id == id && i.UserId == userId);

        if (interview == null) return NotFound();
        if (interview.Status == "completed" && !string.IsNullOrWhiteSpace(interview.Feedback))
        {
            var existingFeedback = JsonSerializer.Deserialize<InterviewFeedbackDto>(interview.Feedback)!;
            return Ok(new CompleteInterviewResponseDto(interview.Score ?? existingFeedback.Score, existingFeedback));
        }

        var transcript = JsonSerializer.Deserialize<List<InterviewMessageDto>>(interview.Transcript)
                         ?? new List<InterviewMessageDto>();

        var feedback = await _aiService.GenerateFeedbackAsync(interview, transcript);

        interview.Status = "completed";
        interview.Feedback = JsonSerializer.Serialize(feedback);
        interview.Score = feedback.Score;
        interview.UpdatedAt = DateTime.UtcNow;

        await _context.SaveChangesAsync();

        return Ok(new CompleteInterviewResponseDto(feedback.Score, feedback));
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
