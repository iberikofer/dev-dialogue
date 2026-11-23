using System.Collections.Concurrent;
using System.Diagnostics;
using System.Net.Http.Json;
using System.Text.Json;
using System.Text.Json.Serialization;
using DevDialogue.Api.Models.Dtos;

namespace DevDialogue.Api.Services.AI;

public sealed record VisionAnalyzeRequest(
    [property: JsonPropertyName("image_base64")] string ImageBase64,
    [property: JsonPropertyName("interview_id")] string? InterviewId = null,
    [property: JsonPropertyName("language")] string Language = "uk",
    [property: JsonPropertyName("instant_score")] int? InstantScore = null,
    [property: JsonPropertyName("instant_lighting")] int? InstantLighting = null,
    [property: JsonPropertyName("instant_presence")] int? InstantPresence = null,
    [property: JsonPropertyName("instant_hands")] int? InstantHands = null,
    [property: JsonPropertyName("instant_posture")] int? InstantPosture = null,
    [property: JsonPropertyName("instant_emotion_score")] int? InstantEmotionScore = null,
    [property: JsonPropertyName("instant_emotion")] string? InstantEmotion = null,
    [property: JsonPropertyName("include_in_score")] bool IncludeInScore = true
);

public sealed record VisionAnalyzeResponse(
    [property: JsonPropertyName("overall_score")] int OverallScore,
    [property: JsonPropertyName("presence_score")] int PresenceScore,
    [property: JsonPropertyName("hands_score")] int HandsScore,
    [property: JsonPropertyName("lighting_score")] int LightingScore,
    [property: JsonPropertyName("appearance_score")] int AppearanceScore,
    [property: JsonPropertyName("posture_score")] int PostureScore,
    [property: JsonPropertyName("emotion_score")] int EmotionScore,
    [property: JsonPropertyName("emotion")] string Emotion,
    [property: JsonPropertyName("person_present")] bool PersonPresent,
    [property: JsonPropertyName("summary")] string Summary,
    [property: JsonPropertyName("model")] string Model,
    [property: JsonPropertyName("latency_ms")] long LatencyMs,
    [property: JsonPropertyName("samples_analyzed")] int SamplesAnalyzed
);

/// <summary>
/// Thread-safe accumulator of per-interview VLM+CV visual samples so the final report
/// and per-turn evaluation can include accurate presence, lighting, hands, posture and emotion metrics.
/// </summary>
public sealed class VisualSessionAggregator
{
    private sealed class Bucket
    {
        public readonly object Sync = new();
        public int Count;
        public long SumOverall;
        public long SumPresence;
        public long SumHands;
        public long SumLighting;
        public long SumAppearance;
        public long SumPosture;
        public long SumEmotion;
        public readonly Dictionary<string, int> EmotionCounts = new(StringComparer.OrdinalIgnoreCase);
        public VisionAnalyzeResponse? LatestSample;
        public bool IncludeInScore = true;
        public string LastSummary = "";
        public string LastModel = "gemma4:e4b";
    }

    private readonly ConcurrentDictionary<string, Bucket> _buckets = new(StringComparer.OrdinalIgnoreCase);

    public int Record(string? interviewId, VisionAnalyzeResponse sample, bool includeInScore = true)
    {
        if (string.IsNullOrWhiteSpace(interviewId))
        {
            return 1;
        }
        var bucket = _buckets.GetOrAdd(interviewId.Trim(), _ => new Bucket());
        lock (bucket.Sync)
        {
            bucket.Count++;
            bucket.SumOverall += sample.OverallScore;
            bucket.SumPresence += sample.PresenceScore;
            bucket.SumHands += sample.HandsScore;
            bucket.SumLighting += sample.LightingScore;
            bucket.SumAppearance += sample.AppearanceScore;
            bucket.SumPosture += sample.PostureScore;
            bucket.SumEmotion += sample.EmotionScore;
            bucket.IncludeInScore = includeInScore;
            bucket.LatestSample = sample;

            var emo = string.IsNullOrWhiteSpace(sample.Emotion) ? "neutral" : sample.Emotion.Trim().ToLowerInvariant();
            bucket.EmotionCounts[emo] = bucket.EmotionCounts.GetValueOrDefault(emo) + 1;

            if (!string.IsNullOrWhiteSpace(sample.Summary))
            {
                bucket.LastSummary = sample.Summary;
            }
            bucket.LastModel = sample.Model;
            return bucket.Count;
        }
    }

    public VisionAnalyzeResponse? GetLatestSample(string? interviewId)
    {
        if (string.IsNullOrWhiteSpace(interviewId) ||
            !_buckets.TryGetValue(interviewId.Trim(), out var bucket))
        {
            return null;
        }
        lock (bucket.Sync)
        {
            return bucket.LatestSample;
        }
    }

    public VisualCriteriaDto? GetAggregate(string? interviewId)
    {
        if (string.IsNullOrWhiteSpace(interviewId) ||
            !_buckets.TryGetValue(interviewId.Trim(), out var bucket))
        {
            return null;
        }
        lock (bucket.Sync)
        {
            if (bucket.Count == 0)
            {
                return null;
            }
            int c = bucket.Count;
            var dominantEmotion = bucket.EmotionCounts.Count > 0
                ? bucket.EmotionCounts.MaxBy(kv => kv.Value).Key
                : "neutral";

            return new VisualCriteriaDto(
                OverallScore: (int)Math.Round((double)bucket.SumOverall / c),
                PresenceScore: (int)Math.Round((double)bucket.SumPresence / c),
                HandsScore: (int)Math.Round((double)bucket.SumHands / c),
                LightingScore: (int)Math.Round((double)bucket.SumLighting / c),
                AppearanceScore: (int)Math.Round((double)bucket.SumAppearance / c),
                PostureScore: (int)Math.Round((double)bucket.SumPosture / c),
                EmotionScore: (int)Math.Round((double)bucket.SumEmotion / c),
                DominantEmotion: dominantEmotion,
                IncludedInScore: bucket.IncludeInScore,
                Summary: bucket.LastSummary,
                Model: $"{bucket.LastModel} + 30 FPS CV",
                SamplesAnalyzed: c
            );
        }
    }
}

/// <summary>
/// Local Ollama vision evaluator backed by the already-loaded lfm3-omni:2b model
/// fused with 30 FPS C++ computer-vision telemetry (skin/face geometry, lighting histogram,
/// motion stability, posture framing, and facial expression/emotion).
/// </summary>
public sealed class LocalVisionService
{
    private static readonly SemaphoreSlim InferenceLock = new(1, 1);

    private readonly HttpClient _http;
    private readonly IConfiguration _config;
    private readonly ILogger<LocalVisionService> _logger;
    private readonly VisualSessionAggregator _aggregator;

    public string VisionModelName =>
        _config["AI:VisionModel"]
        ?? Environment.GetEnvironmentVariable("OLLAMA_VISION_MODEL")
        ?? "gemma4:e4b";

    private string OllamaBaseUrl =>
        (_config["AI:OllamaUrl"]
         ?? Environment.GetEnvironmentVariable("OLLAMA_URL")
         ?? "http://127.0.0.1:11434").TrimEnd('/');

    public LocalVisionService(
        HttpClient http,
        IConfiguration config,
        ILogger<LocalVisionService> logger,
        VisualSessionAggregator aggregator)
    {
        _http = http;
        _http.Timeout = TimeSpan.FromSeconds(60);
        _config = config;
        _logger = logger;
        _aggregator = aggregator;
    }

    public async Task<bool> IsVisionModelAvailableAsync(CancellationToken ct = default)
    {
        try
        {
            using var resp = await _http.GetAsync($"{OllamaBaseUrl}/api/tags", ct);
            if (!resp.IsSuccessStatusCode) return false;
            var json = await resp.Content.ReadAsStringAsync(ct);
            return json.Contains(VisionModelName, StringComparison.OrdinalIgnoreCase);
        }
        catch
        {
            return false;
        }
    }

    public async Task<VisionAnalyzeResponse> AnalyzeFrameAsync(
        VisionAnalyzeRequest request,
        CancellationToken ct = default)
    {
        var sw = Stopwatch.StartNew();
        var lang = string.Equals(request.Language, "en", StringComparison.OrdinalIgnoreCase) ? "en" : "uk";

        var base64Clean = request.ImageBase64.Trim();
        var commaIdx = base64Clean.IndexOf(',');
        if (commaIdx >= 0)
        {
            base64Clean = base64Clean[(commaIdx + 1)..];
        }

        // Fast short-circuit only if frame is pitch-black/covered (CV presence < 12)
        if (request.InstantPresence.HasValue && request.InstantPresence.Value < 12)
        {
            sw.Stop();
            var absentResp = BuildAbsentResponse(request, sw.ElapsedMilliseconds, lang);
            var count = _aggregator.Record(request.InterviewId, absentResp, request.IncludeInScore);
            DebugTraceLog.Add(
                $"Vision {VisionModelName}",
                $"Absent in frame (CV presence={request.InstantPresence.Value}) -> Score={absentResp.OverallScore}/100");
            return absentResp with { SamplesAnalyzed = count };
        }

        var cvHint = $"CV telemetry: presence={request.InstantPresence ?? 70}, lighting={request.InstantLighting ?? 75}, posture={request.InstantPosture ?? 75}. ";
        var prompt = lang == "uk"
            ? cvHint +
              "Analyze this real webcam frame of an interview candidate. " +
              "CRITICAL RULE: If no real human face with eyes/nose/mouth is clearly visible (e.g. empty room, chair, wall, covered camera, or abstract drawing/shape), set person_present=false, presence_score=0, emotion=\"absent\", emotion_score=0. " +
              "If a real person IS visible, score 0..100 for: presence_score (face & upper body visibility), hands_score (natural hand position/gestures, not hiding face or holding phone), lighting_score (even front lighting on face, contrast, no harsh backlight), appearance_score (neat interview attire & grooming), posture_score (centered framing, upright posture, eye contact with camera), emotion (\"confident\"|\"smile\"|\"focused\"|\"neutral\"|\"tense\"|\"distracted\"|\"absent\"), emotion_score (0..100 composure/positivity), and summary (1 short diagnostic sentence in Ukrainian mentioning presence, lighting and emotion). Return ONLY valid JSON."
            : cvHint +
              "Analyze this real webcam frame of an interview candidate. " +
              "CRITICAL RULE: If no real human face with eyes/nose/mouth is clearly visible (e.g. empty room, chair, wall, covered camera, or abstract drawing/shape), set person_present=false, presence_score=0, emotion=\"absent\", emotion_score=0. " +
              "If a real person IS visible, score 0..100 for: presence_score, hands_score, lighting_score, appearance_score, posture_score, emotion (\"confident\"|\"smile\"|\"focused\"|\"neutral\"|\"tense\"|\"distracted\"|\"absent\"), emotion_score (0..100), and summary (1 short diagnostic sentence in English). Return ONLY valid JSON.";

        var payload = new
        {
            model = VisionModelName,
            prompt,
            images = new[] { base64Clean },
            stream = false,
            format = "json",
            keep_alive = "15m",
            options = new
            {
                num_predict = 280,
                temperature = 0.1
            }
        };

        await InferenceLock.WaitAsync(ct);
        try
        {
            using var resp = await _http.PostAsJsonAsync($"{OllamaBaseUrl}/api/generate", payload, ct);
            resp.EnsureSuccessStatusCode();

            using var doc = await JsonDocument.ParseAsync(await resp.Content.ReadAsStreamAsync(ct), cancellationToken: ct);
            var rawText = doc.RootElement.TryGetProperty("response", out var rEl)
                ? (rEl.GetString() ?? "{}")
                : "{}";

            var parsed = ParseAndFuseVisionJson(rawText, request, lang);
            sw.Stop();

            var withTiming = parsed with { LatencyMs = sw.ElapsedMilliseconds };
            var samples = _aggregator.Record(request.InterviewId, withTiming, request.IncludeInScore);
            var finalResp = withTiming with { SamplesAnalyzed = samples };

            DebugTraceLog.Add(
                $"Vision {VisionModelName}",
                $"Score={finalResp.OverallScore} (pres={finalResp.PresenceScore}, light={finalResp.LightingScore}, emo={finalResp.Emotion}:{finalResp.EmotionScore}, posture={finalResp.PostureScore}) {sw.ElapsedMilliseconds}ms");

            return finalResp;
        }
        catch (Exception ex) when (!ct.IsCancellationRequested)
        {
            _logger.LogDebug(ex, "lfm3-omni:2b vision analysis fallback");
            sw.Stop();
            var fallback = BuildInstantFallback(request, sw.ElapsedMilliseconds, lang);
            var samples = _aggregator.Record(request.InterviewId, fallback, request.IncludeInScore);
            return fallback with { SamplesAnalyzed = samples };
        }
        finally
        {
            InferenceLock.Release();
        }
    }

    private VisionAnalyzeResponse ParseAndFuseVisionJson(string rawJson, VisionAnalyzeRequest req, string lang)
    {
        try
        {
            var start = rawJson.IndexOf('{');
            var end = rawJson.LastIndexOf('}');
            if (start >= 0 && end > start)
            {
                rawJson = rawJson[start..(end + 1)];
            }

            using var doc = JsonDocument.Parse(rawJson);
            var root = doc.RootElement;

            bool vlmPresent = true;
            if (root.TryGetProperty("person_present", out var pEl))
            {
                if (pEl.ValueKind is JsonValueKind.True or JsonValueKind.False)
                    vlmPresent = pEl.GetBoolean();
                else if (pEl.ValueKind == JsonValueKind.String && bool.TryParse(pEl.GetString(), out var bp))
                    vlmPresent = bp;
            }

            int ReadScore(string key, int fallbackVal)
            {
                if (root.TryGetProperty(key, out var el))
                {
                    if (el.ValueKind == JsonValueKind.Number && el.TryGetInt32(out int v))
                        return Math.Clamp(v, 0, 100);
                    if (el.ValueKind == JsonValueKind.String && int.TryParse(el.GetString(), out int sv))
                        return Math.Clamp(sv, 0, 100);
                }
                return Math.Clamp(fallbackVal, 0, 100);
            }

            var rawSummary = root.TryGetProperty("summary", out var sEl) && sEl.ValueKind == JsonValueKind.String
                ? (sEl.GetString() ?? "").Trim()
                : "";
            var rawEmotion = root.TryGetProperty("emotion", out var eEl) && eEl.ValueKind == JsonValueKind.String
                ? (eEl.GetString() ?? "neutral").Trim().ToLowerInvariant()
                : (req.InstantEmotion ?? "neutral");

            // Detect if VLM summary itself admits it's an abstract figure / drawing / empty room / no person
            var lowSum = rawSummary.ToLowerInvariant();
            if (lowSum.Contains("no person") || lowSum.Contains("abstract") || lowSum.Contains("empty") ||
                lowSum.Contains("немає людини") || lowSum.Contains("відсутн") || lowSum.Contains("порожн") ||
                rawEmotion == "absent")
            {
                vlmPresent = false;
            }

            // Cross-check with C++ 64x48 CV presence detector
            if (req.InstantPresence.HasValue && req.InstantPresence.Value < 28)
            {
                vlmPresent = false;
            }

            if (!vlmPresent)
            {
                return BuildAbsentResponse(req, 0, lang);
            }

            int vlmPresence = ReadScore("presence_score", req.InstantPresence ?? 84);
            int vlmHands = ReadScore("hands_score", req.InstantHands ?? 80);
            int vlmLighting = ReadScore("lighting_score", req.InstantLighting ?? 78);
            int vlmAppearance = ReadScore("appearance_score", 82);
            int vlmPosture = ReadScore("posture_score", req.InstantPosture ?? 80);
            int vlmEmotionScore = ReadScore("emotion_score", req.InstantEmotionScore ?? 82);

            // Fuse VLM semantic scores with C++ 30 FPS objective CV measurements
            int presence = req.InstantPresence.HasValue
                ? (int)Math.Round(vlmPresence * 0.60 + req.InstantPresence.Value * 0.40)
                : vlmPresence;
            int lighting = req.InstantLighting.HasValue
                ? (int)Math.Round(vlmLighting * 0.50 + req.InstantLighting.Value * 0.50)
                : vlmLighting;
            int hands = req.InstantHands.HasValue
                ? (int)Math.Round(vlmHands * 0.65 + req.InstantHands.Value * 0.35)
                : vlmHands;
            int posture = req.InstantPosture.HasValue
                ? (int)Math.Round(vlmPosture * 0.60 + req.InstantPosture.Value * 0.40)
                : vlmPosture;
            int appearance = Math.Clamp(vlmAppearance, 0, 100);
            int emotionScore = req.InstantEmotionScore.HasValue
                ? (int)Math.Round(vlmEmotionScore * 0.65 + req.InstantEmotionScore.Value * 0.35)
                : vlmEmotionScore;

            var normalizedEmotion = NormalizeEmotionLabel(rawEmotion, presence, emotionScore);
            var summary = BuildDiagnosticSummary(rawSummary, presence, lighting, posture, normalizedEmotion, lang);

            int overall = (int)Math.Round(
                presence * 0.28 +
                lighting * 0.20 +
                posture * 0.18 +
                emotionScore * 0.14 +
                appearance * 0.10 +
                hands * 0.10);

            return new VisionAnalyzeResponse(
                OverallScore: Math.Clamp(overall, 0, 100),
                PresenceScore: Math.Clamp(presence, 0, 100),
                HandsScore: Math.Clamp(hands, 0, 100),
                LightingScore: Math.Clamp(lighting, 0, 100),
                AppearanceScore: Math.Clamp(appearance, 0, 100),
                PostureScore: Math.Clamp(posture, 0, 100),
                EmotionScore: Math.Clamp(emotionScore, 0, 100),
                Emotion: normalizedEmotion,
                PersonPresent: true,
                Summary: summary,
                Model: VisionModelName,
                LatencyMs: 0,
                SamplesAnalyzed: 0
            );
        }
        catch
        {
            return BuildInstantFallback(req, 0, lang);
        }
    }

    private VisionAnalyzeResponse BuildAbsentResponse(VisionAnalyzeRequest req, long elapsedMs, string lang)
    {
        int light = Math.Clamp(req.InstantLighting ?? 35, 0, 100);
        int pres = Math.Clamp(req.InstantPresence ?? 8, 0, 18);
        int overall = Math.Clamp((int)Math.Round(pres * 0.75 + light * 0.15), 0, 22);
        return new VisionAnalyzeResponse(
            OverallScore: overall,
            PresenceScore: pres,
            HandsScore: 0,
            LightingScore: light,
            AppearanceScore: 0,
            PostureScore: 0,
            EmotionScore: 0,
            Emotion: "absent",
            PersonPresent: false,
            Summary: lang == "uk"
                ? "Кандидата не видно в кадрі — поверніться перед камерою."
                : "Candidate is not visible in the frame — please return to the camera.",
            Model: VisionModelName,
            LatencyMs: elapsedMs,
            SamplesAnalyzed: 0
        );
    }

    private VisionAnalyzeResponse BuildInstantFallback(VisionAnalyzeRequest req, long elapsedMs, string lang)
    {
        int pres = Math.Clamp(req.InstantPresence ?? req.InstantScore ?? 65, 0, 100);
        if (pres < 28)
        {
            return BuildAbsentResponse(req, elapsedMs, lang);
        }
        int light = Math.Clamp(req.InstantLighting ?? 75, 0, 100);
        int hands = Math.Clamp(req.InstantHands ?? 76, 0, 100);
        int posture = Math.Clamp(req.InstantPosture ?? pres, 0, 100);
        int emoScore = Math.Clamp(req.InstantEmotionScore ?? 78, 0, 100);
        var emo = NormalizeEmotionLabel(req.InstantEmotion ?? "focused", pres, emoScore);

        int overall = (int)Math.Round(
            pres * 0.30 + light * 0.22 + posture * 0.20 + emoScore * 0.14 + hands * 0.14);

        return new VisionAnalyzeResponse(
            OverallScore: Math.Clamp(overall, 0, 100),
            PresenceScore: pres,
            HandsScore: hands,
            LightingScore: light,
            AppearanceScore: Math.Clamp((pres + light) / 2, 0, 100),
            PostureScore: posture,
            EmotionScore: emoScore,
            Emotion: emo,
            PersonPresent: true,
            Summary: BuildDiagnosticSummary("", pres, light, posture, emo, lang),
            Model: $"{VisionModelName} (CV)",
            LatencyMs: elapsedMs,
            SamplesAnalyzed: 0
        );
    }

    private static string NormalizeEmotionLabel(string raw, int presence, int emotionScore)
    {
        if (presence < 28) return "absent";
        var low = (raw ?? "").Trim().ToLowerInvariant();
        if (low.Contains("smile") || low.Contains("happy") || low.Contains("посміш")) return "smile";
        if (low.Contains("confiden") || low.Contains("calm") || low.Contains("впевн")) return "confident";
        if (low.Contains("focus") || low.Contains("attent") || low.Contains("зосеред")) return "focused";
        if (low.Contains("nerv") || low.Contains("tens") || low.Contains("anxi") || low.Contains("напруж")) return "tense";
        if (low.Contains("distract") || low.Contains("away") || low.Contains("відвол")) return "distracted";
        if (low.Contains("absent") || low.Contains("none")) return "absent";
        return emotionScore >= 78 ? "confident" : "neutral";
    }

    private static string BuildDiagnosticSummary(string vlmSummary, int presence, int lighting, int posture, string emotion, string lang)
    {
        var emoUk = emotion switch
        {
            "smile" => "доброзичлива міміка (посмішка)",
            "confident" => "впевнений і спокійний вираз",
            "focused" => "зосереджений погляд",
            "tense" => "помітна напруженість у міміці",
            "distracted" => "погляд часто відводиться вбік",
            "absent" => "відсутній у кадрі",
            _ => "нейтральний спокійний стан"
        };

        if (lang == "uk")
        {
            var lightNote = lighting >= 75 ? "освітлення рівне"
                : lighting >= 50 ? "освітлення помірне"
                : "освітлення слабке (бракує фронтального світла)";
            var postNote = posture >= 75 ? "кадрування й постава добрі"
                : "варто вирівняти положення в центрі кадру";
            return $"У кадрі присутній ({presence}%), {lightNote}, {postNote}, емоція: {emoUk}.";
        }

        if (!string.IsNullOrWhiteSpace(vlmSummary) && vlmSummary.Length <= 140)
        {
            return vlmSummary;
        }
        return $"Present in frame ({presence}%), lighting {lighting}/100, posture {posture}/100, demeanor: {emotion}.";
    }
}
