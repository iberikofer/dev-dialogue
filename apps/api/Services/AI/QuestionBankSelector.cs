using System.Collections.Concurrent;
using DevDialogue.Api.Data;
using DevDialogue.Api.Models.Entities;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.DependencyInjection;

namespace DevDialogue.Api.Services.AI;

public sealed record SelectedQuestion(string QuestionText, string Topic, string ExpectedAnswer);

public sealed record DebugTraceEntry(DateTime Timestamp, string Stage, string Detail);

/// <summary>
/// Thread-safe ring buffer of recent DB question-selection &amp; AI evaluation events for the Debug panel.
/// </summary>
public static class DebugTraceLog
{
    private static readonly ConcurrentQueue<DebugTraceEntry> Entries = new();

    public static void Add(string stage, string detail)
    {
        Entries.Enqueue(new DebugTraceEntry(DateTime.UtcNow, stage, detail));
        while (Entries.Count > 25 && Entries.TryDequeue(out _)) { }
    }

    public static List<DebugTraceEntry> Snapshot() => Entries.Reverse().ToList();
}

/// <summary>
/// Picks interview questions and reference answers from the active database (SQLite / Postgres / InMemory),
/// falling back to the built-in curated pool when no unused database rows remain.
/// </summary>
public static class QuestionBankSelector
{
    private static readonly Dictionary<string, List<SelectedQuestion>> UkrainianFallback = new()
    {
        ["frontend"] = new()
        {
            new(
                "Поясніть різницю між controlled та uncontrolled компонентами у формах.",
                "React Components & State",
                "У controlled компонентах значення зберігається в React state і змінюється через onChange; у uncontrolled значенням керує сам DOM, а доступ отримують через ref."
            ),
            new(
                "Розкажіть про життєвий цикл компонента в React і як хук useEffect співвідноситься з ним?",
                "React Lifecycle & Hooks",
                "Життєвий цикл охоплює монтування, оновлення та розмонтування; useEffect виконує побічні ефекти після рендеру, реагує на масив залежностей і повертає функцію очищення (cleanup) при оновленні або розмонтуванні."
            ),
            new(
                "Як працює Virtual DOM і за рахунок чого React оптимізує оновлення інтерфейсу?",
                "Virtual DOM & Reconciliation",
                "Virtual DOM — легка копія реального DOM у пам'яті; при зміні стану React будує нове дерево, порівнює його з попереднім (diffing / reconciliation) і оновє в реальному DOM лише змінені вузли."
            ),
            new(
                "Які підходи ви використовуєте для оптимізації рендерингу (useMemo, useCallback, React.memo)?",
                "React Performance",
                "React.memo запобігає повторному рендеру компонента при незмінних props; useMemo кешує результат дорогих обчислень; useCallback мемоїзує посилання на функцію для стабільних props або залежностей."
            ),
            new(
                "Поясніть механізм CSS box model та різницю між display: flex та display: grid.",
                "CSS Layout",
                "Box model складається з content, padding, border та margin (box-sizing: border-box включає padding і border у width/height). Flexbox — одновимірна модель (рядок або стовпець), Grid — двовимірна (рядки і стовпці одночасно)."
            )
        },
        ["backend"] = new()
        {
            new(
                "Поясніть різницю між TCP та UDP. У яких сценаріях UDP є кращим вибором?",
                "Networking & Protocols",
                "TCP встановлює з'єднання, гарантує доставку, порядок пакетів і контроль перевантаження; UDP працює без з'єднання і гарантій, але має мінімальну затримку, тому краще підходить для відеострімінгу, VoIP, онлайн-ігор та DNS."
            ),
            new(
                "Як працюють індекси в базах даних (зокрема B-Tree) та коли індекс може нашкодити продуктивності?",
                "Databases & Indexing",
                "B-Tree індекс зберігає впорядковані ключі у збалансованому дереві для пошуку за O(log N) без повного сканування таблиці; шкодить при частих INSERT/UPDATE/DELETE, бо БД мусить оновлювати індекс і витрачає додаткову пам'ять."
            ),
            new(
                "Що таке Dependency Injection і як налаштовуються Scoped, Transient та Singleton сервіси в ASP.NET Core?",
                "Dependency Injection",
                "DI передає залежності ззовні через контейнер; Transient створюється при кожному запиті сервісу, Scoped — один раз на HTTP-запит, Singleton — один екземпляр на весь час життя застосунку."
            ),
            new(
                "Поясніть принципи REST і різницю між методами PUT та PATCH.",
                "REST API Design",
                "REST базується на ресурсах, stateless-взаємодії та стандартних HTTP-діях/кодах. PUT повністю замінює ресурс (ідемпотентно), а PATCH застосовує часткове оновлення окремих полів."
            ),
            new(
                "Як ви реалізуєте обробку помилок і транзакційність у мікросервісній або модульній системі?",
                "Reliability & Transactions",
                "Використовують глобальний middleware обробки винятків, idempotency keys, retry із backoff, Circuit Breaker та патерни Outbox або Saga для узгодженості між сервісами без розподіленого 2PC."
            )
        },
        ["devops"] = new()
        {
            new(
                "Поясніть різницю між Docker контейнером та віртуальною машиною.",
                "Docker Fundamentals",
                "Контейнер ізолює процеси в user space і ділить спільне ядро хостової ОС (швидкий старт, мала вага), а віртуальна машина емулює апаратне забезпечення через гіпервізор і запускає повноцінну гостьову ОС."
            ),
            new(
                "Як організувати безпечний CI/CD пайплайн з тестуванням та захистом секретів?",
                "CI/CD Security",
                "Автоматичні тести та статичний аналіз (SAST) перед збіркою, секрети лише в захищених сховищах CI (ніколи в коді), принцип найменших привілеїв, перевірка вразливостей образів і затвердження деплою."
            ),
            new(
                "Що таке Kubernetes Pod, Service та Ingress і як вони взаємодіють?",
                "Kubernetes Networking",
                "Pod — найменша одиниця деплою з одним або кількома контейнерами; Service дає стабільну внутрішню IP-адресу та балансування на поди; Ingress керує зовнішнім HTTP/HTTPS-трафіком і маршрутизацією до сервісів."
            ),
            new(
                "Які метрики та інструменти ви використовуєте для моніторингу навантаження систем?",
                "Observability",
                "Золоті сигнали: latency, traffic, errors, saturation (CPU, RAM, disk I/O), а також централізовані логи та трейсинг (Prometheus, Grafana, OpenTelemetry, ELK)."
            ),
            new(
                "Що таке Infrastructure as Code (IaC) і навіщо потрібні Terraform або Ansible?",
                "Infrastructure as Code",
                "IaC описує інфраструктуру декларативно у версіонованому коді (відтворюваність, code review, уникнення ручного дрейфу конфігурацій). Terraform керує створенням ресурсів, Ansible — конфігурацією ОС/сервісів."
            )
        },
        ["qa"] = new()
        {
            new(
                "Яка різниця між регресійним і санітарним (smoke/sanity) тестуванням?",
                "Testing Types",
                "Smoke перевіряє базову працездатність збірки; sanity — точкова перевірка після фіксу конкретного багу/фічі; регресійне тестування перевіряє, що зміни не зламали існуючий функціонал."
            ),
            new(
                "Як ви підходите до складання тест-плану для нового функціоналу авторизації?",
                "Test Planning",
                "Аналіз вимог, перевірка позитивних і негативних сценаріїв (валідні/невалідні дані, блокування, відновлення пароля, токени/сесії), безпеки, кросбраузерності, локалізації та критеріїв входу/виходу."
            ),
            new(
                "Опишіть техніки тест-дизайну: еквівалентне розбиття та аналіз граничних значень.",
                "Test Design",
                "Еквівалентне розбиття ділить вхідні дані на класи, що поводяться однаково; аналіз граничних значень тестує межі цих класів (min-1, min, min+1, max-1, max, max+1), де найчастіше виникають помилки."
            ),
            new(
                "Як правильно скласти баг-репорт і чим відрізняються Severity та Priority?",
                "Bug Reporting",
                "Баг-репорт містить заголовок, кроки відтворення, фактичний і очікуваний результати, середовище та логи/скріншоти. Severity — технічна критичність впливу на систему, Priority — терміновість виправлення для бізнесу."
            ),
            new(
                "Яка різниця між верифікацією та валідацією, і що таке піраміда тестування?",
                "QA Strategy",
                "Верифікація перевіряє чи будуємо продукт правильно (відповідність специфікаціям), валідація — чи правильний продукт будуємо (потреби користувача). Піраміда тестування: багато швидких unit-тестів, менше integration і мінімум повільних E2E."
            )
        },
        ["pm"] = new()
        {
            new(
                "Як ви керуєте ризиками на проекті у випадку зриву дедлайну командою?",
                "Risk Management",
                "Рання ідентифікація блокувань, оцінка критичного шляху, пріоритезація скоупу (MoSCoW/MVP), прозора комунікація зі стейкхолдерами, перерозподіл ресурсів або узгодження нового плану."
            ),
            new(
                "Яка різниця між методологіями Scrum та Kanban і коли яку обирати?",
                "Agile Methodologies",
                "Scrum працює фіксованими спринтами з ролями й церемоніями (добре для продуктової розробки); Kanban — неперервний потік із WIP-лімітами без спринтів (краще для підтримки, операційних задач чи непередбачуваних запитів)."
            ),
            new(
                "Як ви вирішуєте міжособистісні або технічні конфлікти всередині команди?",
                "Conflict Resolution",
                "Індивідуальні 1-on-1 для розуміння позицій, переведення дискусії на факти, цілі продукту й метрики (а не особистості), фасилітація спільного рішення та фіксація домовленостей."
            ),
            new(
                "Які метрики ви використовуєте для оцінки прогресу та здоров'я спринту?",
                "Agile Metrics",
                "Velocity, Sprint Burndown/Burnup, Cycle Time і Lead Time, WIP, процент виконаних задач у спринті та кількість багів/переробок (escaped defects)."
            ),
            new(
                "Як ви пріоритезуєте беклог, коли всі стейкхолдери кажуть, що їхня фіча найважливіша?",
                "Backlog Prioritization",
                "Використовуються об'єктивні фреймворки (RICE, WSJF, MoSCoW) з оцінкою бізнес-цінності, охоплення та вартості розробки (effort), узгоджені зі стратегічними OKR на спільному грумінгу."
            )
        }
    };

    private static readonly Dictionary<string, List<SelectedQuestion>> EnglishFallback = new()
    {
        ["frontend"] = new()
        {
            new(
                "Could you explain the difference between client-side rendering (CSR) and server-side rendering (SSR) in Next.js?",
                "Rendering Strategies",
                "CSR renders UI in the browser via JS after loading a minimal shell; SSR renders HTML per request on the server, improving initial load time (FCP/LCP) and SEO before hydrating on the client."
            ),
            new(
                "How does the browser event loop work, specifically regarding the microtask and macrotask queues?",
                "JavaScript Runtime",
                "The call stack runs synchronous code first; then the event loop drains the entire microtask queue (Promises, queueMicrotask) before picking the next macrotask (setTimeout, I/O, UI events) and rendering."
            ),
            new(
                "How do you approach global state management in modern React applications?",
                "State Management",
                "Separate server cache (React Query/SWR) from client UI state; keep state local when possible and use Context, Zustand, or Redux Toolkit only for shared cross-cutting state."
            ),
            new(
                "What strategies do you employ to improve Core Web Vitals (LCP, INP, CLS)?",
                "Web Performance",
                "Optimize images/fonts and server response for LCP, break up long JS tasks and minimize re-renders for INP, and reserve explicit dimensions for media/ads to prevent layout shifts (CLS)."
            ),
            new(
                "What is the difference between controlled and uncontrolled components in React forms?",
                "React Components & State",
                "Controlled components store value in React state updated via onChange; uncontrolled components keep value in the DOM accessed via ref."
            )
        },
        ["backend"] = new()
        {
            new(
                "Explain the difference between TCP and UDP. When is UDP preferred in real-world distributed systems?",
                "Networking & Protocols",
                "TCP is connection-oriented with guaranteed ordered delivery and congestion control; UDP is connectionless with minimal latency overhead, preferred for video streaming, VoIP, gaming, and DNS."
            ),
            new(
                "How do database transactions work and what are the ACID properties?",
                "Databases & ACID",
                "Transactions group operations into an atomic unit. ACID stands for Atomicity (all or nothing), Consistency (valid invariants), Isolation (concurrent safety), and Durability (persisted after commit)."
            ),
            new(
                "Can you explain the differences between horizontal and vertical scaling for API services?",
                "Scalability",
                "Vertical scaling adds CPU/RAM to a single node (simpler but has hardware limits and SPOF); horizontal scaling adds more instances behind a load balancer (requires stateless services and shared storage)."
            ),
            new(
                "How do asynchronous programming and async/await work under the hood in modern runtimes?",
                "Async Execution",
                "async/await compiles into a state machine that yields the thread during I/O waits and resumes via callbacks/continuations on completion, freeing threads to handle other requests."
            ),
            new(
                "How do database indexes (like B-Tree) work and when can an index hurt performance?",
                "Databases & Indexing",
                "B-Tree indexes keep sorted keys for O(log N) lookups without full table scans; they slow down INSERT/UPDATE/DELETE operations and consume extra memory/storage."
            )
        },
        ["devops"] = new()
        {
            new(
                "What is the difference between container virtualization and hypervisor virtualization?",
                "Containers vs VMs",
                "Containers isolate user-space processes sharing the host OS kernel (fast, lightweight); VMs emulate hardware via a hypervisor and run a full guest OS kernel."
            ),
            new(
                "How do you design a zero-downtime rolling deployment strategy in Kubernetes?",
                "Kubernetes Deployments",
                "Use readiness/liveness probes, RollingUpdate strategy with maxUnavailable=0 and maxSurge>=1, PodDisruptionBudgets, and graceful shutdown (SIGTERM handling)."
            ),
            new(
                "What practices do you recommend for Infrastructure as Code (IaC) governance?",
                "IaC Governance",
                "Version control with PR reviews, remote locked state, modular reusable templates, automated drift detection, and policy-as-code checks before apply."
            )
        },
        ["qa"] = new()
        {
            new(
                "What is the difference between functional and non-functional testing?",
                "Testing Fundamentals",
                "Functional testing verifies what the system does against requirements (features, APIs, workflows); non-functional testing verifies how well it performs (load, security, usability, reliability)."
            ),
            new(
                "How do you design an automated integration test suite for REST APIs?",
                "API Automation",
                "Test contracts, status codes, auth, validation, and DB persistence using isolated test databases/containers, deterministic seed data, and cleanup between tests."
            )
        },
        ["pm"] = new()
        {
            new(
                "How do you prioritize backlog items when stakeholders have competing requirements?",
                "Backlog Prioritization",
                "Align on business goals and impact using frameworks like RICE or MoSCoW, make trade-offs transparent with capacity data, and facilitate agreement among stakeholders."
            ),
            new(
                "How do you identify and mitigate technical debt during rapid iteration cycles?",
                "Tech Debt Management",
                "Track debt explicitly in the backlog with business impact (velocity drag, outages), allocate a consistent capacity percentage (e.g., 15-20%) per sprint, and pair refactoring with related features."
            )
        }
    };

    public static IReadOnlyList<QuestionBank> BuildDefaultSeedQuestions()
    {
        var list = new List<QuestionBank>();
        foreach (var (direction, items) in UkrainianFallback)
        {
            foreach (var item in items)
            {
                list.Add(new QuestionBank
                {
                    Direction = direction,
                    Level = "junior",
                    Stage = "hard_skills",
                    Topic = item.Topic,
                    QuestionText = item.QuestionText,
                    ExpectedAnswer = item.ExpectedAnswer,
                    IsActive = true
                });
            }
        }
        return list;
    }

    public static async Task<SelectedQuestion> PickNextQuestionAsync(
        IServiceScopeFactory? scopeFactory,
        string direction,
        string level,
        string language,
        string? topicHint,
        IReadOnlyCollection<string> askedAssistantTexts,
        int fallbackIndex,
        CancellationToken cancellationToken = default)
    {
        var directionKey = direction.ToLowerInvariant();
        var levelKey = level.ToLowerInvariant();
        var isUk = language.Equals("uk", StringComparison.OrdinalIgnoreCase);

        if (scopeFactory != null)
        {
            try
            {
                using var scope = scopeFactory.CreateScope();
                var db = scope.ServiceProvider.GetService<AppDbContext>();
                if (db != null)
                {
                    var candidates = await db.QuestionsBank
                        .AsNoTracking()
                        .Where(q => q.IsActive && q.Direction == directionKey)
                        .ToListAsync(cancellationToken);

                    var unused = candidates
                        .Where(q => !string.IsNullOrWhiteSpace(q.QuestionText)
                            && !askedAssistantTexts.Any(asked => asked.Contains(q.QuestionText, StringComparison.OrdinalIgnoreCase)))
                        .ToList();

                    if (unused.Count > 0)
                    {
                        var levelMatched = unused.Where(q => q.Level == levelKey).ToList();
                        var poolToPickFrom = levelMatched.Count > 0 ? levelMatched : unused;

                        QuestionBank picked;
                        if (!string.IsNullOrWhiteSpace(topicHint))
                        {
                            picked = poolToPickFrom.FirstOrDefault(q => string.Equals(q.Topic, topicHint, StringComparison.OrdinalIgnoreCase))
                                ?? poolToPickFrom[Random.Shared.Next(poolToPickFrom.Count)];
                        }
                        else
                        {
                            picked = poolToPickFrom[Random.Shared.Next(poolToPickFrom.Count)];
                        }

                        var expected = !string.IsNullOrWhiteSpace(picked.ExpectedAnswer)
                            ? picked.ExpectedAnswer!
                            : ResolveDefaultExpectedAnswer(picked.QuestionText, picked.Topic, isUk);

                        var provider = db.Database.ProviderName ?? "Unknown";
                        DebugTraceLog.Add("DB Question Pick", $"[{provider}] {directionKey}/{levelKey}: picked '{picked.Topic}' ({poolToPickFrom.Count} unused of {candidates.Count} DB rows)");

                        return new SelectedQuestion(picked.QuestionText, picked.Topic, expected);
                    }
                }
            }
            catch (Exception ex)
            {
                DebugTraceLog.Add("DB Question Error", ex.Message);
            }
        }

        var pool = GetFallbackPool(directionKey, isUk);
        var unusedPool = pool
            .Where(q => !askedAssistantTexts.Any(asked => asked.Contains(q.QuestionText, StringComparison.OrdinalIgnoreCase)))
            .ToList();

        var chosen = unusedPool.Count > 0
            ? unusedPool[Random.Shared.Next(unusedPool.Count)]
            : pool[Math.Abs(fallbackIndex) % pool.Count];

        DebugTraceLog.Add("Fallback Question Pick", $"{directionKey}/{levelKey}: picked '{chosen.Topic}' from fallback pool");
        return chosen;
    }

    public static async Task<SelectedQuestion?> FindReferenceForAskedQuestionAsync(
        IServiceScopeFactory? scopeFactory,
        string direction,
        string language,
        string lastAssistantMessage,
        CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(lastAssistantMessage))
        {
            return null;
        }

        var directionKey = direction.ToLowerInvariant();
        var isUk = language.Equals("uk", StringComparison.OrdinalIgnoreCase);

        if (scopeFactory != null)
        {
            try
            {
                using var scope = scopeFactory.CreateScope();
                var db = scope.ServiceProvider.GetService<AppDbContext>();
                if (db != null)
                {
                    var dbRows = await db.QuestionsBank
                        .AsNoTracking()
                        .Where(q => q.Direction == directionKey)
                        .ToListAsync(cancellationToken);

                    var match = dbRows.FirstOrDefault(q =>
                        !string.IsNullOrWhiteSpace(q.QuestionText)
                        && lastAssistantMessage.Contains(q.QuestionText, StringComparison.OrdinalIgnoreCase));

                    if (match != null)
                    {
                        var expected = !string.IsNullOrWhiteSpace(match.ExpectedAnswer)
                            ? match.ExpectedAnswer!
                            : ResolveDefaultExpectedAnswer(match.QuestionText, match.Topic, isUk);
                        return new SelectedQuestion(match.QuestionText, match.Topic, expected);
                    }
                }
            }
            catch
            {
                // Continue to curated fallback lookup
            }
        }

        var pool = GetFallbackPool(directionKey, isUk);
        return pool.FirstOrDefault(q =>
            lastAssistantMessage.Contains(q.QuestionText, StringComparison.OrdinalIgnoreCase));
    }

    private static List<SelectedQuestion> GetFallbackPool(string directionKey, bool isUk)
    {
        var dict = isUk ? UkrainianFallback : EnglishFallback;
        if (!dict.TryGetValue(directionKey, out var list) || list.Count == 0)
        {
            list = dict["backend"];
        }
        return list;
    }

    private static string ResolveDefaultExpectedAnswer(string questionText, string topic, bool isUk)
    {
        var all = (isUk ? UkrainianFallback : EnglishFallback).Values.SelectMany(v => v);
        var match = all.FirstOrDefault(q =>
            string.Equals(q.Topic, topic, StringComparison.OrdinalIgnoreCase)
            || q.QuestionText.Contains(questionText, StringComparison.OrdinalIgnoreCase));

        if (match != null)
        {
            return match.ExpectedAnswer;
        }

        return isUk
            ? $"Чітке технічне пояснення теми «{topic}» з прикладами застосування та ключовими компромісами."
            : $"A clear technical explanation of '{topic}' with practical examples and key trade-offs.";
    }
}
