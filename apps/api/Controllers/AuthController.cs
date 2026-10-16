using DevDialogue.Api.Data;
using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;
using DevDialogue.Api.Security;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Controllers;

[ApiController]
[Route("api/[controller]")]
[Authorize]
public class AuthController : ControllerBase
{
    private readonly AppDbContext _context;
    private readonly ILogger<AuthController> _logger;

    public AuthController(AppDbContext context, ILogger<AuthController> logger)
    {
        _context = context;
        _logger = logger;
    }

    [HttpGet("me")]
    public async Task<ActionResult<AuthMeDto>> GetMe()
    {
        var userId = User.GetUserId();
        var email = User.GetUserEmail();

        var user = await _context.Users.FirstOrDefaultAsync(u => u.Id == userId);

        if (user == null)
        {
            // Self-healing: create profile if first login
            user = new User
            {
                Id = userId,
                Email = string.IsNullOrEmpty(email) ? "user@example.com" : email,
                FullName = User.FindFirst("name")?.Value,
                Role = "user",
                DailyInterviewsCount = 0,
                CreatedAt = DateTime.UtcNow,
                UpdatedAt = DateTime.UtcNow
            };

            _context.Users.Add(user);
            await _context.SaveChangesAsync();
            _logger.LogInformation("Created new user record for {UserId}", userId);
        }

        return Ok(new AuthMeDto(
            user.Id,
            user.Email,
            user.FullName,
            user.AvatarUrl,
            user.Role
        ));
    }
}
