using System.Net.Http.Json;
using System.Text.Json;
using System.Text.RegularExpressions;
using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;
using Microsoft.Extensions.DependencyInjection;

namespace DevDialogue.Api.Services.AI;

/// <summary>
/// Local Ollama-backed AI interviewer (lfm3-omni:2b) using the Vladyslav 3D Face Import harness:
/// - Picks questions + reference answers from the active DB (SQLite / Postgres / InMemory).
/// - Evaluates answers across 4 weighted criteria (0..100 each):
///   Accuracy (0.45), Completeness (0.25), Terminology (0.15), Clarity (0.15).
/// - Supports interactive follow-up actions: explain_reference, explain_answer, next_question.
/// </summary>
public class OllamaAiService : IAiService
{
    private const double WeightAccuracy = 0.45;
    private const double WeightCompleteness = 0.25;
    private const double WeightTerminology = 0.15;
    private const double WeightClarity = 0.15;

    private readonly HttpClient _httpClient;
    private readonly IConfiguration _config;
    private readonly IServiceScopeFactory _scopeFactory;
    private readonly ILogger<OllamaAiService> _logger;
    private readonly MockAiService _fallback;

    public OllamaAiService(
        HttpClient httpClient,
        IConfiguration config,
        IServiceScopeFactory scopeFactory,
        ILogger<OllamaAiService> logger)
    {
        _httpClient = httpClient;
        _config = config;
        _scopeFactory = scopeFactory;
        _logger = logger;
        _fallback = new MockAiService(scopeFactory);

        var ollamaUrl = _config["AI:OllamaUrl"]
                        ?? Environment.GetEnvironmentVariable("OLLAMA_URL")
                        ?? "http://127.0.0.1:11434";
        _httpClient.BaseAddress = new Uri(ollamaUrl.TrimEnd('/') + "/");
        _httpClient.Timeout = TimeSpan.FromSeconds(60);
    }

    public async Task<string> GenerateFirstQuestionAsync(
        CreateInterviewDto dto,
        string? topicHint,
        CancellationToken cancellationToken = default)
    {
        var isUk = dto.Language.Equals("uk", StringComparison.OrdinalIgnoreCase);
        var dir = (dto.Direction ?? "frontend").Trim().ToLowerInvariant();
        var level = (dto.Level ?? "junior").Trim().ToLowerInvariant();

        var domainExamplesUk = dir switch
        {
            "frontend" => "фреймворки та інструменти (наприклад React, Next.js, Vue, Angular, TypeScript)",
            "backend" => "мови, фреймворки та бази даних (наприклад C#/.NET, Node.js, Python, PostgreSQL, Redis, Docker)",
            "devops" => "CI/CD, контейнеризацію та хмарні платформи (Docker, Kubernetes, Terraform, AWS/Azure/GCP)",
            "qa" => "типи тестування, тест-дизайн та інструменти автоматизації (Playwright, Cypress, Postman, Selenium)",
            "pm" => "методології (Scrum, Kanban), інструменти планування та керування ризиками",
            _ => "основні технології та інструменти"
        };

        var systemPrompt = isUk
            ? "Ти — Владислав, розумний український IT-інтерв'ювер.\n" +
              "Перед технічними запитаннями познайомся з кандидатом: одним лаконічним вступним зверненням запитай, хто він, який має досвід, на яку посаду претендує та які саме технології/фреймворки використовує за обраним напрямком.\n" +
              "Пиши коротко, природно, на «ти», БЕЗ емодзі та БЕЗ зайвої води. Поверни ТІЛЬКИ JSON: {\"emotion\":\"smile\",\"actions_sequence\":[\"Smile\"],\"reply\":\"...\"}."
            : "You are Vladyslav, a smart IT interviewer.\n" +
              "Before technical questions, briefly greet the candidate and ask about their background, experience, target role, and specific frameworks/tools they use for the selected track.\n" +
              "Keep it concise and natural, no emojis. Return ONLY JSON: {\"emotion\":\"smile\",\"actions_sequence\":[\"Smile\"],\"reply\":\"...\"}.";

        var userPrompt = isUk
            ? $"Напрямок: {dir.ToUpperInvariant()}, рівень: {level}. Уточни про {domainExamplesUk}."
            : $"Track: {dir.ToUpperInvariant()}, level: {level}. Ask about their experience, role, and stack.";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 180, temperature: 0.35);
        var reply = TrimIncompleteTrailingSentence(ExtractJsonString(json, "reply"));

        if (!string.IsNullOrWhiteSpace(reply) &&
            !reply.StartsWith("текст привітання", StringComparison.OrdinalIgnoreCase))
        {
            return reply;
        }

        return isUk
            ? $"Вітаю! Я Владислав. Перед технічними запитаннями розкажи коротко про себе: який маєш досвід, на яку посаду претендуєш у {dir.ToUpperInvariant()} ({level}) та з якими фреймворками й інструментами ({domainExamplesUk}) працюєш?"
            : $"Hello! I'm Vladyslav. Before we dive into technical questions, briefly tell me about yourself: your experience, target role in {dir.ToUpperInvariant()} ({level}), and the main frameworks/tools you work with.";
    }

    public async Task<string> GenerateNextResponseAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        CancellationToken cancellationToken = default)
    {
        var outcome = await HandleTurnActionAsync(interview, transcript, null, cancellationToken);
        return outcome.Message;
    }

    public async Task<AiTurnOutcome> HandleTurnActionAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        string? action,
        CancellationToken cancellationToken = default,
        VisualContextDto? visualContext = null)
    {
        var normalizedAction = string.IsNullOrWhiteSpace(action) ? "answer" : action.Trim().ToLowerInvariant();
        var isUk = interview.Language.Equals("uk", StringComparison.OrdinalIgnoreCase);
        var userMessagesCount = transcript.Count(m => m.Role == "user");

        return normalizedAction switch
        {
            "explain_reference" => await HandleExplainReferenceAsync(interview, transcript, isUk, userMessagesCount, cancellationToken),
            "explain_answer" => await HandleExplainAnswerAsync(interview, transcript, isUk, userMessagesCount, cancellationToken),
            "next_question" => await HandleNextQuestionAsync(interview, transcript, isUk, userMessagesCount, cancellationToken),
            _ => await HandleEvaluateAnswerAsync(interview, transcript, isUk, userMessagesCount, visualContext, cancellationToken)
        };
    }

    private async Task<AiTurnOutcome> HandleEvaluateAnswerAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        bool isUk,
        int userMessagesCount,
        VisualContextDto? visualContext,
        CancellationToken cancellationToken)
    {
        var lastUserAnswer = transcript.LastOrDefault(m => m.Role == "user")?.Content ?? string.Empty;
        var questionPromptMessage = FindQuestionMessageForLastUserAnswer(transcript);
        var isIntroTurn = userMessagesCount == 1;

        var askedReference = isIntroTurn
            ? null
            : await QuestionBankSelector.FindReferenceForAskedQuestionAsync(
                _scopeFactory,
                interview.Direction,
                interview.Language,
                questionPromptMessage,
                cancellationToken);

        var questionText = askedReference?.QuestionText ?? questionPromptMessage;
        var expectedAnswer = askedReference?.ExpectedAnswer ?? string.Empty;

        var precheck = AssessCandidateAnswerQuality(lastUserAnswer, expectedAnswer);
        var (explicitActions, explicitUiCommands) = ExtractExplicitAvatarCommands(lastUserAnswer);
        CriteriaEvaluationResult evaluation;
        string reaction;
        string emotion;
        List<string> sequence;

        if (precheck != AnswerPrecheckKind.None)
        {
            evaluation = BuildCriteriaResult(0, 0, 0, 0, isUk, precheck.ToString().ToLowerInvariant(), visualContext);
            reaction = await GeneratePrecheckReactionAsync(precheck, questionText, lastUserAnswer, isUk, cancellationToken);
            if (visualContext is { CameraActive: true, IncludeInScore: true } && !visualContext.PersonPresent)
            {
                reaction += isUk
                    ? " До того ж у тебе увімкнена камера, але тебе зараз не видно в кадрі."
                    : " Also, your camera is on, but you are currently not visible in the frame.";
            }
            emotion = precheck switch
            {
                AnswerPrecheckKind.Refusal => "sad",
                AnswerPrecheckKind.Gibberish => "anger",
                AnswerPrecheckKind.CasualOrJunk => "smirk",
                _ => "skeptical"
            };
            sequence = [ToClipName(emotion)];
            DebugTraceLog.Add("AI Precheck", $"{precheck} -> {evaluation.TotalScore}/100 for \"{TruncateSnippet(lastUserAnswer, 32)}\"");
        }
        else if (isIntroTurn)
        {
            evaluation = await EvaluateIntroAnswerAsync(
                interview.Direction,
                interview.Level,
                lastUserAnswer,
                isUk,
                visualContext,
                cancellationToken);

            reaction = await GenerateIntroReactionAsync(
                interview.Direction,
                interview.Level,
                lastUserAnswer,
                visualContext,
                isUk,
                cancellationToken);

            emotion = "smile";
            sequence = ["Smile"];
        }
        else
        {
            evaluation = await EvaluateByCriteriaAsync(
                questionText,
                expectedAnswer,
                lastUserAnswer,
                interview.Level,
                isUk,
                visualContext,
                cancellationToken);

            reaction = await GenerateEvaluationReactionAsync(
                questionText,
                expectedAnswer,
                lastUserAnswer,
                evaluation,
                visualContext,
                isUk,
                cancellationToken);

            emotion = evaluation.TotalScore >= 90 ? "smile"
                : evaluation.TotalScore >= 75 ? "smile"
                : evaluation.TotalScore >= 40 ? "thinking"
                : evaluation.TotalScore <= 15 || evaluation.Verdict == "nonsense" ? "anger"
                : "skeptical";

            sequence = evaluation.TotalScore >= 92
                ? ["Smile", "Laugh"]
                : [ToClipName(emotion)];
        }

        if (explicitActions.Count > 0)
        {
            sequence = explicitActions;
            emotion = explicitActions[^1].ToLowerInvariant();
        }

        // Do NOT announce or display the numeric score in chat — keep evaluation silent until the Final Report!
        var fullMessage = string.IsNullOrWhiteSpace(reaction)
            ? (isUk ? "Дякую за відповідь, зафіксував." : "Thanks for your answer, noted.")
            : reaction.Trim();

        DebugTraceLog.Add(
            "AI Evaluation",
            $"Verdict={evaluation.Verdict}, Emotion={emotion}, StoredTotal={evaluation.TotalScore}/100 (acc={evaluation.CriteriaMap["accuracy"]}, comp={evaluation.CriteriaMap["completeness"]}, term={evaluation.CriteriaMap["terminology"]}, clar={evaluation.CriteriaMap["clarity"]})");

        var actions = isIntroTurn
            ? new List<string> { "next_question" }
            : (userMessagesCount >= 5
                ? new List<string> { "explain_reference", "explain_answer" }
                : new List<string> { "explain_reference", "explain_answer", "next_question" });

        return new AiTurnOutcome(
            Message: fullMessage,
            AwaitingNextAction: true,
            Score: evaluation.TotalScore,
            CriteriaBreakdown: evaluation.Breakdown,
            SuggestedActions: actions,
            StoredCriteria: evaluation.CriteriaMap,
            AdvancesQuestion: false,
            Emotion: emotion,
            ActionsSequence: sequence,
            UiCommands: explicitUiCommands.Count > 0 ? explicitUiCommands : null
        );
    }

    private async Task<AiTurnOutcome> HandleExplainReferenceAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        bool isUk,
        int userMessagesCount,
        CancellationToken cancellationToken)
    {
        var questionMsg = FindQuestionMessageForLastUserAnswer(transcript);
        var askedReference = await QuestionBankSelector.FindReferenceForAskedQuestionAsync(
            _scopeFactory,
            interview.Direction,
            interview.Language,
            questionMsg,
            cancellationToken);

        var questionText = askedReference?.QuestionText ?? questionMsg;
        var expected = askedReference?.ExpectedAnswer ?? string.Empty;

        var systemPrompt = isUk
            ? "Ти — Владислав, дружній бот-інтерв'ювер. Поясни кандидату еталонну відповідь простою, живою українською мовою (2-3 речення, без емодзі), спираючись строго на наданий Еталон.\n" +
              "Поверни ТІЛЬКИ JSON: {\"emotion\":\"thinking\",\"reply\":\"пояснення еталонної відповіді\"}."
            : "You are Vladyslav, a friendly technical interviewer. Explain the reference answer clearly in 2-3 sentences (no emojis) based strictly on the provided Reference.\n" +
              "Return ONLY JSON: {\"emotion\":\"thinking\",\"reply\":\"reference explanation\"}.";

        var userPrompt = isUk
            ? $"Питання: {questionText}\nЕталон: {expected}"
            : $"Question: {questionText}\nReference: {expected}";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 260);
        var explanation = ExtractJsonString(json, "reply");

        if (string.IsNullOrWhiteSpace(explanation))
        {
            explanation = !string.IsNullOrWhiteSpace(expected)
                ? expected
                : (isUk ? "Ключова суть полягає в коректному поясненні механізму та практичних компромісів." : "The ideal answer explains the core mechanism and practical trade-offs.");
        }

        var header = isUk ? "Еталонна відповідь:" : "Reference answer:";
        var remaining = BuildRemainingActions(transcript, userMessagesCount, justRan: "explain_reference");

        return new AiTurnOutcome(
            Message: $"{header}\n{explanation}",
            AwaitingNextAction: true,
            SuggestedActions: remaining,
            AdvancesQuestion: false,
            Emotion: "thinking",
            ActionsSequence: ["Thinking"]
        );
    }

    private async Task<AiTurnOutcome> HandleExplainAnswerAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        bool isUk,
        int userMessagesCount,
        CancellationToken cancellationToken)
    {
        var lastUserIdx = transcript.FindLastIndex(m => m.Role == "user");
        var userAnswer = lastUserIdx >= 0 ? transcript[lastUserIdx].Content : string.Empty;
        var questionMsg = FindQuestionMessageForLastUserAnswer(transcript);
        var askedReference = await QuestionBankSelector.FindReferenceForAskedQuestionAsync(
            _scopeFactory,
            interview.Direction,
            interview.Language,
            questionMsg,
            cancellationToken);

        Dictionary<string, int>? criteria = null;
        for (int i = lastUserIdx + 1; i < transcript.Count; i++)
        {
            if (transcript[i].Role == "assistant" && transcript[i].Criteria is { Count: > 0 })
            {
                criteria = transcript[i].Criteria;
                break;
            }
        }

        var questionText = askedReference?.QuestionText ?? questionMsg;
        var expected = askedReference?.ExpectedAnswer ?? string.Empty;
        var criteriaSummary = BuildCriteriaSummaryLine(criteria, isUk);
        var precheck = AssessCandidateAnswerQuality(userAnswer, expected);
        var total = criteria?.GetValueOrDefault("total") ?? 0;
        var acc = criteria?.GetValueOrDefault("accuracy") ?? 0;

        string comment;
        if (precheck != AnswerPrecheckKind.None)
        {
            comment = await GeneratePrecheckReactionAsync(precheck, questionText, userAnswer, isUk, cancellationToken, explainMode: true);
        }
        else if (total <= 15 && acc <= 15)
        {
            comment = await GenerateNonsenseExplainCommentAsync(questionText, userAnswer, total, isUk, cancellationToken);
        }
        else
        {
            var systemPrompt = isUk
                ? "Ти — Владислав, живий український технічний інтерв'ювер. Спілкуйся на «ти» природною мовою (без слова «Кандидат»).\n" +
                  "У 2 чітких реченнях поясни кандидату саме за його відповіддю, що було вірно, а чого саме бракувало до вищого балу порівняно з Еталоном.\n" +
                  "Спирайся ТІЛЬКИ на реальний текст відповіді користувача і НЕ вигадуй того, чого він не писав. Поверни ТІЛЬКИ JSON: {\"reply\":\"...\"}."
                : "You are Vladyslav, a real technical interviewer. Address the candidate directly as 'you' in 2 crisp sentences.\n" +
                  "Explain strictly what was accurate in the candidate's actual words and what was missing compared to the Reference. Never invent points they didn't mention.\n" +
                  "Return ONLY JSON: {\"reply\":\"...\"}.";

            var scoreInfo = criteria != null
                ? $"Бал: {total}/100 (Точність {acc}, Повнота {criteria.GetValueOrDefault("completeness")}, Термінологія {criteria.GetValueOrDefault("terminology")}, Структура {criteria.GetValueOrDefault("clarity")})"
                : string.Empty;

            var userPrompt = isUk
                ? $"Питання: {questionText}\nЕталон: {expected}\nВідповідь користувача: {userAnswer}\n{scoreInfo}"
                : $"Question: {questionText}\nReference: {expected}\nCandidate answer: {userAnswer}\n{scoreInfo}";

            var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 260, temperature: 0.35);
            comment = TrimIncompleteTrailingSentence(ExtractJsonString(json, "reply"));

            if (string.IsNullOrWhiteSpace(comment))
            {
                comment = isUk
                    ? "Оцінка відображає те, наскільки твоя відповідь покрила ключові тези та термінологію еталону."
                    : "The score reflects how well your answer covered the core points and terminology of the reference.";
            }
        }

        var fullMessage = string.IsNullOrWhiteSpace(criteriaSummary)
            ? comment
            : $"{criteriaSummary}\n\n{comment}";

        var remaining = BuildRemainingActions(transcript, userMessagesCount, justRan: "explain_answer");
        var explainEmotion = total >= 75 ? "smile" : (total >= 35 ? "thinking" : "skeptical");

        return new AiTurnOutcome(
            Message: fullMessage,
            AwaitingNextAction: true,
            SuggestedActions: remaining,
            AdvancesQuestion: false,
            Emotion: explainEmotion,
            ActionsSequence: [ToClipName(explainEmotion)]
        );
    }

    private async Task<AiTurnOutcome> HandleNextQuestionAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        bool isUk,
        int userMessagesCount,
        CancellationToken cancellationToken)
    {
        if (userMessagesCount >= 5)
        {
            var closing = isUk
                ? "Дякую! Ви відповіли на всі 5 запитань цієї сесії. Натисніть «Finish & get report» («Завершити співбесіду»), щоб отримати підсумковий звіт."
                : "Thank you! You have answered all 5 questions for this session. Click 'Finish & get report' to get your summary.";
            return new AiTurnOutcome(
                Message: closing,
                AwaitingNextAction: false,
                AdvancesQuestion: false,
                Emotion: "smile",
                ActionsSequence: ["Smile"]
            );
        }

        var askedTexts = transcript
            .Where(m => m.Role == "assistant")
            .Select(m => m.Content)
            .ToList();

        var nextQuestion = await QuestionBankSelector.PickNextQuestionAsync(
            _scopeFactory,
            interview.Direction,
            interview.Level,
            interview.Language,
            null,
            askedTexts,
            userMessagesCount,
            cancellationToken);

        // Return ONLY the question itself — no filler phrases ("давай перейдемо до наступного" etc.)
        return new AiTurnOutcome(
            Message: nextQuestion.QuestionText,
            AwaitingNextAction: false,
            SuggestedActions: new List<string>(),
            AdvancesQuestion: true,
            Emotion: "thinking",
            ActionsSequence: ["Thinking"]
        );
    }

    public async Task<InterviewFeedbackDto> GenerateFeedbackAsync(
        Interview interview,
        List<InterviewMessageDto> transcript,
        CancellationToken cancellationToken = default)
    {
        var isUk = interview.Language.Equals("uk", StringComparison.OrdinalIgnoreCase);
        var questionScores = ExtractTurnScoresFromTranscript(transcript, isUk);

        if (questionScores.Count == 0)
        {
            return new InterviewFeedbackDto(
                Summary: isUk
                    ? "Співбесіду завершено достроково без наданих відповідей."
                    : "The interview ended before any candidate answers were evaluated.",
                Score: 0,
                Strengths: new List<string>(),
                Weaknesses: new List<string>
                {
                    isUk ? "Не надано жодної оціненої відповіді на запитання." : "No evaluated answers were provided."
                },
                Recommendations: BuildTargetedRecommendations(interview.Direction, questionScores, isUk),
                QuestionScores: questionScores,
                AverageResponseSeconds: 0,
                TotalDurationSeconds: 0,
                TimingAnalysis: isUk ? "Дані про час відповідей відсутні." : "No response timing data recorded."
            );
        }

        var avgScore = (int)Math.Round(questionScores.Average(q => q.Score));
        var avgResponseSec = (int)Math.Round(questionScores.Average(q => q.ResponseSeconds));
        var totalDurationSec = transcript.Count >= 2
            ? Math.Max(avgResponseSec * questionScores.Count,
                (int)Math.Round((transcript.Last().Timestamp - transcript.First().Timestamp).TotalSeconds))
            : avgResponseSec * questionScores.Count;

        var timingAnalysis = BuildTimingAnalysis(questionScores, avgResponseSec, isUk);
        var (strengths, weaknesses) = BuildFactualStrengthsAndWeaknesses(questionScores, avgScore, isUk);
        var recommendations = BuildTargetedRecommendations(interview.Direction, questionScores, isUk);
        var aiSummary = await SynthesizeReportSummaryWithAiAsync(
            interview, questionScores, avgScore, avgResponseSec, timingAnalysis, isUk, cancellationToken);

        DebugTraceLog.Add(
            "Final Report",
            $"AI+Transcript Report: {questionScores.Count} Qs, avgScore={avgScore}/100, avgTime={avgResponseSec}s, total={totalDurationSec}s");

        return new InterviewFeedbackDto(
            Summary: aiSummary,
            Score: avgScore,
            Strengths: strengths,
            Weaknesses: weaknesses,
            Recommendations: recommendations,
            QuestionScores: questionScores,
            AverageResponseSeconds: avgResponseSec,
            TotalDurationSeconds: totalDurationSec,
            TimingAnalysis: timingAnalysis
        );
    }

    private async Task<CriteriaEvaluationResult> EvaluateByCriteriaAsync(
        string questionText,
        string expectedAnswer,
        string userAnswer,
        string level,
        bool isUk,
        VisualContextDto? visualContext,
        CancellationToken cancellationToken)
    {
        var heuristic = ComputeKeywordFallbackCriteria(expectedAnswer, userAnswer);
        var levelNorm = (level ?? "junior").Trim().ToLowerInvariant();

        var systemPrompt = isUk
            ? "Ти — Владислав, глибокий, розумний і справедливий український технічний інтерв'ювер.\n" +
              $"Рівень кандидата: {levelNorm.ToUpperInvariant()}. Оціни відповідь за змістом (verdict) та 4 технічними критеріями від 0 до 100:\n" +
              "- accuracy: технічна правильність суті та відсутність фактичних помилок.\n" +
              "- completeness: наскільки повно розкрито ключові механізми, сценарії та компроміси з Еталону.\n" +
              "- terminology: точне й доречне використання професійних термінів.\n" +
              "- clarity: логічна структура, причинно-наслідкові зв'язки та чіткість пояснення.\n" +
              "Класифікація verdict:\n" +
              "- \"nonsense\": спам, оффтоп або абсурд -> ВСІ 4 критерії став 0!\n" +
              "- \"refusal\": відмова відповідати -> ВСІ 4 критерії став 0!\n" +
              "- \"wrong\": спроба по темі, але суть невірна -> accuracy 5-20.\n" +
              "- \"partial\": базова ідея вірна, але бракує важливих деталей з Еталону -> accuracy 45-75.\n" +
              "- \"solid\": глибока, точна й структурована відповідь -> усі 4 критерії став у діапазоні 78-96.\n" +
              "Обов'язково заповни ВСІ 4 числові поля (не залишай нулями, якщо відповідь змістовна). Поверни ТІЛЬКИ JSON: {\"verdict\":\"solid\",\"accuracy\":86,\"completeness\":82,\"terminology\":88,\"clarity\":84}"
            : "You are Vladyslav, a senior and fair technical interviewer.\n" +
              $"Candidate level: {levelNorm.ToUpperInvariant()}. Classify verdict ('nonsense'|'refusal'|'wrong'|'partial'|'solid') and score accuracy, completeness, terminology, clarity (0..100).\n" +
              "If verdict is 'nonsense' or 'refusal', all 4 scores MUST be 0. Otherwise score all 4 fields realistically. Return ONLY JSON: {\"verdict\":\"solid\",\"accuracy\":86,\"completeness\":82,\"terminology\":88,\"clarity\":84}.";

        var userPrompt = $"Питання ({levelNorm}): {questionText}\nЕталон: {expectedAnswer}\nВідповідь кандидата: {userAnswer}";
        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 140, temperature: 0.15);

        if (json.HasValue
            && TryReadScore(json.Value, "accuracy", out var acc)
            && TryReadScore(json.Value, "completeness", out var comp)
            && TryReadScore(json.Value, "terminology", out var term)
            && TryReadScore(json.Value, "clarity", out var clar))
        {
            var verdict = ExtractJsonString(json, "verdict")?.ToLowerInvariant() ?? string.Empty;
            if (verdict is "nonsense" or "refusal")
            {
                return BuildCriteriaResult(0, 0, 0, 0, isUk, verdict, visualContext);
            }

            // Guard against 2B model leaving secondary criteria at 0 when accuracy is positive
            if (acc >= 40)
            {
                if (comp <= 15) comp = Math.Max(heuristic.comp, acc - 6);
                if (term <= 15) term = Math.Max(heuristic.term, acc - 4);
                if (clar <= 15) clar = Math.Max(heuristic.clar, acc - 5);
            }

            // Blend LLM judgment (65%) with deterministic semantic-stem & reasoning analysis (35%) for calibrated stability
            if (heuristic.acc > 0)
            {
                acc = (int)Math.Round(acc * 0.65 + heuristic.acc * 0.35);
                comp = (int)Math.Round(comp * 0.62 + heuristic.comp * 0.38);
                term = (int)Math.Round(term * 0.62 + heuristic.term * 0.38);
                clar = (int)Math.Round(clar * 0.62 + heuristic.clar * 0.38);
            }

            return BuildCriteriaResult(acc, comp, term, clar, isUk, string.IsNullOrWhiteSpace(verdict) ? "evaluated" : verdict, visualContext);
        }

        var fbVerdict = heuristic.acc == 0 && heuristic.comp == 0 ? "nonsense" : (heuristic.acc >= 75 ? "solid" : "partial");
        return BuildCriteriaResult(heuristic.acc, heuristic.comp, heuristic.term, heuristic.clar, isUk, fbVerdict, visualContext);
    }

    private async Task<string> GenerateEvaluationReactionAsync(
        string questionText,
        string expectedAnswer,
        string userAnswer,
        CriteriaEvaluationResult evaluation,
        VisualContextDto? visualContext,
        bool isUk,
        CancellationToken cancellationToken)
    {
        if (evaluation.Verdict is "nonsense" or "refusal" || (evaluation.CriteriaMap.GetValueOrDefault("accuracy") <= 10 && evaluation.TotalScore <= 20))
        {
            var baseReaction = evaluation.Verdict == "refusal"
                ? await GeneratePrecheckReactionAsync(AnswerPrecheckKind.Refusal, questionText, userAnswer, isUk, cancellationToken)
                : await GenerateNonsenseExplainCommentAsync(questionText, userAnswer, evaluation.TotalScore, isUk, cancellationToken);

            return AppendCameraFeedbackIfActive(baseReaction, visualContext, isUk);
        }

        var camContextPrompt = string.Empty;
        if (visualContext is { CameraActive: true, IncludeInScore: true })
        {
            camContextPrompt = isUk
                ? $"\nДані вебкамери (враховано в балі): присутність={(visualContext.PersonPresent ? $"{visualContext.PresenceScore}%" : "НЕ ВИДНО У КАДРІ")}, освітлення={visualContext.LightingScore}/100, постава={visualContext.PostureScore}/100, емоція={visualContext.Emotion}."
                : $"\nWebcam telemetry (included in score): present={(visualContext.PersonPresent ? $"{visualContext.PresenceScore}%" : "NOT IN FRAME")}, lighting={visualContext.LightingScore}/100, posture={visualContext.PostureScore}/100, emotion={visualContext.Emotion}.";
        }

        var systemPrompt = isUk
            ? "Ти — Владислав, розумний і уважний український технічний інтерв'ювер. Спілкуйся на «ти», живо й професійно (БЕЗ слова «Кандидат»).\n" +
              "Напиши 2 конкретних речення: 1) що саме у відповіді розкрито влучно та якої ключової деталі з Еталону не вистачило; 2) якщо передано Дані вебкамери й кандидата не видно або слабке світло/напруженість — коротко й тактовно згадай це.\n" +
              "СУВОРО ЗАБОРОНЕНО називати будь-які бали чи цифри оцінки (наприклад 80/100 або 0/100) — оцінку буде показано лише у фінальному звіті! Тільки JSON: {\"reply\":\"...\"}."
            : "You are Vladyslav, a smart technical interviewer. Speak directly as 'you' in 2 crisp sentences: 1) what concept they nailed and what specific point from the Reference was missed; 2) if Webcam telemetry is provided, briefly note their camera presence/lighting/demeanor.\n" +
              "STRICTLY FORBIDDEN to mention numeric scores (like 80/100) — scores are only shown in the final report! Return ONLY JSON: {\"reply\":\"...\"}.";

        var userPrompt = isUk
            ? $"Питання: {questionText}\nЕталон: {expectedAnswer}\nВідповідь користувача: {userAnswer}\nОцінка: {evaluation.TotalScore}/100 ({evaluation.Verdict}){camContextPrompt}"
            : $"Question: {questionText}\nReference: {expectedAnswer}\nCandidate answer: {userAnswer}\nScore: {evaluation.TotalScore}/100 ({evaluation.Verdict}){camContextPrompt}";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 250, temperature: 0.3);
        var reply = TrimIncompleteTrailingSentence(ExtractJsonString(json, "reply"));

        var echoedUser = !string.IsNullOrWhiteSpace(reply)
            && !string.IsNullOrWhiteSpace(userAnswer)
            && (reply.Trim().Equals(userAnswer.Trim(), StringComparison.OrdinalIgnoreCase)
                || (userAnswer.Length >= 24 && reply.Contains(userAnswer[..24], StringComparison.OrdinalIgnoreCase)));

        if (!string.IsNullOrWhiteSpace(reply) && !echoedUser)
        {
            return AppendCameraFeedbackIfActive(reply, visualContext, isUk);
        }

        var smartFallback = BuildSmartConceptReaction(expectedAnswer, userAnswer, evaluation, isUk);
        return AppendCameraFeedbackIfActive(smartFallback, visualContext, isUk);
    }

    private static string BuildSmartConceptReaction(
        string expectedAnswer,
        string userAnswer,
        CriteriaEvaluationResult evaluation,
        bool isUk)
    {
        var lowUser = (userAnswer ?? string.Empty).ToLowerInvariant();
        var expTokens = Regex.Matches((expectedAnswer ?? string.Empty).ToLowerInvariant(), @"[\p{L}\p{N}-]{4,}")
            .Select(m => m.Value)
            .Distinct()
            .ToList();

        var covered = new List<string>();
        var missed = new List<string>();
        foreach (var token in expTokens)
        {
            var stem = token.Length >= 6 ? token[..Math.Max(5, token.Length - 2)] : token;
            if (lowUser.Contains(stem, StringComparison.Ordinal))
            {
                if (covered.Count < 4) covered.Add(token);
            }
            else if (token.Length >= 5 && missed.Count < 3)
            {
                missed.Add(token);
            }
        }

        if (isUk)
        {
            var coveredPart = covered.Count > 0
                ? $"Ти влучно розкрив ключові поняття ({string.Join(", ", covered)})."
                : "Базовий напрямок думки вірний.";
            var missedPart = missed.Count > 0 && evaluation.TotalScore < 88
                ? $" Для ще повнішої відповіді варто було також згадати: {string.Join(", ", missed)}."
                : " Механізм і практичні сценарії пояснено чітко та структуровано.";
            return coveredPart + missedPart;
        }
        else
        {
            var coveredPart = covered.Count > 0
                ? $"You accurately covered the key concepts ({string.Join(", ", covered)})."
                : "Your core reasoning is on the right track.";
            var missedPart = missed.Count > 0 && evaluation.TotalScore < 88
                ? $" To make the answer even stronger, also mention: {string.Join(", ", missed)}."
                : " The mechanism and practical trade-offs are explained clearly.";
            return coveredPart + missedPart;
        }
    }

    private static string AppendCameraFeedbackIfActive(string text, VisualContextDto? vc, bool isUk)
    {
        if (vc is not { CameraActive: true, IncludeInScore: true })
        {
            return text;
        }

        var low = text.ToLowerInvariant();
        if (low.Contains("камер") || low.Contains("кадр") || low.Contains("освітл") || low.Contains("camera") || low.Contains("frame"))
        {
            return text;
        }

        if (!vc.PersonPresent || vc.PresenceScore < 28)
        {
            return text + (isUk
                ? " (Зверни увагу: камера увімкнена, але тебе зараз не видно в кадрі)."
                : " (Note: your camera is on, but you aren't visible in the frame).");
        }

        if (vc.LightingScore < 45)
        {
            return text + (isUk
                ? " (По камері: варто додати трохи фронтального освітлення)."
                : " (Camera note: front lighting could be a bit brighter).");
        }

        return text;
    }

    private async Task<CriteriaEvaluationResult> EvaluateIntroAnswerAsync(
        string direction,
        string level,
        string userAnswer,
        bool isUk,
        VisualContextDto? visualContext,
        CancellationToken cancellationToken)
    {
        var systemPrompt = isUk
            ? "Ти — Владислав, IT-інтерв'ювер. Оціни вступну розповідь кандидата про себе, досвід, посаду та стек технологій за 4 критеріями від 0 до 100 (accuracy: релевантність досвіду напрямку, completeness: чи названо досвід, роль та фреймворки, terminology: доречність назв технологій, clarity: чіткість викладу).\n" +
              "Поверни ТІЛЬКИ JSON: {\"accuracy\":85,\"completeness\":82,\"terminology\":84,\"clarity\":86}."
            : "You are Vladyslav, an IT interviewer. Score the candidate's intro (background, role, experience, frameworks) across accuracy, completeness, terminology, clarity (0..100). Return ONLY JSON: {\"accuracy\":85,\"completeness\":82,\"terminology\":84,\"clarity\":86}.";

        var userPrompt = $"Напрямок: {direction}, рівень: {level}.\nВідповідь кандидата: {userAnswer}";
        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 100, temperature: 0.2);

        if (json.HasValue
            && TryReadScore(json.Value, "accuracy", out var acc)
            && TryReadScore(json.Value, "completeness", out var comp)
            && TryReadScore(json.Value, "terminology", out var term)
            && TryReadScore(json.Value, "clarity", out var clar))
        {
            return BuildCriteriaResult(acc, comp, term, clar, isUk, "intro", visualContext);
        }

        var len = (userAnswer ?? string.Empty).Trim().Length;
        var baseScore = Math.Clamp(60 + Math.Min(32, len / 6), 60, 92);
        return BuildCriteriaResult(baseScore, baseScore - 2, baseScore, baseScore + 2, isUk, "intro", visualContext);
    }

    private async Task<string> GenerateIntroReactionAsync(
        string direction,
        string level,
        string userAnswer,
        VisualContextDto? visualContext,
        bool isUk,
        CancellationToken cancellationToken)
    {
        var systemPrompt = isUk
            ? "Ти — Владислав, розумний український IT-інтерв'ювер. Кандидат щойно коротко розповів про себе, свій досвід і стек.\n" +
              "Дай коротку, живу й поважну реакцію (1-2 речення на «ти»): покажи, що почув його досвід та названі технології, і скажи, що переходите до технічних питань. СУВОРО БЕЗ жодних балів чи оцінок (жодних цифр X/100)! Тільки JSON: {\"reply\":\"...\"}."
            : "You are Vladyslav, a smart IT interviewer. The candidate just introduced themselves, their experience, and tech stack.\n" +
              "Respond warmly and concisely in 1-2 sentences ('you'), acknowledging their stated stack and experience without mentioning any numeric score. Return ONLY JSON: {\"reply\":\"...\"}.";

        var userPrompt = $"Напрямок: {direction} ({level}).\nРозповідь кандидата: {userAnswer}";
        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 160, temperature: 0.35);
        var reply = TrimIncompleteTrailingSentence(ExtractJsonString(json, "reply"));

        if (!string.IsNullOrWhiteSpace(reply))
        {
            return AppendCameraFeedbackIfActive(reply, visualContext, isUk);
        }

        var fallback = isUk
            ? "Чудово, приємно познайомитися і бачити твій стек. Переходимо до технічних запитань."
            : "Great, nice to meet you and see your stack. Let's move on to the technical questions.";
        return AppendCameraFeedbackIfActive(fallback, visualContext, isUk);
    }

    private async Task<string> GeneratePrecheckReactionAsync(
        AnswerPrecheckKind kind,
        string questionText,
        string userAnswer,
        bool isUk,
        CancellationToken cancellationToken,
        bool explainMode = false)
    {
        var snippet = TruncateSnippet(userAnswer, 120);
        string systemPrompt;

        if (isUk)
        {
            var actionNote = explainMode
                ? "Поясни в 1-2 реченнях чому така відповідь не зарахована і запропонуй глянути еталон."
                : "Дай живу реакцію в 1-2 реченнях на цей ввід БЕЗ жодних балів чи цифр (не пиши 0/100!) і запропонуй або відповісти по суті, або глянути еталон.";

            systemPrompt = kind switch
            {
                AnswerPrecheckKind.Gibberish =>
                    "Ти — Владислав, живий український IT-інтерв'ювер. Спілкуйся простою українською на «ти».\n" +
                    $"Користувач прислав абракадабру або клавіатурний спам. {actionNote}\n" +
                    "Обов'язково згадай текст користувача та тему саме з Питання нижче. ЗАБОРОНЕНО хвалити, пояснювати теорію або називати бали (0/100). Тільки JSON: {\"reply\":\"...\"}.\n" +
                    "Приклад 1: Питання: Чим flex відрізняється від grid? | Відповідь: fdsafdsa -> {\"reply\":\"«fdsafdsa» — це просто набір символів, а не відповідь про flex і grid. Відкрий еталонну відповідь нижче.\"}\n" +
                    "Приклад 2: Питання: Як працює B-Tree? | Відповідь: йцукен -> {\"reply\":\"Схоже це випадковий набір по клавіатурі («йцукен») замість відповіді про B-Tree. Подивись еталон нижче.\"}",
                AnswerPrecheckKind.Refusal =>
                    "Ти — Владислав, живий український IT-інтерв'ювер. Спілкуйся простою українською на «ти».\n" +
                    $"Користувач чесно визнав, що не знає або забув відповідь. {actionNote}\n" +
                    "Обов'язково згадай тему саме з Питання нижче. ЗАБОРОНЕНО розкривати теорію або називати бали (0/100). Тільки JSON: {\"reply\":\"...\"}.\n" +
                    "Приклад 1: Питання: Що таке індекс у SQL? | Відповідь: не пам'ятаю -> {\"reply\":\"Окей, бачу тему SQL-індексів поки треба підтягнути. Глянь еталонну відповідь нижче.\"}\n" +
                    "Приклад 2: Питання: Чим відрізняється PUT від PATCH? | Відповідь: хз -> {\"reply\":\"Зрозумів, методи PUT і PATCH зараз не згадуєш. Відкрий еталон нижче, щоб швидко освіжити.\"}",
                AnswerPrecheckKind.CasualOrJunk =>
                    "Ти — Владислав, живий український IT-інтерв'ювер. Спілкуйся простою українською на «ти».\n" +
                    $"Користувач замість технічної відповіді написав побутову чи світську репліку. {actionNote}\n" +
                    "Обов'язково відреагуй саме на його слова і поверни до теми з Питання нижче. ЗАБОРОНЕНО називати бали. Тільки JSON: {\"reply\":\"...\"}.",
                _ =>
                    "Ти — Владислав, живий український IT-інтерв'ювер. Спілкуйся простою українською на «ти».\n" +
                    $"Користувач написав надто коротке або пусте повідомлення без суті. {actionNote}\n" +
                    "Згадай тему з Питання нижче. ЗАБОРОНЕНО хвалити, пояснювати теорію або називати бали. Тільки JSON: {\"reply\":\"...\"}."
            };
        }
        else
        {
            systemPrompt =
                $"You are Vladyslav, a real IT interviewer. Address the user directly in 1-2 short sentences for a ({kind}) reply without mentioning any numeric scores.\n" +
                "Mention the user's text and the specific question topic honestly without lecturing or praising. Return ONLY JSON: {\"reply\":\"...\"}.";
        }

        var userPrompt = isUk
            ? $"Питання: {questionText}\nВідповідь користувача: {snippet}"
            : $"Question: {questionText}\nCandidate input: {snippet}";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 180, temperature: 0.35);
        var reply = TrimIncompleteTrailingSentence(ExtractJsonString(json, "reply"));

        if (!string.IsNullOrWhiteSpace(reply) && !ContainsFalsePraise(reply))
        {
            return reply;
        }

        return explainMode
            ? BuildPrecheckExplainComment(kind, userAnswer, isUk)
            : BuildPrecheckReaction(kind, userAnswer, isUk);
    }

    private async Task<string> GenerateNonsenseExplainCommentAsync(
        string questionText,
        string userAnswer,
        int totalScore,
        bool isUk,
        CancellationToken cancellationToken)
    {
        var systemPrompt = isUk
            ? "Ти — Владислав, живий український IT-інтерв'ювер. Спілкуйся простою українською на «ти» (БЕЗ слова «Кандидат»).\n" +
              "Користувач на технічне запитання написав оффтоп або технічну нісенітницю.\n" +
              "За 1-2 коротких речення чесно вкажи саме на невідповідність його конкретних слів темі Питання, без похвали, без викладання теорії і БЕЗ згадування балів (жодних 0/100). Тільки JSON: {\"reply\":\"...\"}."
            : "You are Vladyslav, a real IT interviewer. The candidate wrote off-topic nonsense instead of a technical answer.\n" +
              "Reply honestly in 1-2 short sentences ('you'), calling out their actual words relative to the question topic without praising, teaching, or mentioning numeric scores. Return ONLY JSON: {\"reply\":\"...\"}.";

        var userPrompt = isUk
            ? $"Питання: {questionText}\nВідповідь користувача: {TruncateSnippet(userAnswer, 180)}\nОцінка: {totalScore}/100"
            : $"Question: {questionText}\nCandidate answer: {TruncateSnippet(userAnswer, 180)}\nScore: {totalScore}/100";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 200, temperature: 0.35);
        var reply = TrimIncompleteTrailingSentence(ExtractJsonString(json, "reply"));

        if (!string.IsNullOrWhiteSpace(reply) && !ContainsFalsePraise(reply))
        {
            return reply;
        }

        return isUk
            ? $"Твоя відповідь «{TruncateSnippet(userAnswer, 48)}» не відповідає на поставлене технічне питання — відкрий еталонну відповідь нижче."
            : $"Your answer \"{TruncateSnippet(userAnswer, 48)}\" is off-topic and does not answer the technical question. Check the reference answer below.";
    }

    private async Task<string> GenerateNextQuestionLeadInAsync(
        List<InterviewMessageDto> transcript,
        string nextTopic,
        bool isUk,
        CancellationToken cancellationToken)
    {
        int? lastScore = null;
        for (int i = transcript.Count - 1; i >= 0; i--)
        {
            if (transcript[i].Role == "assistant" && transcript[i].Criteria is { Count: > 0 } crit && crit.TryGetValue("total", out var t))
            {
                lastScore = t;
                break;
            }
            var m = Regex.Match(transcript[i].Content, @"(?:Оцінка|Score)\s*:\s*(\d{1,3})\s*/\s*100", RegexOptions.IgnoreCase);
            if (m.Success && int.TryParse(m.Groups[1].Value, out var parsed))
            {
                lastScore = parsed;
                break;
            }
        }

        var systemPrompt = isUk
            ? "Ти — Владислав, живий український IT-інтерв'ювер. Напиши ОДНЕ коротке природне речення-перехід (до 12 слів) до наступного технічного запитання, враховуючи бал попередньої відповіді. Без привітань, закінчуй двокрапкою. Тільки JSON: {\"lead\":\"...\"}."
            : "You are Vladyslav, a technical interviewer. Write ONE short natural transition phrase (under 12 words, ending with a colon) to the next topic based on the previous score. Return ONLY JSON: {\"lead\":\"...\"}.";

        var userPrompt = isUk
            ? $"Попередній бал: {(lastScore.HasValue ? $"{lastScore}/100" : "невідомо")}\nНаступна тема: {nextTopic}"
            : $"Previous score: {(lastScore.HasValue ? $"{lastScore}/100" : "unknown")}\nNext topic: {nextTopic}";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 80, temperature: 0.4);
        var lead = ExtractJsonString(json, "lead")?.Trim();

        if (!string.IsNullOrWhiteSpace(lead) && lead.Length <= 110)
        {
            return lead.EndsWith(':') || lead.EndsWith('.') ? lead : lead + ":";
        }

        if (isUk)
        {
            return lastScore is <= 15
                ? "Гаразд, рухаємось далі до наступного питання:"
                : "Добре, переходимо до наступної теми:";
        }
        return "Alright, moving on to the next question:";
    }

    private static CriteriaEvaluationResult BuildCriteriaResult(
        int acc,
        int comp,
        int term,
        int clar,
        bool isUk,
        string verdict = "evaluated",
        VisualContextDto? visualContext = null)
    {
        acc = Math.Clamp(acc, 0, 100);
        comp = Math.Clamp(comp, 0, 100);
        term = Math.Clamp(term, 0, 100);
        clar = Math.Clamp(clar, 0, 100);

        var techWeighted = (int)Math.Round(
            acc * WeightAccuracy +
            comp * WeightCompleteness +
            term * WeightTerminology +
            clar * WeightClarity);

        if (acc < 25)
        {
            techWeighted = Math.Min(techWeighted, acc == 0 && comp == 0 && term == 0 ? 0 : 15);
        }

        var useCamera = visualContext is { CameraActive: true, IncludeInScore: true };
        int camScore = 0;
        if (useCamera)
        {
            camScore = (!visualContext!.PersonPresent || visualContext.PresenceScore < 28)
                ? Math.Clamp(Math.Min(visualContext.VisualScore, 18), 0, 18)
                : Math.Clamp(visualContext.VisualScore, 0, 100);
        }

        int total;
        List<CriterionScoreDto> breakdown;
        var map = new Dictionary<string, int>
        {
            ["accuracy"] = acc,
            ["completeness"] = comp,
            ["terminology"] = term,
            ["clarity"] = clar
        };

        if (useCamera)
        {
            const double wAcc = 0.38;
            const double wComp = 0.22;
            const double wTerm = 0.13;
            const double wClar = 0.12;
            const double wCam = 0.15;

            var combined = (int)Math.Round(
                acc * wAcc +
                comp * wComp +
                term * wTerm +
                clar * wClar +
                camScore * wCam);

            if (acc < 20)
            {
                combined = Math.Min(combined, acc == 0 && comp == 0 && term == 0 ? (int)Math.Round(camScore * 0.10) : 18);
            }

            total = Math.Clamp(combined, 0, 100);
            breakdown = new List<CriterionScoreDto>
            {
                new(isUk ? "Точність" : "Accuracy", acc, wAcc),
                new(isUk ? "Повнота" : "Completeness", comp, wComp),
                new(isUk ? "Термінологія" : "Terminology", term, wTerm),
                new(isUk ? "Структура" : "Clarity", clar, wClar),
                new(isUk ? "Камера та невербаліка" : "Camera & Demeanor", camScore, wCam)
            };

            map["visual"] = camScore;
            map["presence"] = Math.Clamp(visualContext!.PresenceScore, 0, 100);
            map["lighting"] = Math.Clamp(visualContext.LightingScore, 0, 100);
            map["emotion_score"] = Math.Clamp(visualContext.EmotionScore, 0, 100);
        }
        else
        {
            total = Math.Clamp(techWeighted, 0, 100);
            breakdown = new List<CriterionScoreDto>
            {
                new(isUk ? "Точність" : "Accuracy", acc, WeightAccuracy),
                new(isUk ? "Повнота" : "Completeness", comp, WeightCompleteness),
                new(isUk ? "Термінологія" : "Terminology", term, WeightTerminology),
                new(isUk ? "Структура" : "Clarity", clar, WeightClarity)
            };
        }

        map["total"] = total;
        return new CriteriaEvaluationResult(total, breakdown, map, verdict);
    }

    private static string BuildCriteriaSummaryLine(Dictionary<string, int>? criteria, bool isUk)
    {
        if (criteria == null || criteria.Count == 0)
        {
            return string.Empty;
        }

        var acc = criteria.GetValueOrDefault("accuracy");
        var comp = criteria.GetValueOrDefault("completeness");
        var term = criteria.GetValueOrDefault("terminology");
        var clar = criteria.GetValueOrDefault("clarity");
        var total = criteria.GetValueOrDefault("total");

        if (criteria.TryGetValue("visual", out var vis))
        {
            var pres = criteria.GetValueOrDefault("presence", vis);
            var light = criteria.GetValueOrDefault("lighting", vis);
            var emo = criteria.GetValueOrDefault("emotion_score", vis);
            return isUk
                ? $"Розбір балу з урахуванням камери ({total}/100):\n• Точність (38%): {acc}/100\n• Повнота (22%): {comp}/100\n• Термінологія (13%): {term}/100\n• Структура (12%): {clar}/100\n• Камера та невербаліка (15%): {vis}/100 (присутність {pres}%, освітлення {light}%, емоції {emo}%)"
                : $"Score breakdown with camera ({total}/100):\n• Accuracy (38%): {acc}/100\n• Completeness (22%): {comp}/100\n• Terminology (13%): {term}/100\n• Clarity (12%): {clar}/100\n• Camera & Demeanor (15%): {vis}/100 (presence {pres}%, lighting {light}%, emotion {emo}%)";
        }

        return isUk
            ? $"Розбір балу ({total}/100):\n• Точність (45%): {acc}/100\n• Повнота (25%): {comp}/100\n• Термінологія (15%): {term}/100\n• Структура (15%): {clar}/100"
            : $"Score breakdown ({total}/100):\n• Accuracy (45%): {acc}/100\n• Completeness (25%): {comp}/100\n• Terminology (15%): {term}/100\n• Clarity (15%): {clar}/100";
    }

    private static List<string> BuildRemainingActions(List<InterviewMessageDto> transcript, int userMessagesCount, string justRan)
    {
        var actions = new List<string>();
        var lastUserIdx = transcript.FindLastIndex(m => m.Role == "user");
        var recentReplies = lastUserIdx >= 0
            ? transcript.Skip(lastUserIdx + 1).Select(m => m.Content).ToList()
            : new List<string>();

        var refAlreadyShown = justRan == "explain_reference"
            || recentReplies.Any(r => r.StartsWith("Еталонна відповідь:", StringComparison.Ordinal) || r.StartsWith("Reference answer:", StringComparison.Ordinal));
        var ansAlreadyShown = justRan == "explain_answer"
            || recentReplies.Any(r => r.StartsWith("Розбір балу", StringComparison.Ordinal) || r.StartsWith("Score breakdown", StringComparison.Ordinal));

        if (!refAlreadyShown) actions.Add("explain_reference");
        if (!ansAlreadyShown) actions.Add("explain_answer");
        if (userMessagesCount < 5) actions.Add("next_question");
        return actions;
    }

    private static string FindQuestionMessageForLastUserAnswer(List<InterviewMessageDto> transcript)
    {
        var lastUserIdx = transcript.FindLastIndex(m => m.Role == "user");
        if (lastUserIdx <= 0)
        {
            return transcript.LastOrDefault(m => m.Role == "assistant")?.Content ?? string.Empty;
        }

        for (int i = lastUserIdx - 1; i >= 0; i--)
        {
            if (transcript[i].Role == "assistant")
            {
                return transcript[i].Content;
            }
        }
        return string.Empty;
    }

    private async Task<JsonElement?> TryCallOllamaJsonAsync(
        string systemPrompt,
        string userPrompt,
        CancellationToken cancellationToken,
        int numPredict = 200,
        double temperature = 0.2)
    {
        var model = _config["AI:OllamaModel"]
                    ?? Environment.GetEnvironmentVariable("OLLAMA_MODEL")
                    ?? "gemma4:e4b";

        var rawPrompt =
            $"<|im_start|>system\n{systemPrompt}<|im_end|>\n" +
            $"<|im_start|>user\n{userPrompt}<|im_end|>\n" +
            "<|im_start|>assistant\n<think>\n\n</think>\n\n";

        var payload = new
        {
            model,
            prompt = rawPrompt,
            raw = true,
            stream = false,
            keep_alive = "30m",
            options = new
            {
                temperature,
                repeat_penalty = 1.15,
                num_predict = numPredict,
                stop = new[] { "<|im_end|>", "<|endoftext|>" }
            }
        };

        try
        {
            using var response = await _httpClient.PostAsJsonAsync("api/generate", payload, cancellationToken);
            response.EnsureSuccessStatusCode();

            var body = await response.Content.ReadFromJsonAsync<JsonElement>(cancellationToken: cancellationToken);
            var rawText = body.TryGetProperty("response", out var respProp)
                ? respProp.GetString()?.Trim() ?? string.Empty
                : string.Empty;

            var match = Regex.Match(rawText, @"\{[\s\S]*\}");
            if (!match.Success)
            {
                return null;
            }

            using var doc = JsonDocument.Parse(match.Value);
            return doc.RootElement.Clone();
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Local Ollama call failed for model {Model}; using deterministic fallback.", model);
            return null;
        }
    }

    private static bool TryReadScore(JsonElement json, string propName, out int value)
    {
        value = 0;
        if (!json.TryGetProperty(propName, out var prop))
        {
            return false;
        }
        if (prop.ValueKind == JsonValueKind.Number && prop.TryGetInt32(out value))
        {
            return true;
        }
        if (prop.ValueKind == JsonValueKind.String && int.TryParse(prop.GetString(), out value))
        {
            return true;
        }
        return false;
    }

    private static string? ExtractJsonString(JsonElement? element, string propertyName)
    {
        if (element.HasValue
            && element.Value.TryGetProperty(propertyName, out var prop)
            && prop.ValueKind == JsonValueKind.String)
        {
            return prop.GetString()?.Trim();
        }
        return null;
    }

    private static (int acc, int comp, int term, int clar) ComputeKeywordFallbackCriteria(string expectedAnswer, string userAnswer)
    {
        if (AssessCandidateAnswerQuality(userAnswer, expectedAnswer) != AnswerPrecheckKind.None)
        {
            return (0, 0, 0, 0);
        }

        var low = userAnswer.ToLowerInvariant().Trim();
        if (string.IsNullOrWhiteSpace(expectedAnswer))
        {
            var baseVal = Math.Clamp(35 + Math.Min(45, userAnswer.Length / 4), 20, 85);
            return (baseVal, baseVal, baseVal, baseVal);
        }

        var expectedTokens = Regex.Matches(expectedAnswer.ToLowerInvariant(), @"[\p{L}\p{N}]{4,}")
            .Select(m => m.Value)
            .Distinct()
            .ToList();

        if (expectedTokens.Count == 0)
        {
            return (60, 55, 60, 65);
        }

        // Stem-aware matching (matches 5-char stems so Ukrainian/English inflections match reliably)
        var hits = 0;
        foreach (var token in expectedTokens)
        {
            var stem = token.Length >= 6 ? token[..Math.Max(5, token.Length - 2)] : token;
            if (low.Contains(stem, StringComparison.Ordinal))
            {
                hits++;
            }
        }

        if (hits == 0)
        {
            return (0, 0, 0, 0);
        }

        var hasReasoning = Regex.IsMatch(low, @"(?:тому що|оскільки|за рахунок|дозволяє|забезпечує|на відміну|компроміс|складність|наприклад|because|allows|ensures|whereas|trade-?off|for example)");
        var reasoningBonus = hasReasoning ? 8 : 0;

        var ratio = (double)hits / Math.Min(expectedTokens.Count, 8);
        var acc = Math.Clamp((int)Math.Round(28 + ratio * 66 + reasoningBonus), 20, 96);
        var comp = Math.Clamp((int)Math.Round(18 + ratio * 74 + (userAnswer.Length >= 90 ? 6 : 0)), 12, 95);
        var term = Math.Clamp((int)Math.Round(26 + ratio * 68), 20, 95);
        var clar = Math.Clamp((userAnswer.Length > 45 ? 76 : 54) + reasoningBonus, 20, 94);
        return (acc, comp, term, clar);
    }

    private static readonly HashSet<string> ConsonantTechTerms = new(StringComparer.OrdinalIgnoreCase)
    {
        "http", "https", "html", "html5", "css3", "scss", "less", "json", "xml", "yml", "yaml",
        "sql", "nosql", "jwt", "mvc", "mvvm", "mvp", "tdd", "bdd", "ddd", "cqrs", "grpc", "rpc",
        "tcp", "udp", "dns", "tls", "ssl", "ssh", "vpn", "cdn", "waf", "vpc", "k8s", "npm", "npx",
        "cli", "sdk", "jdk", "jvm", "clr", "cpu", "ram", "hdd", "ssd", "dry", "kiss", "srp", "ocp",
        "lsp", "isp", "dip", "2pc", "3nf", "ttl", "rps", "qps", "mttr", "mttf", "sync", "ctrl",
        "prop", "props", "nginx", "csrf", "cors", "xss", "crud", "rest", "soap", "wasm", "vite",
        "next", "nuxt", "jest", "mock", "refs", "helm", "lint", "cron", "bash", "grep", "curl",
        "ping", "port", "host", "root", "sudo", "chmod", "chown", "push", "pull", "fork", "diff",
        "blob", "tree", "hash", "salt", "scrum", "sprint", "burn", "risk", "plan", "test", "bugs",
        "spec", "user", "stub", "flux", "hook", "hooks", "form", "grid", "flex", "post", "drop",
        "join", "view", "lock", "acid", "http2", "http3", "smtp", "icmp", "ipv4", "ipv6", "pods",
        "node", "nodes", "kind", "sast", "dast", "trivy", "vault", "logs", "rbac", "oidc", "saml",
        "oauth", "dom", "api", "orm", "e2e"
    };

    private static readonly string[] KeyboardSmashSequences =
    {
        "qwerty", "qwert", "asdf", "zxcv", "йцуке", "фіва", "олдж", "ячсм",
        "ghbds", "ghbdt", "ntcn", "ghjcnj", "rfr ltk"
    };

    private static readonly HashSet<string> RefusalPhrases = new(StringComparer.OrdinalIgnoreCase)
    {
        "не знаю", "незнаю", "хз", "не пам'ятаю", "не памятаю", "не впевнений", "не шарю",
        "поняття не маю", "без поняття", "пропустити", "далі", "наступне", "забув", "забула",
        "idk", "i don't know", "dont know", "no idea", "pass", "skip", "next", "-", "?"
    };

    private static readonly HashSet<string> CasualJunkPhrases = new(StringComparer.OrdinalIgnoreCase)
    {
        "привіт", "хай", "ку", "добрий день", "здарова", "hello", "hi", "hey",
        "тест", "перевірка", "норм", "ок", "окей", "ok", "okay", "добре",
        "пофіг", "байдуже", "шлак", "фігня", "дурниця", "маячня", "нічого",
        "бла бла", "бла-бла", "bla bla", "blah", "lol", "лол", "haha", "ахах", "хаха"
    };

    private static AnswerPrecheckKind AssessCandidateAnswerQuality(string userAnswer, string expectedAnswer)
    {
        var trimmed = (userAnswer ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(trimmed))
        {
            return AnswerPrecheckKind.EmptyOrTooShort;
        }

        var normalized = Regex.Replace(trimmed.ToLowerInvariant(), @"\s+", " ").TrimEnd('.', '!', '?');
        if (RefusalPhrases.Contains(normalized))
        {
            return AnswerPrecheckKind.Refusal;
        }
        if (CasualJunkPhrases.Contains(normalized))
        {
            return AnswerPrecheckKind.CasualOrJunk;
        }

        var alnumMatches = Regex.Matches(normalized, @"[\p{L}\p{N}]");
        if (alnumMatches.Count < 4)
        {
            return AnswerPrecheckKind.EmptyOrTooShort;
        }

        var wordTokens = Regex.Matches(normalized, @"[\p{L}]+")
            .Select(m => m.Value)
            .ToList();

        if (wordTokens.Count == 0)
        {
            return AnswerPrecheckKind.Gibberish;
        }

        var compact = Regex.Replace(normalized, @"[^a-zа-яіїєґ0-9]", "");
        if (KeyboardSmashSequences.Any(seq => compact.Contains(seq, StringComparison.Ordinal))
            || Regex.IsMatch(compact, @"(.)\1{3,}")
            || (compact.Length >= 6 && Regex.IsMatch(compact, @"^(.{2,4})\1+$")))
        {
            return AnswerPrecheckKind.Gibberish;
        }

        var badTokens = 0;
        foreach (var token in wordTokens)
        {
            if (token.Length >= 4 && !ConsonantTechTerms.Contains(token))
            {
                var hasVowel = Regex.IsMatch(token, @"[aeiouyаеєиіїоуюяёыэ]");
                var tooManyConsonants = Regex.IsMatch(token, @"[^aeiouyаеєиіїоуюяёыэ]{6,}");
                if (!hasVowel || tooManyConsonants)
                {
                    badTokens++;
                }
            }
        }

        if (badTokens > 0 && badTokens * 2 >= wordTokens.Count)
        {
            return AnswerPrecheckKind.Gibberish;
        }

        if (wordTokens.Count <= 8)
        {
            var hasExpectedHit = !string.IsNullOrWhiteSpace(expectedAnswer)
                && wordTokens.Any(w => w.Length >= 4 && expectedAnswer.Contains(w, StringComparison.OrdinalIgnoreCase));

            if (!hasExpectedHit)
            {
                if (Regex.IsMatch(normalized, @"(?:^|\b)(?:не знаю|незнаю|не пам'ятаю|не памятаю|забув|забула|не шарю|поняття не маю|без поняття|хз|idk|don't know|dont know|no idea)(?:\b|$)"))
                {
                    return AnswerPrecheckKind.Refusal;
                }

                if (Regex.IsMatch(normalized, @"^(?:привіт|хай|здарова|добрий день|доброго дня|як справи|як твої справи|як ти|hello|hi|hey|how are you)(?:\b|,|$)"))
                {
                    return AnswerPrecheckKind.CasualOrJunk;
                }
            }
        }

        if (wordTokens.Count == 1 && normalized.Length < 15)
        {
            var single = wordTokens[0];
            var inExpected = !string.IsNullOrWhiteSpace(expectedAnswer)
                && expectedAnswer.Contains(single, StringComparison.OrdinalIgnoreCase);
            if (!inExpected && !ConsonantTechTerms.Contains(single))
            {
                return AnswerPrecheckKind.CasualOrJunk;
            }
        }

        return AnswerPrecheckKind.None;
    }

    private static string BuildPrecheckReaction(AnswerPrecheckKind kind, string userAnswer, bool isUk)
    {
        var snippet = TruncateSnippet(userAnswer, 48);
        return kind switch
        {
            AnswerPrecheckKind.Gibberish => isUk
                ? $"Слухай, «{snippet}» — це просто набір символів без технічного змісту. Можеш подивитися еталонну відповідь нижче або перейти до наступного питання."
                : $"Honestly, \"{snippet}\" is just random characters with no technical meaning. Check the reference answer below or move to the next question.",
            AnswerPrecheckKind.Refusal => isUk
                ? $"Окей, бачу що відповіді на це питання поки немає («{snippet}»). Можеш відкрити еталонну відповідь або перейти далі."
                : $"Got it, no answer here (\"{snippet}\"). Feel free to check the reference answer below or continue.",
            AnswerPrecheckKind.CasualOrJunk => isUk
                ? $"«{snippet}» — це не технічна відповідь по суті питання. Можеш переглянути еталон або перейти до наступного питання."
                : $"\"{snippet}\" isn't a technical answer to the question. Check the reference answer below or move on.",
            _ => isUk
                ? "Повідомлення занадто коротке і без технічної суті. Глянь еталонну відповідь нижче або перейди до наступного питання."
                : "Too short and lacking technical substance. Check the reference answer below or move to the next question."
        };
    }

    private static string BuildPrecheckExplainComment(AnswerPrecheckKind kind, string userAnswer, bool isUk)
    {
        var snippet = TruncateSnippet(userAnswer, 48);
        return kind switch
        {
            AnswerPrecheckKind.Gibberish => isUk
                ? $"Ти написав «{snippet}» — це беззмістовний набір символів (повний шлак), а не відповідь на технічне запитання. Жодної тези чи терміну по суті тут немає, тому по всіх чотирьох критеріях стоїть 0/100."
                : $"You entered \"{snippet}\", which is meaningless characters rather than a technical answer. Because there is no relevant substance, all four criteria are 0/100.",
            AnswerPrecheckKind.Refusal => isUk
                ? $"У повідомленні «{snippet}» ти фактично відмовився від відповіді. Тому за точністю, повнотою, термінологією та структурою виставлено 0/100."
                : $"In \"{snippet}\" you skipped answering the question, so all four criteria are scored 0/100.",
            AnswerPrecheckKind.CasualOrJunk => isUk
                ? $"«{snippet}» не має стосунку до технічної суті запитання і не містить пояснення теми. Через це за всіма критеріями виставлено 0/100."
                : $"\"{snippet}\" is unrelated to the technical question and contains no explanation, resulting in 0/100 across all criteria.",
            _ => isUk
                ? "Повідомлення занадто коротке і не розкриває питання, тому за всіма критеріями виставлено 0/100."
                : "Your message was too short to evaluate against the question, so all criteria are 0/100."
        };
    }

    private static string TruncateSnippet(string text, int maxLen)
    {
        var clean = Regex.Replace((text ?? string.Empty).Trim(), @"\s+", " ");
        return clean.Length <= maxLen ? clean : clean[..maxLen] + "...";
    }

    private static bool ContainsFalsePraise(string reply)
    {
        var low = reply.ToLowerInvariant();
        return low.Contains("правильну основу")
            || low.Contains("чудовий приклад")
            || low.Contains("гарна відповідь")
            || low.Contains("молодець")
            || low.Contains("добре відповів")
            || low.Contains("вірно вказав");
    }

    private static string TrimIncompleteTrailingSentence(string? text)
    {
        var clean = (text ?? string.Empty).Trim();
        if (string.IsNullOrEmpty(clean))
        {
            return string.Empty;
        }

        if (clean.EndsWith('.') || clean.EndsWith('!') || clean.EndsWith('?') || clean.EndsWith('»') || clean.EndsWith('"'))
        {
            return clean;
        }

        var lastPunct = clean.LastIndexOfAny(new[] { '.', '!', '?' });
        if (lastPunct >= 20)
        {
            return clean[..(lastPunct + 1)].Trim();
        }

        return clean;
    }

    private static List<QuestionScoreDto> ExtractTurnScoresFromTranscript(List<InterviewMessageDto> transcript, bool isUk)
    {
        var results = new List<QuestionScoreDto>();
        for (int i = 0; i < transcript.Count; i++)
        {
            if (transcript[i].Role != "user")
            {
                continue;
            }

            string? questionText = null;
            DateTime questionTime = transcript[i].Timestamp;
            for (int qIdx = i - 1; qIdx >= 0; qIdx--)
            {
                if (transcript[qIdx].Role == "assistant")
                {
                    questionText = ExtractCleanQuestionText(transcript[qIdx].Content);
                    questionTime = transcript[qIdx].Timestamp;
                    break;
                }
            }

            var responseSeconds = Math.Max(0, (int)Math.Round((transcript[i].Timestamp - questionTime).TotalSeconds));
            var userAnswer = transcript[i].Content.Trim();

            for (int eIdx = i + 1; eIdx < transcript.Count && transcript[eIdx].Role == "assistant"; eIdx++)
            {
                var content = transcript[eIdx].Content;
                int score = -1;
                bool matchedTurn = false;

                if (transcript[eIdx].Criteria is { Count: > 0 } crit && crit.TryGetValue("total", out var critTotal))
                {
                    score = critTotal;
                    matchedTurn = true;
                }
                else
                {
                    var scoreMatch = Regex.Match(content, @"(?:Оцінка|Score)\s*:\s*(\d{1,3})\s*/\s*100", RegexOptions.IgnoreCase);
                    if (scoreMatch.Success && int.TryParse(scoreMatch.Groups[1].Value, out var parsedScore))
                    {
                        score = parsedScore;
                        matchedTurn = true;
                    }
                }

                if (matchedTurn)
                {
                    var reactionComment = Regex.Replace(content, @"^(?:Оцінка|Score)\s*:\s*\d{1,3}\s*/\s*100\s*", "", RegexOptions.IgnoreCase).Trim();
                    if (string.IsNullOrWhiteSpace(reactionComment))
                    {
                        for (int bIdx = eIdx + 1; bIdx < transcript.Count && transcript[bIdx].Role == "assistant"; bIdx++)
                        {
                            if (transcript[bIdx].Content.StartsWith("Розбір балу", StringComparison.Ordinal)
                                || transcript[bIdx].Content.StartsWith("Score breakdown", StringComparison.Ordinal))
                            {
                                var split = transcript[bIdx].Content.Split("\n\n", 2, StringSplitOptions.RemoveEmptyEntries);
                                reactionComment = split.Length > 1 ? split[1].Trim() : transcript[bIdx].Content;
                                break;
                            }
                        }
                    }

                    if (string.IsNullOrWhiteSpace(reactionComment))
                    {
                        reactionComment = isUk
                            ? "Оцінено інтерв'юером Владиславом."
                            : "Evaluated by Vladyslav.";
                    }

                    results.Add(new QuestionScoreDto(
                        Question: questionText ?? "Question",
                        Score: Math.Clamp(score, 0, 100),
                        Comment: reactionComment,
                        ResponseSeconds: responseSeconds,
                        UserAnswer: TruncateSnippet(userAnswer, 180),
                        Criteria: transcript[eIdx].Criteria
                    ));
                    break;
                }
            }
        }
        return results;
    }

    private static string ExtractCleanQuestionText(string assistantText)
    {
        var lines = assistantText.Split('\n', StringSplitOptions.RemoveEmptyEntries)
            .Select(l => l.Trim())
            .Where(l => !string.IsNullOrWhiteSpace(l))
            .ToList();

        if (lines.Count == 0)
        {
            return assistantText.Trim();
        }

        var last = lines.Last();
        var colonIdx = last.IndexOf("першого запитання:", StringComparison.OrdinalIgnoreCase);
        if (colonIdx >= 0)
        {
            var after = last[(colonIdx + "першого запитання:".Length)..].Trim();
            if (!string.IsNullOrWhiteSpace(after))
            {
                return after;
            }
        }
        return last;
    }

    private async Task<string> SynthesizeReportSummaryWithAiAsync(
        Interview interview,
        List<QuestionScoreDto> scores,
        int avgScore,
        int avgResponseSec,
        string timingAnalysis,
        bool isUk,
        CancellationToken cancellationToken)
    {
        var sb = new System.Text.StringBuilder();
        for (int i = 0; i < scores.Count; i++)
        {
            var q = scores[i];
            sb.AppendLine($"{i + 1}) \"{TruncateSnippet(q.Question, 90)}\" -> \"{TruncateSnippet(q.UserAnswer, 80)}\" | {q.Score}/100 | {q.ResponseSeconds}s");
        }

        var systemPrompt = isUk
            ? "Ти — Владислав, старший український IT-інтерв'ювер. Дай живий, чесний і стислий підсумковий коментар (2-3 речення на «ти») про всю сесію загалом, спираючись на відповіді та темп.\n" +
              "Якщо більшість відповідей провальні або спам — скажи про це прямо без вигаданої похвали. НЕ повторюй дослівно список питань.\n" +
              "Поверни ТІЛЬКИ JSON: {\"summary\":\"...\"}"
            : "You are Vladyslav, a senior technical interviewer. Write an honest 2-3 sentence overall assessment addressing the candidate as 'you' based on their answers, scores, and pacing. Return ONLY JSON: {\"summary\":\"...\"}.";

        var userPrompt = isUk
            ? $"Напрямок: {interview.Direction.ToUpperInvariant()} ({interview.Level}). Середній бал: {avgScore}/100. Середній час відповіді: {avgResponseSec}с.\nАналіз темпу: {timingAnalysis}\nЗапитання та результати:\n{sb}"
            : $"Track: {interview.Direction.ToUpperInvariant()} ({interview.Level}). Avg score: {avgScore}/100. Avg response time: {avgResponseSec}s.\nPacing: {timingAnalysis}\nTurns:\n{sb}";

        var json = await TryCallOllamaJsonAsync(systemPrompt, userPrompt, cancellationToken, numPredict: 220, temperature: 0.3);
        var reply = TrimIncompleteTrailingSentence(ExtractJsonString(json, "summary"));

        if (!string.IsNullOrWhiteSpace(reply)
            && !reply.StartsWith("Напрямок:", StringComparison.OrdinalIgnoreCase)
            && !reply.StartsWith("Track:", StringComparison.OrdinalIgnoreCase)
            && !(avgScore < 20 && ContainsFalsePraise(reply)))
        {
            return reply;
        }

        if (isUk)
        {
            return avgScore switch
            {
                >= 80 => $"Сильна співбесіда за напрямком {interview.Direction.ToUpperInvariant()} ({interview.Level}) — середній бал {avgScore}/100 при середньому темпі {avgResponseSec}с на запитання. Ти впевнено володієш ключовою теорією та аргументацією.",
                >= 50 => $"Непоганий базовий результат ({avgScore}/100 за {scores.Count} запитань, середній час {avgResponseSec}с). Є розуміння основ, але частину важливих деталей варто доопрацювати.",
                >= 20 => $"Результат нижче очікуваного для {interview.Direction.ToUpperInvariant()} ({interview.Level}) — {avgScore}/100 (середній час {avgResponseSec}с). Потрібно системно повторити прогалини за наведеними темами нижче.",
                _ => $"Ця сесія показала критичну прогалину в підготовці ({avgScore}/100 за {scores.Count} запитань, середній час {avgResponseSec}с) — більшість відповідей не містили технічної суті. Рекомендую почати з базових тем у списку нижче."
            };
        }

        return $"Completed {scores.Count} questions in {interview.Direction.ToUpperInvariant()} ({interview.Level}) with an average score of {avgScore}/100 and {avgResponseSec}s average response time.";
    }

    private static string BuildTimingAnalysis(List<QuestionScoreDto> scores, int avgSec, bool isUk)
    {
        if (scores.Count == 0)
        {
            return isUk ? "Дані про час відсутні." : "No timing data available.";
        }

        var fastest = scores.MinBy(q => q.ResponseSeconds)!;
        var slowest = scores.MaxBy(q => q.ResponseSeconds)!;
        var rushedPoorCount = scores.Count(q => q.ResponseSeconds <= 5 && q.Score <= 15);
        var thoughtfulGoodCount = scores.Count(q => q.ResponseSeconds >= 10 && q.Score >= 65);

        if (isUk)
        {
            var parts = new List<string>
            {
                $"Середній час на відповідь — {FormatSecondsUk(avgSec)} (від {FormatSecondsUk(fastest.ResponseSeconds)} до {FormatSecondsUk(slowest.ResponseSeconds)})."
            };

            if (rushedPoorCount > 0)
            {
                parts.Add(rushedPoorCount == scores.Count
                    ? "Усі відповіді надіслано за лічені секунди без технічного розбору."
                    : $"Зафіксовано {rushedPoorCount} миттєвих відповідей (≤5с) із низьким або нульовим результатом — варто не поспішати й формулювати думку повніше.");
            }
            else if (thoughtfulGoodCount == scores.Count)
            {
                parts.Add("Рівний, вдумливий темп — достатньо часу на обдумування без втрати якості.");
            }
            else if (slowest.ResponseSeconds >= 45 && slowest.Score < 50)
            {
                parts.Add($"Найдовші роздуми ({FormatSecondsUk(slowest.ResponseSeconds)}) на складному питанні не дали точного результату — цю тему варто закріпити.");
            }
            else
            {
                parts.Add(avgSec <= 12
                    ? "Темп швидкий; на розгорнутих питаннях корисно додавати більше прикладів."
                    : "Спокійний і послідовний темп відповідей упродовж сесії.");
            }

            return string.Join(" ", parts);
        }

        return $"Average response time is {avgSec}s (range {fastest.ResponseSeconds}s–{slowest.ResponseSeconds}s)" +
               (rushedPoorCount > 0
                   ? $", with {rushedPoorCount} rushed (≤5s) low-score replies."
                   : ".");
    }

    private static (List<string> Strengths, List<string> Weaknesses) BuildFactualStrengthsAndWeaknesses(
        List<QuestionScoreDto> scores,
        int avgScore,
        bool isUk)
    {
        var strengths = new List<string>();
        var weaknesses = new List<string>();

        foreach (var q in scores.Where(s => s.Score >= 65).OrderByDescending(s => s.Score).Take(3))
        {
            strengths.Add(isUk
                ? $"Впевнена відповідь ({q.Score}/100 за {q.ResponseSeconds}с): «{TruncateSnippet(q.Question, 68)}»"
                : $"Strong answer ({q.Score}/100 in {q.ResponseSeconds}s): \"{TruncateSnippet(q.Question, 68)}\"");
        }

        var withCriteria = scores.Where(s => s.Criteria is { Count: > 0 }).ToList();
        if (withCriteria.Count > 0)
        {
            var avgAcc = (int)Math.Round(withCriteria.Average(s => s.Criteria!.GetValueOrDefault("accuracy")));
            var avgComp = (int)Math.Round(withCriteria.Average(s => s.Criteria!.GetValueOrDefault("completeness")));
            var avgTerm = (int)Math.Round(withCriteria.Average(s => s.Criteria!.GetValueOrDefault("terminology")));
            var avgClar = (int)Math.Round(withCriteria.Average(s => s.Criteria!.GetValueOrDefault("clarity")));

            var metrics = new (string LabelUk, string LabelEn, int Val)[]
            {
                ("Точність", "Accuracy", avgAcc),
                ("Повнота", "Completeness", avgComp),
                ("Термінологія", "Terminology", avgTerm),
                ("Структура", "Clarity", avgClar)
            };

            var best = metrics.MaxBy(m => m.Val);
            var worst = metrics.MinBy(m => m.Val);

            if (best.Val >= 60 && strengths.Count < 3)
            {
                strengths.Add(isUk
                    ? $"Найсильніший критерій сесії — {best.LabelUk} ({best.Val}/100)"
                    : $"Strongest criterion across session: {best.LabelEn} ({best.Val}/100)");
            }
            if (worst.Val < 70)
            {
                weaknesses.Add(isUk
                    ? $"Найслабший критерій — {worst.LabelUk} ({worst.Val}/100)"
                    : $"Weakest criterion across session: {worst.LabelEn} ({worst.Val}/100)");
            }
        }

        foreach (var q in scores.Where(s => s.Score < 60).OrderBy(s => s.Score).Take(3))
        {
            weaknesses.Add(isUk
                ? $"Прогалина ({q.Score}/100 за {q.ResponseSeconds}с): «{TruncateSnippet(q.Question, 68)}»"
                : $"Gap ({q.Score}/100 in {q.ResponseSeconds}s): \"{TruncateSnippet(q.Question, 68)}\"");
        }

        if (strengths.Count == 0 && avgScore >= 40)
        {
            strengths.Add(isUk
                ? "Є базове уявлення про частину тем, але відповідям бракує глибини та чітких прикладів."
                : "Shows partial familiarity with topics, though answers need deeper technical detail.");
        }

        if (weaknesses.Count == 0 && avgScore >= 85)
        {
            weaknesses.Add(isUk
                ? "Критичних прогалин не виявлено — для росту до Senior варто поглиблювати edge-cases та архітектурні компроміси."
                : "No critical gaps found; focus on production edge cases and architectural trade-offs.");
        }

        return (strengths, weaknesses);
    }

    private static List<RecommendationDto> BuildTargetedRecommendations(
        string direction,
        List<QuestionScoreDto> scores,
        bool isUk)
    {
        var recs = new List<RecommendationDto>();
        var weakText = string.Join(" ", scores.Where(s => s.Score < 75).Select(s => s.Question)).ToLowerInvariant();

        void TryAdd(string keyword, string topicUk, string topicEn, string url)
        {
            if (recs.Count >= 4 || recs.Any(r => r.Resource == url))
            {
                return;
            }
            if (weakText.Contains(keyword, StringComparison.Ordinal))
            {
                recs.Add(new RecommendationDto(isUk ? topicUk : topicEn, url));
            }
        }

        TryAdd("react", "React — офіційна документація (Hooks & Rendering)", "React Docs — Hooks & Rendering", "https://react.dev/reference/react");
        TryAdd("virtual dom", "React Reconciliation & Rendering", "React Reconciliation & Rendering", "https://react.dev/learn/preserving-and-resetting-state");
        TryAdd("css", "MDN CSS Layout (Flexbox & Grid)", "MDN CSS Layout (Flexbox & Grid)", "https://developer.mozilla.org/en-US/docs/Learn/CSS/CSS_layout");
        TryAdd("tcp", "MDN / Cloudflare — TCP vs UDP & Мережі", "TCP vs UDP & Networking Basics", "https://www.cloudflare.com/learning/ddos/glossary/tcp-ip/");
        TryAdd("b-tree", "Use The Index, Luke — B-Tree та SQL індекси", "Use The Index, Luke — SQL Indexing", "https://use-the-index-luke.com");
        TryAdd("dependency injection", "ASP.NET Core Dependency Injection", "ASP.NET Core Dependency Injection", "https://learn.microsoft.com/en-us/aspnet/core/fundamentals/dependency-injection");
        TryAdd("rest", "RESTful API Design Best Practices", "RESTful API Design Best Practices", "https://learn.microsoft.com/en-us/azure/architecture/best-practices/api-design");
        TryAdd("docker", "Docker Docs — Контейнери та образи", "Docker Docs — Containers & Images", "https://docs.docker.com/get-started/");
        TryAdd("kubernetes", "Kubernetes — Pods, Services & Ingress", "Kubernetes — Pods, Services & Ingress", "https://kubernetes.io/docs/concepts/");
        TryAdd("верифікац", "Піраміда тестування та QA-стратегія (Martin Fowler)", "Practical Test Pyramid (Martin Fowler)", "https://martinfowler.com/articles/practical-test-pyramid.html");
        TryAdd("тест-дизайн", "ISTQB — Техніки тест-дизайну", "ISTQB — Test Design Techniques", "https://www.istqb.org/certifications/certified-tester-foundation-level");
        TryAdd("scrum", "Офіційний Scrum Guide", "The Official Scrum Guide", "https://scrumguides.org/scrum-guide.html");

        if (recs.Count == 0)
        {
            var dir = direction.ToLowerInvariant();
            if (dir == "frontend") recs.Add(new(isUk ? "React та сучасний Frontend (react.dev)" : "React Docs", "https://react.dev/learn"));
            else if (dir == "backend") recs.Add(new(isUk ? "Архітектура та БД (System Design Primer)" : "System Design Primer", "https://github.com/donnemartin/system-design-primer"));
            else if (dir == "devops") recs.Add(new(isUk ? "Docker та Kubernetes Основи" : "Kubernetes Concepts", "https://kubernetes.io/docs/concepts/"));
            else if (dir == "qa") recs.Add(new(isUk ? "Практична піраміда тестування" : "Practical Test Pyramid", "https://martinfowler.com/articles/practical-test-pyramid.html"));
            else recs.Add(new(isUk ? "Agile, Scrum та управління ризиками" : "The Official Scrum Guide", "https://scrumguides.org/scrum-guide.html"));
        }

        return recs;
    }

    private static string FormatSecondsUk(int seconds)
    {
        if (seconds < 60)
        {
            return $"{seconds}с";
        }
        var mins = seconds / 60;
        var rem = seconds % 60;
        return rem == 0 ? $"{mins}хв" : $"{mins}хв {rem}с";
    }

    private enum AnswerPrecheckKind
    {
        None,
        EmptyOrTooShort,
        Gibberish,
        Refusal,
        CasualOrJunk
    }

    private static string ToClipName(string emotion) => emotion.ToLowerInvariant() switch
    {
        "laugh" => "Laugh",
        "smile" => "Smile",
        "sad" => "Sad",
        "surprise" => "Surprise",
        "thinking" => "Thinking",
        "anger" => "Anger",
        "skeptical" => "Skeptical",
        "wink" => "Wink",
        "fear" => "Fear",
        "shy" => "Shy",
        "disgust" => "Disgust",
        "smirk" => "Smirk",
        _ => "Smile"
    };

    private static (List<string> Actions, List<AvatarUiCommandDto> UiCommands) ExtractExplicitAvatarCommands(string text)
    {
        var actions = new List<string>();
        var uiCommands = new List<AvatarUiCommandDto>();
        if (string.IsNullOrWhiteSpace(text)) return (actions, uiCommands);

        var low = text.ToLowerInvariant();
        var emotionPatterns = new (string Clip, string Pattern)[]
        {
            ("Smile", @"(?<![\w\u0400-\u04FF])(?:посміхн\w*|усміхн\w*|усміш\w*|посміш\w*|улиб\w*|smile\w*)"),
            ("Laugh", @"(?<![\w\u0400-\u04FF])(?:посмій\w*|засмій\w*|смійс\w*|сміх\w*|регот\w*|ха-?ха\w*|laugh\w*)"),
            ("Anger", @"(?<![\w\u0400-\u04FF])(?:назмур\w*|нахмур\w*|хмур\w*|насуп\w*|розізл\w*|злис\w*|сердит\w*|гнів\w*|anger|angry|frown\w*)"),
            ("Sad", @"(?<![\w\u0400-\u04FF])(?:засуму\w*|посуму\w*|сумуй\w*|поплач\w*|плач\w*|sad\w*)"),
            ("Surprise", @"(?<![\w\u0400-\u04FF])(?:здиву\w*|подиву\w*|зроби подив|surprise\w*)"),
            ("Thinking", @"(?<![\w\u0400-\u04FF])(?:подума\w*|задума\w*|помірку\w*|thinking|think\w*)"),
            ("Skeptical", @"(?<![\w\u0400-\u04FF])(?:скептич\w*|прищур\w*|недовір\w*|skeptical)"),
            ("Wink", @"(?<![\w\u0400-\u04FF])(?:підморг\w*|моргн\w*|wink\w*)"),
            ("Fear", @"(?<![\w\u0400-\u04FF])(?:зляка\w*|наляка\w*|зроби страх|fear\w*)"),
            ("Shy", @"(?<![\w\u0400-\u04FF])(?:засором\w*|зніяков\w*|почервон\w*|shy)"),
            ("Disgust", @"(?<![\w\u0400-\u04FF])(?:скриви\w*|відраз\w*|гид\w*|disgust\w*)"),
            ("Smirk", @"(?<![\w\u0400-\u04FF])(?:ухмил\w*|хмил\w*|лукав\w*|smirk\w*)"),
        };

        var hits = new List<(int Index, string Clip)>();
        foreach (var (clip, pattern) in emotionPatterns)
        {
            foreach (Match m in Regex.Matches(low, pattern))
            {
                hits.Add((m.Index, clip));
            }
        }
        hits.Sort((a, b) => a.Index.CompareTo(b.Index));
        foreach (var (_, clip) in hits)
        {
            if (actions.Count == 0 || actions[^1] != clip)
                actions.Add(clip);
        }

        var accessoryPatterns = new (string Name, string Pattern)[]
        {
            ("Hat", @"(?:капелюх\w*|шляп\w*|\bhat\b)"),
            ("Horns", @"(?:рог\w*|ріжк\w*|\bhorns?\b)"),
            ("Halo", @"(?:німб\w*|\bhalo\b)"),
            ("Glasses", @"(?:окуляр\w*|\bglasses\b)"),
            ("Tie", @"(?:краватк\w*|галстук\w*|\btie\b)"),
        };

        var clauses = Regex.Split(low, @"[,;.!]|\s+(?:і|та|й|або|потім|далі|після чого)\s+");
        foreach (var rawClause in clauses)
        {
            var c = rawClause.Trim();
            if (string.IsNullOrEmpty(c)) continue;

            var isOff = Regex.IsMatch(c, @"(?:зніми|знять|прибер|вимкн|виключ|видал|схова|без\s+|off|remove|hide)");
            var isOn = Regex.IsMatch(c, @"(?:одягн|надін|вдягн|дода|увімкн|включ|покаж|постав|надяг|wear|put|enable|\bon\b)");

            if (Regex.IsMatch(c, @"(?:зніми|прибер|вимкн|без).*(?:все|усе|аксесуар|одяг)"))
            {
                foreach (var (name, _) in accessoryPatterns)
                    uiCommands.Add(new("accessory_off", name));
                continue;
            }

            foreach (var (accName, accPat) in accessoryPatterns)
            {
                if (Regex.IsMatch(c, accPat))
                {
                    var act = isOff ? "accessory_off" : (isOn ? "accessory_on" : "accessory_toggle");
                    uiCommands.Add(new(act, accName));
                }
            }

            if (Regex.IsMatch(c, @"(?:жіноч\w*|жінк\w*|дівчин\w*|\bfemale\b)") &&
                Regex.IsMatch(c, @"(?:модел|аватар|персонаж|перемкн|зміни|стань|будь|переключ|switch|model|avatar)"))
            {
                uiCommands.Add(new("set_gender", "Female"));
            }
            else if (Regex.IsMatch(c, @"(?:чоловіч\w*|чоловік\w*|хлопц\w*|\bmale\b)") &&
                     Regex.IsMatch(c, @"(?:модел|аватар|персонаж|перемкн|зміни|стань|будь|переключ|switch|model|avatar)"))
            {
                uiCommands.Add(new("set_gender", "Male"));
            }
        }

        return (actions, uiCommands);
    }

    private sealed record CriteriaEvaluationResult(
        int TotalScore,
        List<CriterionScoreDto> Breakdown,
        Dictionary<string, int> CriteriaMap,
        string Verdict = "evaluated");
}
