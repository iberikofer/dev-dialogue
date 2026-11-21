using DevDialogue.Api.Data;
using DevDialogue.Api.Services.AI;
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

    [HttpGet]
    [Route("/api/health/local-model")]
    public async Task<ActionResult<LocalModelStatusDto>> GetLocalModelStatus(
        [FromServices] LocalModelManager modelManager,
        [FromServices] AvatarSpeechService avatarSpeech,
        [FromServices] ParakeetSpeechToTextService sttService,
        CancellationToken cancellationToken)
    {
        var baseStatus = await modelManager.GetStatusAsync(cancellationToken, autoStartOllama: true);
        var avatarStatus = avatarSpeech.Get3DServersStatus();
        if (!avatarStatus.Http8080Running || !avatarStatus.Tts8085Running)
        {
            avatarStatus = await avatarSpeech.Ensure3DServersRunningAsync(forceRestart: false, cancellationToken);
        }

        var full = baseStatus with
        {
            AvatarServerReady = avatarStatus.Ready,
            AvatarHttp8080Running = avatarStatus.Http8080Running,
            AvatarTts8085Running = avatarStatus.Tts8085Running,
            RhubarbAvailable = avatarStatus.RhubarbAvailable,
            AvatarStatusText = avatarStatus.StatusText,
            SttModelReady = sttService.IsModelAvailable,
            SttModelInitialized = sttService.IsInitialized,
            SttModelName = sttService.ModelName
        };
        return Ok(full);
    }

    [HttpPost]
    [Route("/api/health/services/ensure")]
    public async Task<ActionResult<LocalModelStatusDto>> EnsureServicesRunning(
        [FromServices] LocalModelManager modelManager,
        [FromServices] AvatarSpeechService avatarSpeech,
        [FromServices] ParakeetSpeechToTextService sttService,
        CancellationToken cancellationToken)
    {
        await modelManager.EnsureReadyForInterviewAsync(cancellationToken);
        var avatarStatus = await avatarSpeech.Ensure3DServersRunningAsync(forceRestart: false, cancellationToken);
        var baseStatus = await modelManager.GetStatusAsync(cancellationToken, autoStartOllama: true);

        return Ok(baseStatus with
        {
            AvatarServerReady = avatarStatus.Ready,
            AvatarHttp8080Running = avatarStatus.Http8080Running,
            AvatarTts8085Running = avatarStatus.Tts8085Running,
            RhubarbAvailable = avatarStatus.RhubarbAvailable,
            AvatarStatusText = avatarStatus.StatusText,
            SttModelReady = sttService.IsModelAvailable,
            SttModelInitialized = sttService.IsInitialized,
            SttModelName = sttService.ModelName
        });
    }

    public sealed record ServiceRestartRequestDto(string? Target = "all");

    [HttpPost]
    [Route("/api/health/services/restart")]
    public async Task<ActionResult<LocalModelStatusDto>> RestartServices(
        [FromBody] ServiceRestartRequestDto? request,
        [FromServices] LocalModelManager modelManager,
        [FromServices] AvatarSpeechService avatarSpeech,
        [FromServices] ParakeetSpeechToTextService sttService,
        CancellationToken cancellationToken)
    {
        var target = (request?.Target ?? "all").Trim().ToLowerInvariant();

        if (target is "all" or "avatar" or "3d")
        {
            await avatarSpeech.Ensure3DServersRunningAsync(forceRestart: true, cancellationToken);
        }

        if (target is "all" or "models" or "ollama")
        {
            await modelManager.RestartOllamaAsync(cancellationToken);
        }

        if (target is "all" or "models" or "stt")
        {
            await sttService.EnsureReadyAsync(forceReload: true, cancellationToken);
        }

        var avatarStatus = avatarSpeech.Get3DServersStatus();
        var baseStatus = await modelManager.GetStatusAsync(cancellationToken, autoStartOllama: true);

        return Ok(baseStatus with
        {
            AvatarServerReady = avatarStatus.Ready,
            AvatarHttp8080Running = avatarStatus.Http8080Running,
            AvatarTts8085Running = avatarStatus.Tts8085Running,
            RhubarbAvailable = avatarStatus.RhubarbAvailable,
            AvatarStatusText = avatarStatus.StatusText,
            SttModelReady = sttService.IsModelAvailable,
            SttModelInitialized = sttService.IsInitialized,
            SttModelName = sttService.ModelName
        });
    }

    [HttpPost]
    [Route("/api/health/local-model/download")]
    public async Task<ActionResult<LocalModelStatusDto>> StartLocalModelDownload(
        [FromServices] LocalModelManager modelManager,
        CancellationToken cancellationToken)
    {
        var status = await modelManager.StartDownloadAsync(cancellationToken);
        return Accepted(status);
    }
}
