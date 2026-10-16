using DevDialogue.Api.Models.Dtos;
using DevDialogue.Api.Models.Entities;

namespace DevDialogue.Api.Services.AI;

public class MockAiService : IAiService
{
    private static readonly Dictionary<string, List<string>> UkrainianQuestions = new()
    {
        ["frontend"] = new()
        {
            "Розкажіть про життєвий цикл компонента в React і як хук useEffect співвідноситься з ним?",
            "У чому різниця між controlled та uncontrolled компонентами у формах?",
            "Як працює Virtual DOM і за рахунок чого React оптимізує оновлення інтерфейсу?",
            "Які підходи ви використовуєте для оптимізації рендерингу (useMemo, useCallback, React.memo)?",
            "Поясніть механізм CSS box model та різницю між display: flex та display: grid."
        },
        ["backend"] = new()
        {
            "Поясніть різницю між протоколами TCP та UDP. У яких сценаріях UDP є кращим вибором?",
            "Як працюють індекси в базах даних (зокрема B-Tree) та коли індекс може нашкодити продуктивності?",
            "Що таке Dependency Injection і як налаштовуються Scoped, Transient та Singleton сервіси в ASP.NET Core?",
            "Поясніть принципи REST і різницю між методами PUT та PATCH.",
            "Як ви реалізуєте обробку помилок і транзакційність у мікросервісній або модульній системі?"
        },
        ["devops"] = new()
        {
            "Поясніть різницю між Docker контейнером та віртуальною машиною.",
            "Як організувати безпечний CI/CD пайплайн з тестуванням та захистом секретів?",
            "Що таке Kubernetes Pod, Service та Ingress і як вони взаємодіють?",
            "Які метрики та інструменти ви використовуєте для моніторингу навантаження систем?"
        },
        ["qa"] = new()
        {
            "Яка різниця між регресійним і санітарним (smoke/sanity) тестуванням?",
            "Як ви підходите до складання тест-плану для нового функціоналу авторизації?",
            "Опишіть техніки тест-дизайну: еквівалентне розбиття та аналіз граничних значень."
        },
        ["pm"] = new()
        {
            "Як ви керуєте ризиками на проекті у випадку зриву дедлайну командою?",
            "Яка різниця між методологіями Scrum та Kanban і коли яку обирати?",
            "Як ви вирішуєте міжособистісні або технічні конфлікти всередині команди?"
        }
    };

    private static readonly Dictionary<string, List<string>> EnglishQuestions = new()
    {
        ["frontend"] = new()
        {
            "Could you explain the difference between client-side rendering (CSR) and server-side rendering (SSR) in Next.js?",
            "How does the browser event loop work, specifically regarding the microtask and macrotask queues?",
            "How do you approach global state management in modern React applications?",
            "What strategies do you employ to improve Core Web Vitals (LCP, INP, CLS)?"
        },
        ["backend"] = new()
        {
            "Explain the difference between TCP and UDP. When is UDP preferred in real-world distributed systems?",
            "How do database transactions work and what are the ACID properties?",
            "Can you explain the differences between horizontal and vertical scaling for API services?",
            "How do asynchronous programming and async/await work under the hood in modern runtimes?"
        },
        ["devops"] = new()
        {
            "What is the difference between container virtualization and hypervisor virtualization?",
            "How do you design a zero-downtime rolling deployment strategy in Kubernetes?",
            "What practices do you recommend for Infrastructure as Code (IaC) governance?"
        },
        ["qa"] = new()
        {
            "What is the difference between functional and non-functional testing?",
            "How do you design an automated integration test suite for REST APIs?"
        },
        ["pm"] = new()
        {
            "How do you prioritize backlog items when stakeholders have competing requirements?",
            "How do you identify and mitigate technical debt during rapid iteration cycles?"
        }
    };

    public Task<string> GenerateFirstQuestionAsync(CreateInterviewDto dto, string? topicHint, CancellationToken cancellationToken = default)
    {
        var isUk = dto.Language.ToLowerInvariant() == "uk";
        var dict = isUk ? UkrainianQuestions : EnglishQuestions;
        var directionKey = dto.Direction.ToLowerInvariant();

        if (!dict.TryGetValue(directionKey, out var list) || list.Count == 0)
        {
            list = isUk ? UkrainianQuestions["backend"] : EnglishQuestions["backend"];
        }

        var greeting = isUk
            ? $"Вітаю на технічній співбесіді на позицію {dto.Direction.ToUpperInvariant()} ({dto.Level})! Мене звати AI-інтерв'юер. Давайте почнемо з базових концепцій.\n\n"
            : $"Welcome to your technical interview for the {dto.Direction.ToUpperInvariant()} ({dto.Level}) position! I will be your interviewer today. Let's begin with our first question.\n\n";

        var question = list[0];
        return Task.FromResult(greeting + question);
    }

    public Task<string> GenerateNextResponseAsync(Interview interview, List<InterviewMessageDto> transcript, CancellationToken cancellationToken = default)
    {
        var isUk = interview.Language.ToLowerInvariant() == "uk";
        var dict = isUk ? UkrainianQuestions : EnglishQuestions;
        var directionKey = interview.Direction.ToLowerInvariant();

        if (!dict.TryGetValue(directionKey, out var list) || list.Count == 0)
        {
            list = isUk ? UkrainianQuestions["backend"] : EnglishQuestions["backend"];
        }

        // Determine question index based on user message count
        var userMessagesCount = transcript.Count(m => m.Role == "user");

        if (userMessagesCount >= 5)
        {
            var closing = isUk
                ? "Дякую! Ви відповіли на всі запитання цієї сесії співбесіди. Зараз ви можете натиснути кнопку 'Завершити співбесіду', щоб я підготував для вас детальний звіт та оцінку знань."
                : "Thank you! You have answered all primary questions for this interview session. You can now click 'Complete Interview' to generate your comprehensive performance report and scoring.";
            return Task.FromResult(closing);
        }

        var nextIndex = userMessagesCount % list.Count;
        var question = list[nextIndex];

        var followUpsUk = new[]
        {
            "Дякую за відповідь, це гарне пояснення. Давайте поглибимося: ",
            "Цікава думка. А як ви підійдете до наступного питання: ",
            "Чудово. Переходимо до наступної теми: "
        };

        var followUpsEn = new[]
        {
            "Thank you for that explanation, well articulated. Moving on to the next topic: ",
            "Good point. Now let's explore this next area: ",
            "Great. Here is our next question: "
        };

        var followUps = isUk ? followUpsUk : followUpsEn;
        var prefix = followUps[userMessagesCount % followUps.Length];

        return Task.FromResult(prefix + question);
    }

    public Task<InterviewFeedbackDto> GenerateFeedbackAsync(Interview interview, List<InterviewMessageDto> transcript, CancellationToken cancellationToken = default)
    {
        var isUk = interview.Language.ToLowerInvariant() == "uk";

        var feedbackUk = new InterviewFeedbackDto(
            Summary: $"Кандидат продемонстрував високий рівень підготовки за напрямком {interview.Direction.ToUpperInvariant()} ({interview.Level}). Відповіді логічні, структуровані та демонструють практичний досвід.",
            Score: 82,
            Strengths: new List<string>
            {
                "Впевнене володіння базовою термінологією та архітектурними паттернами",
                "Чітке розуміння протоколів взаємодії та безпеки",
                "Здатність аргументувати вибір технологій та підходів"
            },
            Weaknesses: new List<string>
            {
                "Потребує глибшого вивчення внутрішніх механізмів оптимізації та індексації",
                "Варто приділити увагу обробці крайніх випадків (edge cases) при масштабуванні"
            },
            Recommendations: new List<RecommendationDto>
            {
                new("Database Indexing & Performance", "https://use-the-index-luke.com"),
                new("System Design Primer", "https://github.com/donnemartin/system-design-primer"),
                new("Modern Web & API Architectures", "https://martinfowler.com/architecture")
            },
            QuestionScores: new List<QuestionScoreDto>
            {
                new("Базові протоколи та архітектура", 88, "Відмінна аргументація з практичними прикладами"),
                new("Оптимізація та масштабування", 76, "Добра відповідь, але бракувало деталей щодо розподілених транзакцій")
            }
        );

        var feedbackEn = new InterviewFeedbackDto(
            Summary: $"The candidate demonstrated solid competence in {interview.Direction.ToUpperInvariant()} ({interview.Level}). Explanations were clear, structured, and reflected practical engineering experience.",
            Score: 84,
            Strengths: new List<string>
            {
                "Strong grasp of core architecture patterns and fundamentals",
                "Clear understanding of protocols and system trade-offs",
                "Structured communication and reasoning"
            },
            Weaknesses: new List<string>
            {
                "Could expand deeper into concurrency edge-cases and distributed locking",
                "Could discuss telemetry and observability in more detail"
            },
            Recommendations: new List<RecommendationDto>
            {
                new("Database Indexing Internals", "https://use-the-index-luke.com"),
                new("System Design Primer", "https://github.com/donnemartin/system-design-primer")
            },
            QuestionScores: new List<QuestionScoreDto>
            {
                new("Core protocols and architectural design", 85, "Well structured with clear rationale"),
                new("Scaling & Optimization", 83, "Solid answer covering primary challenges")
            }
        );

        return Task.FromResult(isUk ? feedbackUk : feedbackEn);
    }
}
