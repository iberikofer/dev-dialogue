using System.Buffers.Binary;
using System.Diagnostics;
using SherpaOnnx;

namespace DevDialogue.Api.Services.AI;

public sealed record SttTranscriptionResult(
    string Text,
    double AudioSeconds,
    long LatencyMs,
    string Model);

/// <summary>
/// Local speech-to-text powered by NVIDIA Parakeet TDT 0.6B v3 (INT8 ONNX via SherpaOnnx).
/// Thread-safe lazy initialization keeps startup instant until the first microphone request arrives.
/// </summary>
public sealed class ParakeetSpeechToTextService : IDisposable
{
    private readonly ILogger<ParakeetSpeechToTextService> _logger;
    private readonly SemaphoreSlim _sync = new(1, 1);
    private OfflineRecognizer? _recognizer;
    private bool _disposed;

    private string? _modelDirectory;
    public string? ModelDirectory => _modelDirectory ??= ResolveModelDirectory();
    public bool IsModelAvailable => !string.IsNullOrEmpty(ModelDirectory);
    public bool IsInitialized => _recognizer is not null;
    public string ModelName => "parakeet-tdt-0.6b-v3";

    public static string DefaultInstallDirectory => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "DevDialogue", "models", "parakeet-tdt-0.6b");

    public async Task<bool> EnsureReadyAsync(bool forceReload = false, CancellationToken cancellationToken = default)
    {
        if (!IsModelAvailable || ModelDirectory is null)
        {
            return false;
        }

        await _sync.WaitAsync(cancellationToken);
        try
        {
            if (forceReload && _recognizer is not null)
            {
                _recognizer.Dispose();
                _recognizer = null;
            }
            EnsureRecognizerInitialized();
            return _recognizer is not null;
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Failed to initialize Parakeet TDT 0.6B v3 recognizer");
            return false;
        }
        finally
        {
            _sync.Release();
        }
    }

    public ParakeetSpeechToTextService(ILogger<ParakeetSpeechToTextService> logger)
    {
        _logger = logger;
        _modelDirectory = ResolveModelDirectory();
        if (_modelDirectory is not null)
        {
            _logger.LogInformation("Parakeet TDT 0.6B v3 model found at: {Path}", _modelDirectory);
        }
        else
        {
            _logger.LogWarning("Parakeet TDT 0.6B v3 model directory not found.");
        }
    }

    public async Task<SttTranscriptionResult> TranscribeWavAsync(
        byte[] wavBytes,
        CancellationToken cancellationToken = default)
    {
        if (!IsModelAvailable || ModelDirectory is null)
        {
            throw new InvalidOperationException(
                "Parakeet TDT 0.6B v3 model files (encoder/decoder/joiner/tokens) were not found.");
        }

        var (sampleRate, samples) = ParseWavToFloatSamples(wavBytes);
        if (samples.Length == 0 || sampleRate <= 0)
        {
            return new SttTranscriptionResult(string.Empty, 0.0, 0, ModelName);
        }

        var durationSec = Math.Round((double)samples.Length / sampleRate, 2);
        var sw = Stopwatch.StartNew();

        await _sync.WaitAsync(cancellationToken);
        try
        {
            EnsureRecognizerInitialized();
            using var stream = _recognizer!.CreateStream();
            stream.AcceptWaveform(sampleRate, samples);
            _recognizer.Decode(stream);
            var text = (stream.Result?.Text ?? string.Empty).Trim();
            sw.Stop();

            DebugTraceLog.Add(
                "STT Parakeet",
                $"{sw.ElapsedMilliseconds}ms ({durationSec:F1}s audio) -> \"{Truncate(text, 70)}\"");

            return new SttTranscriptionResult(text, durationSec, sw.ElapsedMilliseconds, ModelName);
        }
        finally
        {
            _sync.Release();
        }
    }

    private void EnsureRecognizerInitialized()
    {
        if (_recognizer is not null)
        {
            return;
        }

        var encoder = Path.Combine(ModelDirectory!, "encoder.int8.onnx");
        var decoder = Path.Combine(ModelDirectory!, "decoder.int8.onnx");
        var joiner = Path.Combine(ModelDirectory!, "joiner.int8.onnx");
        var tokens = Path.Combine(ModelDirectory!, "tokens.txt");

        var config = new OfflineRecognizerConfig();
        config.FeatConfig.SampleRate = 16000;
        config.FeatConfig.FeatureDim = 80;

        config.ModelConfig.Transducer.Encoder = encoder;
        config.ModelConfig.Transducer.Decoder = decoder;
        config.ModelConfig.Transducer.Joiner = joiner;
        config.ModelConfig.Tokens = tokens;
        config.ModelConfig.ModelType = "nemo_transducer";
        config.ModelConfig.Provider = "cpu";
        config.ModelConfig.NumThreads = Math.Clamp(Environment.ProcessorCount / 2, 2, 6);
        config.ModelConfig.Debug = 0;

        config.DecodingMethod = "greedy_search";
        config.MaxActivePaths = 4;

        var sw = Stopwatch.StartNew();
        _recognizer = new OfflineRecognizer(config);
        sw.Stop();
        _logger.LogInformation("Loaded Parakeet TDT 0.6B v3 recognizer in {Ms}ms", sw.ElapsedMilliseconds);
        DebugTraceLog.Add("STT Init", $"Loaded Parakeet TDT 0.6B v3 in {sw.ElapsedMilliseconds}ms");
    }

    private static (int SampleRate, float[] Samples) ParseWavToFloatSamples(byte[] wavBytes)
    {
        if (wavBytes.Length < 44)
        {
            throw new InvalidDataException("Audio payload is too short to be a valid WAV file.");
        }

        var span = wavBytes.AsSpan();
        if (!span[..4].SequenceEqual("RIFF"u8) || !span.Slice(8, 4).SequenceEqual("WAVE"u8))
        {
            throw new InvalidDataException("Expected RIFF/WAVE audio data.");
        }

        ushort audioFormat = 1;
        ushort channels = 1;
        int sampleRate = 16000;
        ushort bitsPerSample = 16;
        int dataOffset = -1;
        int dataLength = 0;

        int pos = 12;
        while (pos + 8 <= span.Length)
        {
            var chunkId = span.Slice(pos, 4);
            var chunkSize = BinaryPrimitives.ReadInt32LittleEndian(span.Slice(pos + 4, 4));
            var payloadStart = pos + 8;

            if (chunkSize < 0 || payloadStart > span.Length)
            {
                break;
            }

            if (chunkId.SequenceEqual("fmt "u8) && payloadStart + 16 <= span.Length)
            {
                audioFormat = BinaryPrimitives.ReadUInt16LittleEndian(span.Slice(payloadStart, 2));
                channels = BinaryPrimitives.ReadUInt16LittleEndian(span.Slice(payloadStart + 2, 2));
                sampleRate = BinaryPrimitives.ReadInt32LittleEndian(span.Slice(payloadStart + 4, 4));
                bitsPerSample = BinaryPrimitives.ReadUInt16LittleEndian(span.Slice(payloadStart + 14, 2));
            }
            else if (chunkId.SequenceEqual("data"u8))
            {
                dataOffset = payloadStart;
                dataLength = Math.Min(chunkSize, span.Length - payloadStart);
                break;
            }

            pos = payloadStart + chunkSize + (chunkSize & 1);
        }

        if (dataOffset < 0 || dataLength <= 0 || channels == 0)
        {
            return (sampleRate, Array.Empty<float>());
        }

        var dataSpan = span.Slice(dataOffset, dataLength);
        var bytesPerSample = Math.Max(1, bitsPerSample / 8);
        var bytesPerFrame = bytesPerSample * channels;
        var frameCount = dataLength / bytesPerFrame;
        var samples = new float[frameCount];

        if (audioFormat == 1 && bitsPerSample == 16)
        {
            for (int i = 0; i < frameCount; i++)
            {
                float sum = 0f;
                int baseOff = i * bytesPerFrame;
                for (int ch = 0; ch < channels; ch++)
                {
                    short s = BinaryPrimitives.ReadInt16LittleEndian(dataSpan.Slice(baseOff + ch * 2, 2));
                    sum += s / 32768f;
                }
                samples[i] = sum / channels;
            }
        }
        else if (audioFormat == 3 && bitsPerSample == 32)
        {
            for (int i = 0; i < frameCount; i++)
            {
                float sum = 0f;
                int baseOff = i * bytesPerFrame;
                for (int ch = 0; ch < channels; ch++)
                {
                    sum += BinaryPrimitives.ReadSingleLittleEndian(dataSpan.Slice(baseOff + ch * 4, 4));
                }
                samples[i] = sum / channels;
            }
        }
        else
        {
            throw new NotSupportedException($"Unsupported WAV format {audioFormat} ({bitsPerSample}-bit).");
        }

        return (sampleRate, samples);
    }

    private static string? ResolveModelDirectory()
    {
        var candidates = new List<string?>
        {
            Environment.GetEnvironmentVariable("DEVDIALOGUE_STT_MODEL_DIR"),
            DefaultInstallDirectory,
            Path.Combine(AppContext.BaseDirectory, "models", "parakeet-tdt-0.6b"),
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "TypeWhisper", "PluginData", "com.typewhisper.sherpa-onnx", "Models", "parakeet-tdt-0.6b")
        };

        var dir = new DirectoryInfo(Directory.GetCurrentDirectory());
        for (int i = 0; i < 6 && dir is not null; i++)
        {
            candidates.Add(Path.Combine(dir.FullName, "models", "parakeet-tdt-0.6b"));
            dir = dir.Parent;
        }

        foreach (var candidate in candidates)
        {
            if (string.IsNullOrWhiteSpace(candidate) || !Directory.Exists(candidate))
            {
                continue;
            }

            if (File.Exists(Path.Combine(candidate, "encoder.int8.onnx")) &&
                File.Exists(Path.Combine(candidate, "decoder.int8.onnx")) &&
                File.Exists(Path.Combine(candidate, "joiner.int8.onnx")) &&
                File.Exists(Path.Combine(candidate, "tokens.txt")))
            {
                return candidate;
            }
        }

        return null;
    }

    private static string Truncate(string s, int maxLen)
        => s.Length <= maxLen ? s : s[..maxLen] + "...";

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _recognizer?.Dispose();
        _recognizer = null;
        _sync.Dispose();
    }
}
