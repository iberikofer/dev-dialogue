using System.Diagnostics;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace DevDialogue.Api.Services.AI;

public sealed record AvatarServersStatusDto(
    bool Ready,
    bool EmbeddedReady,
    bool Http8080Running,
    bool Tts8085Running,
    bool RhubarbAvailable,
    string StatusText);

public sealed record AvatarVoiceInfo(
    string Id,
    string Name,
    string Lang,
    string Gender,
    string Desc);

public sealed record AvatarWordBoundary(
    double Start,
    double Duration,
    double End,
    string Text);

public sealed record AvatarMouthCue(
    double Start,
    double End,
    string Value);

public sealed record AvatarTtsResponse(
    string Text,
    string Voice,
    double Duration,
    List<AvatarWordBoundary> Words,
    List<AvatarMouthCue> MouthCues,
    string AudioData);

public class AvatarSpeechService
{
    private static readonly IReadOnlyList<AvatarVoiceInfo> CuratedVoices =
    [
        new("uk-UA-OstapNeural", "🇺🇦 Остап (UA, Чол.)", "uk-UA", "Male", "Теплий, виразний чоловічий голос"),
        new("uk-UA-PolinaNeural", "🇺🇦 Поліна (UA, Жін.)", "uk-UA", "Female", "М'який, природний жіночий голос"),
        new("en-US-AvaNeural", "🇺🇸 Ava (EN-US, Жін.)", "en-US", "Female", "Природна розмовна американська вимова"),
        new("en-US-AndrewNeural", "🇺🇸 Andrew (EN-US, Чол.)", "en-US", "Male", "Енергійний, теплий чоловічий тембр"),
        new("en-US-EmmaMultilingualNeural", "🇺🇸 Emma (Мультимовний)", "en-US", "Female", "Універсальний мультимовний жіночий голос"),
        new("en-US-BrianNeural", "🇺🇸 Brian (EN-US, Баритон)", "en-US", "Male", "Глибокий діловий баритон"),
        new("en-GB-SoniaNeural", "🇬🇧 Sonia (EN-GB, Жін.)", "en-GB", "Female", "Витончена британська вимова"),
        new("en-GB-RyanNeural", "🇬🇧 Ryan (EN-GB, Чол.)", "en-GB", "Male", "Сучасний британський чоловічий тембр"),
        new("de-DE-KatjaNeural", "🇩🇪 Katja (DE, Жін.)", "de-DE", "Female", "Німецький природний жіночий голос"),
        new("fr-FR-DeniseNeural", "🇫🇷 Denise (FR, Жін.)", "fr-FR", "Female", "Французький виразний жіночий тембр"),
        new("pl-PL-MarekNeural", "🇵🇱 Marek (PL, Чол.)", "pl-PL", "Male", "Польський чоловічий голос"),
    ];

    private static readonly JsonSerializerOptions JsonOpts = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        PropertyNameCaseInsensitive = true
    };

    private readonly ILogger<AvatarSpeechService> _logger;
    private readonly string _cacheDir;
    private readonly object _serverSync = new();
    private Process? _http8080Process;
    private Process? _tts8085Process;

    public string? AvatarRootPath { get; }
    public string? RhubarbExePath { get; }
    public string PythonExePath { get; }
    public string FfmpegExePath { get; }

    public AvatarSpeechService(ILogger<AvatarSpeechService> logger)
    {
        _logger = logger;
        _cacheDir = Path.Combine(Path.GetTempPath(), "dev-dialogue-tts");
        Directory.CreateDirectory(_cacheDir);

        AvatarRootPath = ResolveAvatarRoot();
        RhubarbExePath = ResolveRhubarbExe(AvatarRootPath);
        PythonExePath = ResolvePythonExe(AvatarRootPath);
        FfmpegExePath = ResolveFfmpegExe();
    }

    public AvatarServersStatusDto Get3DServersStatus()
    {
        var embeddedReady = !string.IsNullOrEmpty(AvatarRootPath)
                            && File.Exists(Path.Combine(AvatarRootPath, "model.html"));
        var rhubarbReady = !string.IsNullOrEmpty(RhubarbExePath) && File.Exists(RhubarbExePath);
        var p8080 = IsLocalPortOpen(8080);
        var p8085 = IsLocalPortOpen(8085);
        var ready = embeddedReady && (p8085 || rhubarbReady);

        var statusText = ready
            ? (p8080 && p8085
                ? "3D Avatar (:5238/avatar + :8080 + :8085 TTS/Rhubarb) активні"
                : "3D Avatar (:5238/avatar + Edge-TTS/Rhubarb) активний")
            : (!embeddedReady ? "Файли 3D-моделі не знайдено" : "3D-сервери очікують запуску");

        return new AvatarServersStatusDto(
            Ready: ready,
            EmbeddedReady: embeddedReady,
            Http8080Running: p8080,
            Tts8085Running: p8085,
            RhubarbAvailable: rhubarbReady,
            StatusText: statusText);
    }

    public async Task<AvatarServersStatusDto> Ensure3DServersRunningAsync(
        bool forceRestart = false,
        CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrEmpty(AvatarRootPath) || !Directory.Exists(AvatarRootPath))
        {
            return Get3DServersStatus();
        }

        if (forceRestart)
        {
            lock (_serverSync)
            {
                TryStopProcess(ref _http8080Process);
                TryStopProcess(ref _tts8085Process);
            }
            await Task.Delay(250, cancellationToken);
        }

        var p8080 = IsLocalPortOpen(8080);
        if (!p8080)
        {
            lock (_serverSync)
            {
                if (_http8080Process is null || _http8080Process.HasExited)
                {
                    try
                    {
                        _http8080Process = Process.Start(new ProcessStartInfo
                        {
                            FileName = PythonExePath,
                            Arguments = "-m http.server 8080",
                            WorkingDirectory = AvatarRootPath,
                            UseShellExecute = false,
                            CreateNoWindow = true,
                            WindowStyle = ProcessWindowStyle.Hidden
                        });
                    }
                    catch (Exception ex)
                    {
                        _logger.LogWarning(ex, "Could not start 3D Face http.server on 8080");
                    }
                }
            }
        }

        var p8085 = IsLocalPortOpen(8085);
        var ttsScript = Path.Combine(AvatarRootPath, "tools", "tts_backend.py");
        if (!p8085 && File.Exists(ttsScript))
        {
            lock (_serverSync)
            {
                if (_tts8085Process is null || _tts8085Process.HasExited)
                {
                    try
                    {
                        _tts8085Process = Process.Start(new ProcessStartInfo
                        {
                            FileName = PythonExePath,
                            Arguments = $"\"{ttsScript}\" 8085",
                            WorkingDirectory = AvatarRootPath,
                            UseShellExecute = false,
                            CreateNoWindow = true,
                            WindowStyle = ProcessWindowStyle.Hidden
                        });
                    }
                    catch (Exception ex)
                    {
                        _logger.LogWarning(ex, "Could not start 3D Face tts_backend.py on 8085");
                    }
                }
            }
        }

        for (int i = 0; i < 8; i++)
        {
            if (IsLocalPortOpen(8080) && IsLocalPortOpen(8085))
            {
                break;
            }
            await Task.Delay(200, cancellationToken);
        }

        return Get3DServersStatus();
    }

    private static bool IsLocalPortOpen(int port)
    {
        try
        {
            using var client = new TcpClient();
            var connectTask = client.ConnectAsync("127.0.0.1", port);
            return connectTask.Wait(180) && client.Connected;
        }
        catch
        {
            return false;
        }
    }

    private static void TryStopProcess(ref Process? proc)
    {
        if (proc is null) return;
        try
        {
            if (!proc.HasExited)
            {
                proc.Kill(entireProcessTree: true);
                proc.WaitForExit(600);
            }
        }
        catch
        {
        }
        finally
        {
            proc.Dispose();
            proc = null;
        }
    }

    public IReadOnlyList<AvatarVoiceInfo> GetVoices() => CuratedVoices;

    public async Task<AvatarTtsResponse> SynthesizeAsync(
        string text,
        string? voice,
        string? rate,
        string? pitch,
        CancellationToken cancellationToken = default)
    {
        var cleanText = (text ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(cleanText))
            throw new ArgumentException("Text cannot be empty.", nameof(text));

        var resolvedVoice = string.IsNullOrWhiteSpace(voice) ? "uk-UA-OstapNeural" : voice.Trim();
        var resolvedRate = string.IsNullOrWhiteSpace(rate) ? "+0%" : rate.Trim();
        var resolvedPitch = string.IsNullOrWhiteSpace(pitch) ? "+0Hz" : pitch.Trim();

        var hashBytes = MD5.HashData(Encoding.UTF8.GetBytes($"{cleanText}_{resolvedVoice}_{resolvedRate}_{resolvedPitch}"));
        var hashKey = Convert.ToHexString(hashBytes).ToLowerInvariant()[..12];

        var cachedJsonPath = Path.Combine(_cacheDir, $"tts_{hashKey}_cache.json");
        if (File.Exists(cachedJsonPath))
        {
            try
            {
                var cachedJson = await File.ReadAllTextAsync(cachedJsonPath, cancellationToken);
                var cached = JsonSerializer.Deserialize<AvatarTtsResponse>(cachedJson, JsonOpts);
                if (cached is not null && !string.IsNullOrEmpty(cached.AudioData))
                {
                    return cached;
                }
            }
            catch
            {
                // Regenerate below if cache is corrupted.
            }
        }

        var mp3Path = Path.Combine(_cacheDir, $"tts_{hashKey}.mp3");
        var wordsPath = Path.Combine(_cacheDir, $"tts_{hashKey}_words.json");
        var wavPath = Path.Combine(_cacheDir, $"tts_{hashKey}.wav");
        var rhubarbJsonPath = Path.Combine(_cacheDir, $"tts_{hashKey}_rhubarb.json");
        var scriptPath = Path.Combine(_cacheDir, "edge_tts_helper.py");

        EnsurePythonHelperScript(scriptPath);

        // Step 1: Synthesize speech MP3 + word boundary timestamps via edge-tts
        var pyExit = await RunProcessAsync(
            PythonExePath,
            [scriptPath, cleanText, resolvedVoice, resolvedRate, resolvedPitch, mp3Path, wordsPath],
            TimeSpan.FromSeconds(30),
            cancellationToken);

        if (pyExit.ExitCode != 0 || !File.Exists(mp3Path))
        {
            throw new InvalidOperationException($"edge-tts failed (exit {pyExit.ExitCode}): {pyExit.StdErr}");
        }

        var words = new List<AvatarWordBoundary>();
        if (File.Exists(wordsPath))
        {
            try
            {
                var rawWords = await File.ReadAllTextAsync(wordsPath, cancellationToken);
                words = JsonSerializer.Deserialize<List<AvatarWordBoundary>>(rawWords, JsonOpts) ?? [];
            }
            catch (Exception ex)
            {
                _logger.LogWarning(ex, "Failed to deserialize word boundaries.");
            }
        }

        // Step 2: Convert MP3 to 16kHz mono WAV and analyze mouth shapes with Rhubarb
        var mouthCues = new List<AvatarMouthCue>();
        if (!string.IsNullOrEmpty(RhubarbExePath) && File.Exists(RhubarbExePath))
        {
            var ff = await RunProcessAsync(
                FfmpegExePath,
                ["-y", "-i", mp3Path, "-ar", "16000", "-ac", "1", wavPath],
                TimeSpan.FromSeconds(20),
                cancellationToken);

            if (ff.ExitCode == 0 && File.Exists(wavPath))
            {
                var rh = await RunProcessAsync(
                    RhubarbExePath,
                    ["-r", "phonetic", "-f", "json", "--extendedShapes", "GHX", "-o", rhubarbJsonPath, wavPath],
                    TimeSpan.FromSeconds(25),
                    cancellationToken);

                if (rh.ExitCode == 0 && File.Exists(rhubarbJsonPath))
                {
                    try
                    {
                        using var doc = JsonDocument.Parse(await File.ReadAllTextAsync(rhubarbJsonPath, cancellationToken));
                        if (doc.RootElement.TryGetProperty("mouthCues", out var cuesEl) &&
                            cuesEl.ValueKind == JsonValueKind.Array)
                        {
                            foreach (var cue in cuesEl.EnumerateArray())
                            {
                                var start = cue.TryGetProperty("start", out var sEl) ? sEl.GetDouble() : 0.0;
                                var end = cue.TryGetProperty("end", out var eEl) ? eEl.GetDouble() : 0.0;
                                var val = cue.TryGetProperty("value", out var vEl) ? (vEl.GetString() ?? "X") : "X";
                                mouthCues.Add(new AvatarMouthCue(start, end, val));
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        _logger.LogWarning(ex, "Failed to parse Rhubarb mouthCues.");
                    }
                }
            }
        }

        var duration = 0.0;
        if (words.Count > 0)
            duration = words.Max(w => w.End) + 0.25;
        if (mouthCues.Count > 0)
            duration = Math.Max(duration, mouthCues.Max(c => c.End));
        if (duration <= 0.1)
            duration = Math.Max(1.5, cleanText.Length * 0.065);

        var mp3Bytes = await File.ReadAllBytesAsync(mp3Path, cancellationToken);
        var result = new AvatarTtsResponse(
            cleanText,
            resolvedVoice,
            Math.Round(duration, 3),
            words,
            mouthCues,
            "data:audio/mp3;base64," + Convert.ToBase64String(mp3Bytes));

        try
        {
            await File.WriteAllTextAsync(cachedJsonPath, JsonSerializer.Serialize(result, JsonOpts), cancellationToken);
        }
        catch
        {
            // Cache write failures are non-fatal.
        }

        return result;
    }

    private static void EnsurePythonHelperScript(string scriptPath)
    {
        const string helperSource = """
import asyncio
import json
import sys
import edge_tts

async def main():
    text, voice, rate, pitch, mp3_out, words_out = sys.argv[1:7]
    comm = edge_tts.Communicate(text, voice, rate=rate, pitch=pitch, boundary="WordBoundary")
    words = []
    audio = bytearray()
    async for chunk in comm.stream():
        if chunk["type"] == "audio":
            audio.extend(chunk["data"])
        elif chunk["type"] == "WordBoundary":
            words.append({
                "start": round(chunk["offset"] / 10000000.0, 3),
                "duration": round(chunk["duration"] / 10000000.0, 3),
                "end": round((chunk["offset"] + chunk["duration"]) / 10000000.0, 3),
                "text": chunk["text"],
            })
    with open(mp3_out, "wb") as f:
        f.write(audio)
    with open(words_out, "w", encoding="utf-8") as f:
        json.dump(words, f, ensure_ascii=False)

if __name__ == "__main__":
    asyncio.run(main())
""";
        if (!File.Exists(scriptPath) || File.ReadAllText(scriptPath, Encoding.UTF8) != helperSource)
        {
            File.WriteAllText(scriptPath, helperSource, Encoding.UTF8);
        }
    }

    private static async Task<(int ExitCode, string StdOut, string StdErr)> RunProcessAsync(
        string fileName,
        IReadOnlyList<string> args,
        TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        var psi = new ProcessStartInfo
        {
            FileName = fileName,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8
        };
        foreach (var arg in args)
        {
            psi.ArgumentList.Add(arg);
        }

        using var proc = Process.Start(psi)
            ?? throw new InvalidOperationException($"Failed to start process '{fileName}'.");

        using var linked = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        linked.CancelAfter(timeout);

        var stdOutTask = proc.StandardOutput.ReadToEndAsync(linked.Token);
        var stdErrTask = proc.StandardError.ReadToEndAsync(linked.Token);

        await proc.WaitForExitAsync(linked.Token);
        var stdout = await stdOutTask;
        var stderr = await stdErrTask;
        return (proc.ExitCode, stdout, stderr);
    }

    private static string? ResolveAvatarRoot()
    {
        var roots = new[] { Directory.GetCurrentDirectory(), AppContext.BaseDirectory };
        foreach (var start in roots)
        {
            var dir = new DirectoryInfo(start);
            for (var i = 0; i < 7 && dir is not null; i++)
            {
                var candidate = Path.Combine(dir.FullName, "3D Face Import");
                if (Directory.Exists(candidate) && File.Exists(Path.Combine(candidate, "model.html")))
                    return candidate;
                dir = dir.Parent;
            }
        }
        return null;
    }

    private static string? ResolveRhubarbExe(string? avatarRoot)
    {
        if (string.IsNullOrEmpty(avatarRoot)) return null;
        var direct = Path.Combine(avatarRoot, "tools", "rhubarb", "Rhubarb-Lip-Sync-1.14.0-Windows", "rhubarb.exe");
        if (File.Exists(direct)) return direct;

        var toolsDir = Path.Combine(avatarRoot, "tools", "rhubarb");
        if (Directory.Exists(toolsDir))
        {
            return Directory.EnumerateFiles(toolsDir, "rhubarb.exe", SearchOption.AllDirectories).FirstOrDefault();
        }
        return null;
    }

    private static string ResolvePythonExe(string? avatarRoot)
    {
        var candidates = new List<string?>
        {
            Environment.GetEnvironmentVariable("DEVDIALOGUE_TTS_PYTHON"),
            @"B:\PROJECTS\models\3dface\.venv\Scripts\python.exe",
            avatarRoot is not null ? Path.Combine(avatarRoot, ".venv", "Scripts", "python.exe") : null
        };
        foreach (var c in candidates)
        {
            if (!string.IsNullOrWhiteSpace(c) && File.Exists(c))
                return c;
        }
        return "python";
    }

    private static string ResolveFfmpegExe()
    {
        var env = Environment.GetEnvironmentVariable("FFMPEG_PATH");
        if (!string.IsNullOrWhiteSpace(env) && File.Exists(env)) return env;

        var localBin = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
            ".local", "bin", "ffmpeg.exe");
        if (File.Exists(localBin)) return localBin;

        return "ffmpeg";
    }
}
