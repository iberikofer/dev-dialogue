using DevDialogue.Api.Services.AI;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DevDialogue.Api.Controllers;

public sealed record TtsRequestDto(
    string Text,
    string? Voice = "uk-UA-OstapNeural",
    string? Rate = "+0%",
    string? Pitch = "+0Hz");

[ApiController]
[AllowAnonymous]
[Route("api/tts")]
public class AvatarController : ControllerBase
{
    private readonly AvatarSpeechService _speechService;
    private readonly ILogger<AvatarController> _logger;

    public AvatarController(AvatarSpeechService speechService, ILogger<AvatarController> logger)
    {
        _speechService = speechService;
        _logger = logger;
    }

    [HttpGet("status")]
    public IActionResult GetStatus()
    {
        return Ok(new
        {
            status = "online",
            engine = "Edge-TTS + Rhubarb Lip-Sync",
            avatarAvailable = _speechService.AvatarRootPath is not null,
            rhubarbAvailable = !string.IsNullOrEmpty(_speechService.RhubarbExePath)
        });
    }

    [HttpGet("voices")]
    public IActionResult GetVoices()
    {
        return Ok(new { voices = _speechService.GetVoices() });
    }

    [HttpPost]
    [HttpPost("speak")]
    public async Task<IActionResult> Speak([FromBody] TtsRequestDto request, CancellationToken cancellationToken)
    {
        if (request is null || string.IsNullOrWhiteSpace(request.Text))
            return BadRequest(new { error = "Text cannot be empty" });

        try
        {
            var result = await _speechService.SynthesizeAsync(
                request.Text,
                request.Voice,
                request.Rate,
                request.Pitch,
                cancellationToken);
            return Ok(result);
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Avatar TTS synthesis failed for voice {Voice}", request.Voice);
            return StatusCode(500, new { error = ex.Message });
        }
    }
}
