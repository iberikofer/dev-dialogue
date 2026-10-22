using DevDialogue.Api.Data;
using DevDialogue.Api.Helpers;
using DevDialogue.Api.Models;
using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;
using DevDialogue.Api.Security;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Controllers;

[ApiController]
[Route("api/admin/questions")]
[Authorize]
[RequireRole("admin")]
public class AdminQuestionsController : ControllerBase
{
    private readonly AppDbContext _context;

    public AdminQuestionsController(AppDbContext context)
    {
        _context = context;
    }

    [HttpGet]
    public async Task<ActionResult<PagedResult<QuestionBankDto>>> GetQuestions(
        [FromQuery] string? direction,
        [FromQuery] string? level,
        [FromQuery] string? stage,
        [FromQuery] bool? isActive,
        [FromQuery] int page = 1,
        [FromQuery] int pageSize = 20)
    {
        var query = _context.QuestionsBank.AsNoTracking().AsQueryable();

        if (!string.IsNullOrEmpty(direction))
            query = query.Where(q => q.Direction == direction);
        if (!string.IsNullOrEmpty(level))
            query = query.Where(q => q.Level == level);
        if (!string.IsNullOrEmpty(stage))
            query = query.Where(q => q.Stage == stage);
        if (isActive.HasValue)
            query = query.Where(q => q.IsActive == isActive.Value);

        var pagedResult = await query
            .OrderByDescending(q => q.CreatedAt)
            .ThenByDescending(q => q.Id)
            .Select(q => new QuestionBankDto(
                q.Id,
                q.Direction,
                q.Stage,
                q.Level,
                q.Topic,
                q.QuestionText,
                q.IsActive,
                q.CreatedBy,
                q.CreatedAt
            ))
            .ToPagedAsync(page, pageSize);

        return Ok(pagedResult);
    }

    [HttpPost]
    public async Task<ActionResult<QuestionBankDto>> CreateQuestion([FromBody] CreateQuestionDto dto)
    {
        if (!SupportedOptions.Directions.Contains(dto.Direction.ToLowerInvariant()))
        {
            return BadRequest(new { error = $"Invalid direction '{dto.Direction}'. Supported: {string.Join(", ", SupportedOptions.Directions)}", code = "INVALID_DIRECTION" });
        }

        if (!SupportedOptions.Levels.Contains(dto.Level.ToLowerInvariant()))
        {
            return BadRequest(new { error = $"Invalid level '{dto.Level}'. Supported: {string.Join(", ", SupportedOptions.Levels)}", code = "INVALID_LEVEL" });
        }

        if (!SupportedOptions.Stages.Contains(dto.Stage.ToLowerInvariant()))
        {
            return BadRequest(new { error = $"Invalid stage '{dto.Stage}'. Supported: {string.Join(", ", SupportedOptions.Stages)}", code = "INVALID_STAGE" });
        }

        var userId = User.GetUserId();

        var item = new QuestionBank
        {
            Id = Guid.NewGuid(),
            Direction = dto.Direction.ToLowerInvariant(),
            Stage = dto.Stage.ToLowerInvariant(),
            Level = dto.Level.ToLowerInvariant(),
            Topic = dto.Topic,
            QuestionText = dto.QuestionText,
            IsActive = true,
            CreatedBy = userId,
            CreatedAt = DateTime.UtcNow
        };

        _context.QuestionsBank.Add(item);
        await _context.SaveChangesAsync();

        return CreatedAtAction(nameof(GetQuestions), new { id = item.Id }, new QuestionBankDto(
            item.Id,
            item.Direction,
            item.Stage,
            item.Level,
            item.Topic,
            item.QuestionText,
            item.IsActive,
            item.CreatedBy,
            item.CreatedAt
        ));
    }

    [HttpPut("{id:guid}")]
    public async Task<ActionResult<QuestionBankDto>> UpdateQuestion(Guid id, [FromBody] UpdateQuestionDto dto)
    {
        var item = await _context.QuestionsBank.FirstOrDefaultAsync(q => q.Id == id);
        if (item == null) return NotFound();

        if (!string.IsNullOrWhiteSpace(dto.Direction)) item.Direction = dto.Direction;
        if (!string.IsNullOrWhiteSpace(dto.Stage)) item.Stage = dto.Stage;
        if (!string.IsNullOrWhiteSpace(dto.Level)) item.Level = dto.Level;
        if (!string.IsNullOrWhiteSpace(dto.Topic)) item.Topic = dto.Topic;
        if (!string.IsNullOrWhiteSpace(dto.QuestionText)) item.QuestionText = dto.QuestionText;
        if (dto.IsActive.HasValue) item.IsActive = dto.IsActive.Value;

        await _context.SaveChangesAsync();

        return Ok(new QuestionBankDto(
            item.Id,
            item.Direction,
            item.Stage,
            item.Level,
            item.Topic,
            item.QuestionText,
            item.IsActive,
            item.CreatedBy,
            item.CreatedAt
        ));
    }

    [HttpDelete("{id:guid}")]
    public async Task<IActionResult> DeleteQuestion(Guid id)
    {
        var item = await _context.QuestionsBank.FirstOrDefaultAsync(q => q.Id == id);
        if (item == null) return NotFound();

        _context.QuestionsBank.Remove(item);
        await _context.SaveChangesAsync();

        return NoContent();
    }
}
