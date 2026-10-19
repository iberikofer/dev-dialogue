using Microsoft.AspNetCore.Mvc;

namespace DevDialogue.Api.Controllers;

[ApiController]
[Route("api/[controller]")]
public class OptionsController : ControllerBase
{
    [HttpGet]
    public IActionResult GetOptions()
    {
        return Ok(new
        {
            directions = new[] { "frontend", "backend", "devops", "qa", "pm" },
            levels = new[] { "junior", "middle", "senior" },
            languages = new[] { "uk", "en" }
        });
    }
}