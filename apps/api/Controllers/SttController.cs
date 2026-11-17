using DevDialogue.Api.Services.AI;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DevDialogue.Api.Controllers;

[ApiController]
[AllowAnonymous]
[Route("api/stt")]
public class SttController : ControllerBase
{
    private readonly ParakeetSpeechToTextService _sttService;
    private readonly ILogger<SttController> _logger;

    public SttController(ParakeetSpeechToTextService sttService, ILogger<SttController> logger)
    {
        _sttService = sttService;
        _logger = logger;
    }

    [HttpGet("status")]
    public IActionResult GetStatus()
    {
        return Ok(new
        {
            available = _sttService.IsModelAvailable,
            initialized = _sttService.IsInitialized,
            model = _sttService.ModelName,
            modelDirectory = _sttService.ModelDirectory
        });
    }

    [HttpPost]
    [RequestSizeLimit(32 * 1024 * 1024)]
    public async Task<IActionResult> Transcribe(CancellationToken cancellationToken)
    {
        try
        {
            byte[] wavBytes;
            if (Request.HasFormContentType)
            {
                var form = await Request.ReadFormAsync(cancellationToken);
                var file = form.Files.FirstOrDefault();
                if (file is null || file.Length == 0)
                    return BadRequest(new { error = "Audio file is missing." });
                using var ms = new MemoryStream();
                await file.CopyToAsync(ms, cancellationToken);
                wavBytes = ms.ToArray();
            }
            else
            {
                using var ms = new MemoryStream();
                await Request.Body.CopyToAsync(ms, cancellationToken);
                wavBytes = ms.ToArray();
            }

            if (wavBytes.Length < 44)
                return BadRequest(new { error = "Empty audio payload." });

            var result = await _sttService.TranscribeWavAsync(wavBytes, cancellationToken);
            return Ok(result);
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Parakeet STT failed.");
            return StatusCode(500, new { error = ex.Message });
        }
    }
}
