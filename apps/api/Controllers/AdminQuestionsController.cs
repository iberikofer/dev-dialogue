using DevDialogue.Api.Data;
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
    public async Task<ActionResult<List<QuestionBankDto>>> GetQuestions(
        [FromQuery] string? direction,
        [FromQuery] string? level,
        [FromQuery] string? stage,
        [FromQuery] bool? isActive)
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

        var items = await query
            .OrderByDescending(q => q.CreatedAt)
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
            .ToListAsync();

        return Ok(items);
    }

    [HttpPost]
    public async Task<ActionResult<QuestionBankDto>> CreateQuestion([FromBody] CreateQuestionDto dto)
    {
        var userId = User.GetUserId();

        var item = new QuestionBank
        {
            Id = Guid.NewGuid(),
            Direction = dto.Direction,
            Stage = dto.Stage,
            Level = dto.Level,
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
