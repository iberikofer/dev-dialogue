using DevDialogue.Api.Data;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace DevDialogue.Api.Controllers;

[ApiController]
[Route("[controller]")]
public class HealthController : ControllerBase
{
    [HttpGet]
    [Route("/health")]
    [Route("/api/health/ping")]
    public IActionResult GetHealth()
    {
        return Ok(new
        {
            status = "healthy",
            service = "dev-dialogue-api",
            timestamp = DateTime.UtcNow
        });
    }

    [HttpGet]
    [Route("/api/health/db")]
    public async Task<IActionResult> CheckDatabase([FromServices] AppDbContext db)
    {
        try
        {
            var canConnect = await db.Database.CanConnectAsync();
            return canConnect
                ? Ok(new
                {
                    status = "DB connected",
                    database = db.Database.ProviderName,
                    timestamp = DateTime.UtcNow
                })
                : StatusCode(500, new { status = "DB connection failed" });
        }
        catch (Exception ex)
        {
            return StatusCode(500, new { status = "DB error", message = ex.Message });
        }
    }
}
