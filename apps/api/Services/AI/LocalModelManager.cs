using System.Diagnostics;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;

namespace DevDialogue.Api.Services.AI;

public sealed record LocalModelStatusDto(
    string AiProvider,
    string Model,
    bool Required,
    bool OllamaInstalled,
    bool OllamaRunning,
    bool ModelDownloaded,
    bool IsDownloading,
    int DownloadProgress,
    string StatusText,
    string Error,
    bool AvatarServerReady = true,
    bool AvatarHttp8080Running = false,
    bool AvatarTts8085Running = false,
    bool RhubarbAvailable = false,
    string AvatarStatusText = "",
    bool SttModelReady = true,
    bool SttModelInitialized = false,
    string SttModelName = "parakeet-tdt-0.6b-v3"
);

/// <summary>
/// Inspects and downloads the local Ollama model (default: gemma4:e4b)
/// so interviews cannot start while the required local model is missing.
/// </summary>
public sealed class LocalModelManager
{
    private readonly IConfiguration _config;
    private readonly ILogger<LocalModelManager> _logger;
    private readonly ParakeetSpeechToTextService _sttService;
    private readonly object _sync = new();

    private bool _isDownloading;
    private int _downloadProgress;
    private string _statusText = string.Empty;
    private string _lastError = string.Empty;

    public LocalModelManager(
        IConfiguration config,
        ILogger<LocalModelManager> logger,
        ParakeetSpeechToTextService sttService)
    {
        _config = config;
        _logger = logger;
        _sttService = sttService;
    }

    public string AiProvider => _config["AI:Provider"] ?? "Mock";

    public string OllamaUrl =>
        (_config["AI:OllamaUrl"] ?? Environment.GetEnvironmentVariable("OLLAMA_URL") ?? "http://127.0.0.1:11434").TrimEnd('/');

    public string ModelName =>
        _config["AI:OllamaModel"] ?? Environment.GetEnvironmentVariable("OLLAMA_MODEL") ?? "gemma4:e4b";

    public bool IsLocalModelRequired =>
        string.Equals(AiProvider, "Ollama", StringComparison.OrdinalIgnoreCase);

    public async Task<LocalModelStatusDto> GetStatusAsync(
        CancellationToken cancellationToken = default,
        bool autoStartOllama = true)
    {
        var model = ModelName;
        var ollamaPath = FindOllamaExecutable();
        var installed = !string.IsNullOrEmpty(ollamaPath);
        var running = false;
        var modelDownloaded = false;

        if (installed && autoStartOllama && !await PingOllamaAsync(cancellationToken))
        {
            await EnsureOllamaRunningAsync(cancellationToken);
        }

        try
        {
            using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(2) };
            using var resp = await http.GetAsync($"{OllamaUrl}/api/tags", cancellationToken);
            if (resp.IsSuccessStatusCode)
            {
                running = true;
                var json = await resp.Content.ReadFromJsonAsync<JsonElement>(cancellationToken: cancellationToken);
                if (json.TryGetProperty("models", out var modelsArr) && modelsArr.ValueKind == JsonValueKind.Array)
                {
                    foreach (var item in modelsArr.EnumerateArray())
                    {
                        var name = item.TryGetProperty("name", out var n) ? n.GetString() : null;
                        var modelField = item.TryGetProperty("model", out var m) ? m.GetString() : null;
                        if (string.Equals(name, model, StringComparison.OrdinalIgnoreCase)
                            || string.Equals(modelField, model, StringComparison.OrdinalIgnoreCase))
                        {
                            modelDownloaded = true;
                            break;
                        }
                    }
                }
            }
        }
        catch
        {
            running = false;
        }

        if (!running && HasManifestOnDisk(model))
        {
            modelDownloaded = true;
        }

        bool downloading;
        int progress;
        string statusText;
        string error;
        lock (_sync)
        {
            downloading = _isDownloading;
            progress = modelDownloaded ? 100 : _downloadProgress;
            statusText = modelDownloaded
                ? (running ? "Ready" : "Ollama stopped")
                : (_isDownloading && !string.IsNullOrWhiteSpace(_statusText)
                    ? _statusText
                    : (installed ? "Model not downloaded" : "Ollama not installed"));
            error = _lastError;
        }

        return new LocalModelStatusDto(
            AiProvider: AiProvider,
            Model: model,
            Required: IsLocalModelRequired,
            OllamaInstalled: installed,
            OllamaRunning: running,
            ModelDownloaded: modelDownloaded,
            IsDownloading: downloading,
            DownloadProgress: progress,
            StatusText: statusText,
            Error: error
        );
    }

    public async Task<bool> RestartOllamaAsync(CancellationToken cancellationToken = default)
    {
        if (OperatingSystem.IsWindows())
        {
            foreach (var procName in new[] { "ollama" })
            {
                foreach (var p in Process.GetProcessesByName(procName))
                {
                    try { p.Kill(entireProcessTree: true); } catch { }
                    p.Dispose();
                }
            }
            await Task.Delay(350, cancellationToken);
        }

        return await EnsureOllamaRunningAsync(cancellationToken);
    }

    public async Task<bool> EnsureReadyForInterviewAsync(CancellationToken cancellationToken = default)
    {
        if (!_sttService.IsModelAvailable)
        {
            return false;
        }
        var status = await GetStatusAsync(cancellationToken);
        if (!status.Required)
        {
            return true;
        }
        if (!status.ModelDownloaded)
        {
            return false;
        }
        if (!status.OllamaRunning && status.OllamaInstalled)
        {
            await EnsureOllamaRunningAsync(cancellationToken);
        }
        return true;
    }

    public Task<LocalModelStatusDto> StartDownloadAsync(CancellationToken cancellationToken = default)
    {
        lock (_sync)
        {
            if (_isDownloading)
            {
                return GetStatusAsync(cancellationToken);
            }
            _isDownloading = true;
            _downloadProgress = 0;
            _lastError = string.Empty;
            _statusText = "Preparing local model download...";
        }

        _ = Task.Run(RunDownloadWorkflowAsync);
        return GetStatusAsync(cancellationToken);
    }

    private async Task RunDownloadWorkflowAsync()
    {
        try
        {
            var ollamaExe = FindOllamaExecutable();
            if (string.IsNullOrEmpty(ollamaExe))
            {
                UpdateProgress(2, "Downloading Ollama installer...");
                await InstallOllamaWindowsAsync();
                ollamaExe = FindOllamaExecutable();
                if (string.IsNullOrEmpty(ollamaExe))
                {
                    throw new InvalidOperationException("Ollama was installed, but ollama.exe was not found.");
                }
            }

            if (!await EnsureOllamaRunningAsync(CancellationToken.None))
            {
                throw new InvalidOperationException($"Ollama server did not respond on {OllamaUrl}.");
            }

            var pulled = await PullModelAsync();
            if (pulled == PullOutcome.RequiresOllamaUpdate)
            {
                UpdateProgress(3, "Updating Ollama to the latest version...");
                await InstallOllamaWindowsAsync();
                if (!await EnsureOllamaRunningAsync(CancellationToken.None))
                {
                    throw new InvalidOperationException($"Updated Ollama server did not respond on {OllamaUrl}.");
                }
                var retried = await PullModelAsync();
                if (retried == PullOutcome.RequiresOllamaUpdate)
                {
                    throw new InvalidOperationException("Ollama still requires a newer version to pull this model.");
                }
            }

            if (!_sttService.IsModelAvailable)
            {
                await DownloadParakeetSttAsync();
            }

            UpdateProgress(100, "Ready");
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to download local AI model {Model}", ModelName);
            lock (_sync)
            {
                _lastError = ex.Message;
                _statusText = "Download failed";
            }
        }
        finally
        {
            lock (_sync)
            {
                _isDownloading = false;
            }
        }
    }

    private enum PullOutcome
    {
        Succeeded,
        RequiresOllamaUpdate
    }

    private async Task<PullOutcome> PullModelAsync()
    {
        UpdateProgress(5, $"Downloading {ModelName}...");
        using var http = new HttpClient { Timeout = TimeSpan.FromMinutes(60) };
        var payload = new StringContent(
            JsonSerializer.Serialize(new { name = ModelName, stream = true }),
            Encoding.UTF8,
            "application/json");

        using var req = new HttpRequestMessage(HttpMethod.Post, $"{OllamaUrl}/api/pull") { Content = payload };
        using var resp = await http.SendAsync(req, HttpCompletionOption.ResponseHeadersRead);
        if ((int)resp.StatusCode == 412)
        {
            return PullOutcome.RequiresOllamaUpdate;
        }
        resp.EnsureSuccessStatusCode();

        await using var stream = await resp.Content.ReadAsStreamAsync();
        using var reader = new StreamReader(stream);

        var layerTotals = new Dictionary<string, (long Total, long Completed)>(StringComparer.Ordinal);
        var maxPullPct = 5;

        while (await reader.ReadLineAsync() is { } line)
        {
            if (string.IsNullOrWhiteSpace(line))
            {
                continue;
            }

            try
            {
                using var doc = JsonDocument.Parse(line);
                var root = doc.RootElement;
                if (root.TryGetProperty("error", out var errProp))
                {
                    var errText = errProp.GetString() ?? "Ollama pull failed.";
                    if (errText.Contains("412", StringComparison.OrdinalIgnoreCase)
                        || errText.Contains("newer version of Ollama", StringComparison.OrdinalIgnoreCase))
                    {
                        return PullOutcome.RequiresOllamaUpdate;
                    }
                    throw new InvalidOperationException(errText);
                }

                var status = root.TryGetProperty("status", out var sProp) ? sProp.GetString() ?? string.Empty : string.Empty;
                if (root.TryGetProperty("total", out var tProp) && root.TryGetProperty("completed", out var cProp))
                {
                    var total = tProp.GetInt64();
                    var completed = cProp.GetInt64();
                    if (total > 0)
                    {
                        var digestKey = root.TryGetProperty("digest", out var dProp)
                            ? dProp.GetString() ?? $"layer-{total}"
                            : $"layer-{total}";
                        layerTotals[digestKey] = (total, Math.Clamp(completed, 0, total));

                        long sumTotal = 0;
                        long sumCompleted = 0;
                        foreach (var entry in layerTotals.Values)
                        {
                            sumTotal += entry.Total;
                            sumCompleted += entry.Completed;
                        }

                        var rawPct = sumTotal > 0 ? (int)Math.Round(sumCompleted * 100.0 / sumTotal) : 5;
                        maxPullPct = Math.Max(maxPullPct, Math.Clamp(rawPct, 5, 99));
                        UpdateProgress(maxPullPct, $"Downloading {ModelName} ({maxPullPct}%)...");
                        continue;
                    }
                }

                if (!string.IsNullOrWhiteSpace(status))
                {
                    UpdateProgress(maxPullPct, status);
                }
            }
            catch (JsonException)
            {
                // Ignore partial non-JSON lines from stream
            }
        }

        return PullOutcome.Succeeded;
    }

    private async Task DownloadParakeetSttAsync()
    {
        var targetDir = ParakeetSpeechToTextService.DefaultInstallDirectory;
        Directory.CreateDirectory(targetDir);
        var files = new[] { "encoder.int8.onnx", "decoder.int8.onnx", "joiner.int8.onnx", "tokens.txt" };
        const string baseUrl = "https://huggingface.co/csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-v3-int8/resolve/main";

        using var http = new HttpClient { Timeout = TimeSpan.FromMinutes(30) };
        for (int i = 0; i < files.Length; i++)
        {
            var fileName = files[i];
            var dest = Path.Combine(targetDir, fileName);
            if (File.Exists(dest))
            {
                continue;
            }
            UpdateProgress(90 + i * 2, $"Downloading speech model ({fileName})...");
            using var resp = await http.GetAsync($"{baseUrl}/{fileName}", HttpCompletionOption.ResponseHeadersRead);
            resp.EnsureSuccessStatusCode();
            await using var src = await resp.Content.ReadAsStreamAsync();
            await using var dst = File.Create(dest);
            await src.CopyToAsync(dst);
        }
    }

    private async Task InstallOllamaWindowsAsync()
    {
        if (!OperatingSystem.IsWindows())
        {
            throw new PlatformNotSupportedException("Automatic Ollama installation is only supported on Windows.");
        }

        // Stop any older Ollama desktop/server processes so setup can replace binaries cleanly.
        foreach (var procName in new[] { "ollama app", "ollama" })
        {
            foreach (var p in Process.GetProcessesByName(procName))
            {
                try { p.Kill(entireProcessTree: true); } catch { }
                p.Dispose();
            }
        }
        await Task.Delay(800);

        var installerPath = Path.Combine(Path.GetTempPath(), "OllamaSetup.exe");
        using (var http = new HttpClient { Timeout = TimeSpan.FromMinutes(15) })
        {
            using var resp = await http.GetAsync("https://ollama.com/download/OllamaSetup.exe", HttpCompletionOption.ResponseHeadersRead);
            resp.EnsureSuccessStatusCode();
            var totalBytes = resp.Content.Headers.ContentLength ?? -1L;
            await using var src = await resp.Content.ReadAsStreamAsync();
            await using var dst = File.Create(installerPath);

            var buffer = new byte[81920];
            long downloadedBytes = 0;
            int read;
            while ((read = await src.ReadAsync(buffer)) > 0)
            {
                await dst.WriteAsync(buffer.AsMemory(0, read));
                downloadedBytes += read;
                if (totalBytes > 0)
                {
                    var pct = Math.Clamp((int)Math.Round(downloadedBytes * 100.0 / totalBytes), 3, 98);
                    UpdateProgress(pct, $"Downloading Ollama update ({pct}%)...");
                }
                else
                {
                    UpdateProgress(25, $"Downloading Ollama update ({downloadedBytes / (1024 * 1024)} MB)...");
                }
            }
        }

        UpdateProgress(4, "Installing latest Ollama...");
        using var proc = Process.Start(new ProcessStartInfo
        {
            FileName = installerPath,
            Arguments = "/VERYSILENT /NORESTART",
            UseShellExecute = true,
            WindowStyle = ProcessWindowStyle.Hidden
        });

        if (proc != null)
        {
            await proc.WaitForExitAsync();
            if (proc.ExitCode != 0)
            {
                throw new InvalidOperationException($"OllamaSetup.exe exited with code {proc.ExitCode}.");
            }
        }

        try { File.Delete(installerPath); } catch { }
    }

    private async Task<bool> EnsureOllamaRunningAsync(CancellationToken cancellationToken)
    {
        if (await PingOllamaAsync(cancellationToken))
        {
            return true;
        }

        var exe = FindOllamaExecutable();
        if (string.IsNullOrEmpty(exe))
        {
            return false;
        }

        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = exe,
                Arguments = "serve",
                UseShellExecute = false,
                CreateNoWindow = true,
                WindowStyle = ProcessWindowStyle.Hidden
            });
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Could not start ollama serve process.");
            return false;
        }

        for (int i = 0; i < 20; i++)
        {
            await Task.Delay(500, cancellationToken);
            if (await PingOllamaAsync(cancellationToken))
            {
                return true;
            }
        }

        return false;
    }

    private async Task<bool> PingOllamaAsync(CancellationToken cancellationToken)
    {
        try
        {
            using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(1.5) };
            using var resp = await http.GetAsync($"{OllamaUrl}/api/tags", cancellationToken);
            return resp.IsSuccessStatusCode;
        }
        catch
        {
            return false;
        }
    }

    private void UpdateProgress(int progress, string statusText)
    {
        lock (_sync)
        {
            _downloadProgress = progress;
            _statusText = statusText;
        }
    }

    private static string? FindOllamaExecutable()
    {
        var candidates = new List<string?>
        {
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "Ollama", "ollama.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Ollama", "ollama.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Ollama", "ollama.exe")
        };

        var pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
        foreach (var dir in pathEnv.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
        {
            candidates.Add(Path.Combine(dir.Trim(), "ollama.exe"));
        }

        return candidates.FirstOrDefault(p => !string.IsNullOrWhiteSpace(p) && File.Exists(p));
    }

    private static bool HasManifestOnDisk(string modelName)
    {
        var parts = modelName.Split(':', 2);
        var repo = parts[0];
        var tag = parts.Length > 1 ? parts[1] : "latest";
        var manifest = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
            ".ollama", "models", "manifests", "registry.ollama.ai", "library", repo, tag);
        return File.Exists(manifest);
    }
}
