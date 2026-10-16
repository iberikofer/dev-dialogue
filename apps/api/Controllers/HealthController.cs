using Microsoft.AspNetCore.Mvc;

namespace DevDialogue.Api.Controllers;

[ApiController]
[Route("[controller]")]
public class HealthController : ControllerBase
{
    [HttpGet]
    [Route("/health")]
    public IActionResult GetHealth()
    {
        return Ok(new
        {
            status = "healthy",
            service = "dev-dialogue-api",
            timestamp = DateTime.UtcNow
        });
    }
}
