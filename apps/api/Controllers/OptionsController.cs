using DevDialogue.Api.Models;
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
            directions = SupportedOptions.Directions,
            levels = SupportedOptions.Levels,
            stages = SupportedOptions.Stages,
            languages = new[]
            {
                new { id = "uk", code = "uk", name = "Українська" },
                new { id = "en", code = "en", name = "English" }
            }
        });
    }
}