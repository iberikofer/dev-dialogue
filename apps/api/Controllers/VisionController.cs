using DevDialogue.Api.Services.AI;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DevDialogue.Api.Controllers;

[ApiController]
[AllowAnonymous]
[Route("api/vision")]
public class VisionController : ControllerBase
{
    private readonly LocalVisionService _visionService;
    private readonly ILogger<VisionController> _logger;

    public VisionController(LocalVisionService visionService, ILogger<VisionController> logger)
    {
        _visionService = visionService;
        _logger = logger;
    }

    [HttpGet("status")]
    public async Task<IActionResult> GetStatus(CancellationToken cancellationToken)
    {
        var available = await _visionService.IsVisionModelAvailableAsync(cancellationToken);
        return Ok(new
        {
            available,
            model = _visionService.VisionModelName,
            localOnly = true
        });
    }

    [HttpPost("analyze")]
    [RequestSizeLimit(4 * 1024 * 1024)]
    public async Task<ActionResult<VisionAnalyzeResponse>> AnalyzeFrame(
        [FromBody] VisionAnalyzeRequest request,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(request.ImageBase64))
        {
            return BadRequest(new { error = "image_base64 is required." });
        }

        try
        {
            var result = await _visionService.AnalyzeFrameAsync(request, cancellationToken);
            return Ok(result);
        }
        catch (OperationCanceledException)
        {
            return StatusCode(499);
        }
        catch (Exception ex)
        {
            _logger.LogDebug(ex, "Vision analysis error");
            return StatusCode(500, new { error = ex.Message });
        }
    }
}
