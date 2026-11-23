#include "services/DemoPreparationService.hpp"
#include <QTimer>
#include <algorithm>

namespace devdialogue::services {

DemoPreparationService::DemoPreparationService(QObject* parent)
    : QObject(parent) {}

void DemoPreparationService::analyzeProfile(
    const QString& direction,
    const QString& level,
    const QString& userDescription,
    const QString& language,
    std::function<void(const models::PreparationPlanResult& result, const QString& error)> callback
) {
    if (!callback) return;

    // Artificial 1000ms delay to give a natural feeling of deep profile analysis
    QTimer::singleShot(1000, this, [this, direction, level, userDescription, language, callback]() {
        models::PreparationPlanResult result = evaluateSynchronously(direction, level, userDescription, language);
        callback(result, QString());
    });
}

models::TopicPriority DemoPreparationService::adjustPriority(
    models::TopicPriority base,
    bool isFundamental,
    bool isAdvanced,
    const QString& level
) const {
    const QString lvl = level.trimmed().toLower();
    if (lvl == QStringLiteral("trainee")) {
        if (isFundamental) return models::TopicPriority::High;
        if (isAdvanced) return models::TopicPriority::Low;
    } else if (lvl == QStringLiteral("middle")) {
        if (isAdvanced) return models::TopicPriority::High;
        if (isFundamental) return models::TopicPriority::Low;
    }
    return base;
}

models::PreparationTopicStatus DemoPreparationService::evaluateStatus(
    const QString& descriptionLower,
    const QStringList& keywords,
    QString& outReason,
    bool isUk
) const {
    static const QStringList ukNegations = {
        QStringLiteral("не "), QStringLiteral("ні "), QStringLiteral("без "), QStringLiteral("немає"),
        QStringLiteral("не маю"), QStringLiteral("не вмію"), QStringLiteral("не знаю"), QStringLiteral("не вчив"),
        QStringLiteral("не вчила"), QStringLiteral("не володію"), QStringLiteral("не працював"), QStringLiteral("не працювала"),
        QStringLiteral("не доводилось"), QStringLiteral("не доводилося"), QStringLiteral("жодного досвіду"),
        QStringLiteral("нуль досвіду"), QStringLiteral("не використовував"), QStringLiteral("не застосовував")
    };

    static const QStringList enNegations = {
        QStringLiteral("not "), QStringLiteral("no "), QStringLiteral("never "), QStringLiteral("don't "),
        QStringLiteral("dont "), QStringLiteral("didn't "), QStringLiteral("didnt "), QStringLiteral("haven't "),
        QStringLiteral("havent "), QStringLiteral("without "), QStringLiteral("lack "), QStringLiteral("no experience"),
        QStringLiteral("zero experience"), QStringLiteral("never used"), QStringLiteral("never worked"), QStringLiteral("unfamiliar with")
    };

    static const QStringList ukQualifiers = {
        QStringLiteral("трохи"), QStringLiteral("трішки"), QStringLiteral("базово"), QStringLiteral("поверхнево"),
        QStringLiteral("початківець"), QStringLiteral("лише чув"), QStringLiteral("чула"), QStringLiteral("знайомий лише"),
        QStringLiteral("тільки починаю"), QStringLiteral("мінімально")
    };

    static const QStringList enQualifiers = {
        QStringLiteral("a bit"), QStringLiteral("little"), QStringLiteral("basic"), QStringLiteral("basics"),
        QStringLiteral("beginner"), QStringLiteral("heard of"), QStringLiteral("surface"), QStringLiteral("just started"),
        QStringLiteral("elementary"), QStringLiteral("minimal")
    };

    int bestMatchPos = -1;
    QString matchedKeyword;

    for (const QString& kw : keywords) {
        int pos = descriptionLower.indexOf(kw);
        if (pos != -1) {
            bestMatchPos = pos;
            matchedKeyword = kw;
            break;
        }
    }

    if (bestMatchPos == -1) {
        outReason = isUk ? QStringLiteral("У вашому описі це не згадано.")
                         : QStringLiteral("Not mentioned in your description.");
        return models::PreparationTopicStatus::NotMentioned;
    }

    // Inspect prefix window (~45 characters before keyword) for negations or qualifiers
    int windowStart = std::max(0, bestMatchPos - 45);
    QString prefixWindow = descriptionLower.mid(windowStart, bestMatchPos - windowStart);

    // 1. Check negation
    bool hasNegation = false;
    for (const QString& neg : ukNegations) {
        if (prefixWindow.contains(neg)) { hasNegation = true; break; }
    }
    if (!hasNegation) {
        for (const QString& neg : enNegations) {
            if (prefixWindow.contains(neg)) { hasNegation = true; break; }
        }
    }

    if (hasNegation) {
        outReason = isUk ? QStringLiteral("У вашому описі зазначено відсутність досвіду з цієї теми.")
                         : QStringLiteral("Your description indicates a lack of experience with this topic.");
        return models::PreparationTopicStatus::NotMentioned;
    }

    // 2. Check qualifier/unclear
    bool hasQualifier = false;
    for (const QString& qual : ukQualifiers) {
        if (prefixWindow.contains(qual)) { hasQualifier = true; break; }
    }
    if (!hasQualifier) {
        for (const QString& qual : enQualifiers) {
            if (prefixWindow.contains(qual)) { hasQualifier = true; break; }
        }
    }

    if (hasQualifier) {
        outReason = isUk ? QStringLiteral("Ви згадали поверхневе або базове знайомство з цією темою.")
                         : QStringLiteral("You mentioned a surface-level or tentative acquaintance with this topic.");
        return models::PreparationTopicStatus::Unclear;
    }

    // 3. Confident mention
    outReason = isUk ? QStringLiteral("У вашому описі зазначено володіння цією темою.")
                     : QStringLiteral("Your description indicates practical familiarity with this topic.");
    return models::PreparationTopicStatus::Known;
}

QVector<DemoPreparationService::TopicDefinition> DemoPreparationService::getDefinitionsForDirection(const QString& direction) const {
    const QString dir = direction.trimmed().toLower();
    QVector<TopicDefinition> list;

    if (dir == QStringLiteral("frontend")) {
        list.append({
            QStringLiteral("fe-html-css"),
            QStringLiteral("Semantic HTML5 & Modern CSS Layouts"), QStringLiteral("Семантичний HTML5 та сучасні CSS-макети"),
            QStringLiteral("Core Web"), QStringLiteral("Базовий веб"),
            models::TopicPriority::High,
            {QStringLiteral("html"), QStringLiteral("css"), QStringLiteral("flexbox"), QStringLiteral("grid"), QStringLiteral("гріди"), QStringLiteral("флекси"), QStringLiteral("семантика")},
            {{QStringLiteral("MDN Web Docs: HTML & CSS"), QStringLiteral("https://developer.mozilla.org/docs/Web/HTML")}},
            {{QStringLiteral("MDN Web Docs: Довідник з HTML та CSS"), QStringLiteral("https://developer.mozilla.org/docs/Web/HTML")}},
            false, true
        });
        list.append({
            QStringLiteral("fe-js-ts"),
            QStringLiteral("TypeScript & Modern JavaScript (ES6+, Async)"), QStringLiteral("TypeScript та сучасний JavaScript (ES6+, Async)"),
            QStringLiteral("Language"), QStringLiteral("Мова програмування"),
            models::TopicPriority::High,
            {QStringLiteral("typescript"), QStringLiteral("тайпскрипт"), QStringLiteral("javascript"), QStringLiteral("джаваскрипт"), QStringLiteral("js"), QStringLiteral("ts"), QStringLiteral("es6"), QStringLiteral("async/await"), QStringLiteral("promise")},
            {{QStringLiteral("TypeScript Handbook"), QStringLiteral("https://www.typescriptlang.org/docs/handbook/intro.html")}},
            {{QStringLiteral("Документація TypeScript"), QStringLiteral("https://www.typescriptlang.org/docs/handbook/intro.html")}},
            false, true
        });
        list.append({
            QStringLiteral("fe-react-core"),
            QStringLiteral("React Framework, Hooks & Component Lifecycle"), QStringLiteral("React: хуки, стан та життєвий цикл компонентів"),
            QStringLiteral("Framework"), QStringLiteral("Фреймворк"),
            models::TopicPriority::High,
            {QStringLiteral("react"), QStringLiteral("реакт"), QStringLiteral("reactjs"), QStringLiteral("hooks"), QStringLiteral("хуки"), QStringLiteral("useeffect"), QStringLiteral("usestate")},
            {{QStringLiteral("React Official Documentation"), QStringLiteral("https://react.dev/learn")}},
            {{QStringLiteral("Офіційний посібник React"), QStringLiteral("https://react.dev/learn")}},
            false, true
        });
        list.append({
            QStringLiteral("fe-nextjs"),
            QStringLiteral("Next.js App Router & Server Components (SSR/SSG)"), QStringLiteral("Next.js: App Router, серверні компоненти (SSR/SSG)"),
            QStringLiteral("Framework"), QStringLiteral("Фреймворк"),
            models::TopicPriority::Medium,
            {QStringLiteral("next.js"), QStringLiteral("nextjs"), QStringLiteral("некст"), QStringLiteral("ssr"), QStringLiteral("ssg"), QStringLiteral("server components")},
            {{QStringLiteral("Next.js Documentation"), QStringLiteral("https://nextjs.org/docs")}},
            {{QStringLiteral("Документація Next.js"), QStringLiteral("https://nextjs.org/docs")}},
            true, false
        });
        list.append({
            QStringLiteral("fe-state-mgmt"),
            QStringLiteral("Global State Management (Zustand, Redux Toolkit)"), QStringLiteral("Керування глобальним станом (Zustand, Redux Toolkit)"),
            QStringLiteral("Architecture"), QStringLiteral("Архітектура"),
            models::TopicPriority::Medium,
            {QStringLiteral("zustand"), QStringLiteral("redux"), QStringLiteral("зустанд"), QStringLiteral("редакс"), QStringLiteral("state management"), QStringLiteral("керування станом")},
            {{QStringLiteral("Redux Toolkit Essentials"), QStringLiteral("https://redux.js.org/introduction/getting-started")}},
            {{QStringLiteral("Посібник з Redux Toolkit"), QStringLiteral("https://redux.js.org/introduction/getting-started")}},
            false, false
        });
        list.append({
            QStringLiteral("fe-performance"),
            QStringLiteral("Web Performance, Core Web Vitals & DevTools"), QStringLiteral("Оптимізація продуктивності вебу та Core Web Vitals"),
            QStringLiteral("Performance"), QStringLiteral("Продуктивність"),
            models::TopicPriority::Low,
            {QStringLiteral("performance"), QStringLiteral("продуктивність"), QStringLiteral("lighthouse"), QStringLiteral("лайтхаус"), QStringLiteral("core web vitals"), QStringLiteral("devtools"), QStringLiteral("bundle size")},
            {{QStringLiteral("web.dev Performance Overview"), QStringLiteral("https://web.dev/explore/fast")}},
            {{QStringLiteral("web.dev: Оптимізація швидкодії веб-сторінок"), QStringLiteral("https://web.dev/explore/fast")}},
            true, false
        });
        list.append({
            QStringLiteral("fe-testing"),
            QStringLiteral("Frontend Unit & Integration Testing (Vitest, RTL)"), QStringLiteral("Модульне тестування інтерфейсу (Vitest, Testing Library)"),
            QStringLiteral("Testing"), QStringLiteral("Тестування"),
            models::TopicPriority::Medium,
            {QStringLiteral("vitest"), QStringLiteral("jest"), QStringLiteral("джест"), QStringLiteral("testing library"), QStringLiteral("rtl"), QStringLiteral("юніт тести")},
            {{QStringLiteral("React Testing Library Docs"), QStringLiteral("https://testing-library.com/docs/react-testing-library/intro/")}},
            {{QStringLiteral("Документація React Testing Library"), QStringLiteral("https://testing-library.com/docs/react-testing-library/intro/")}},
            false, false
        });
        list.append({
            QStringLiteral("fe-build-tools"),
            QStringLiteral("Modern Build Tooling (Vite, Package Managers)"), QStringLiteral("Інструменти збірки проекту (Vite, пакетні менеджери)"),
            QStringLiteral("Tooling"), QStringLiteral("Інструменти"),
            models::TopicPriority::Low,
            {QStringLiteral("vite"), QStringLiteral("вайт"), QStringLiteral("webpack"), QStringLiteral("вебпак"), QStringLiteral("npm"), QStringLiteral("pnpm"), QStringLiteral("yarn")},
            {{QStringLiteral("Vite Guide"), QStringLiteral("https://vitejs.dev/guide/")}},
            {{QStringLiteral("Посібник з Vite"), QStringLiteral("https://vitejs.dev/guide/")}},
            false, false
        });
    } else if (dir == QStringLiteral("backend")) {
        list.append({
            QStringLiteral("be-csharp"),
            QStringLiteral("C# Language Semantics, Memory & Modern OOP"), QStringLiteral("Мова C#: семантика, керування пам'яттю та сучасне ООП"),
            QStringLiteral("Language"), QStringLiteral("Мова програмування"),
            models::TopicPriority::High,
            {QStringLiteral("c#"), QStringLiteral("сі шарп"), QStringLiteral("csharp"), QStringLiteral(".net"), QStringLiteral("дотнет"), QStringLiteral("dotnet"), QStringLiteral("oop"), QStringLiteral("ооп")},
            {{QStringLiteral("Microsoft Learn: C# Documentation"), QStringLiteral("https://learn.microsoft.com/dotnet/csharp/")}},
            {{QStringLiteral("Microsoft Learn: Документація C#"), QStringLiteral("https://learn.microsoft.com/dotnet/csharp/")}},
            false, true
        });
        list.append({
            QStringLiteral("be-aspnet"),
            QStringLiteral("ASP.NET Core Architecture & RESTful API Design"), QStringLiteral("Архітектура ASP.NET Core та проектування REST API"),
            QStringLiteral("Framework"), QStringLiteral("Фреймворк"),
            models::TopicPriority::High,
            {QStringLiteral("asp.net"), QStringLiteral("aspnet"), QStringLiteral("асп.нет"), QStringLiteral("rest api"), QStringLiteral("рест"), QStringLiteral("restful"), QStringLiteral("web api"), QStringLiteral("контролери")},
            {{QStringLiteral("Microsoft Learn: ASP.NET Core"), QStringLiteral("https://learn.microsoft.com/aspnet/core/")}},
            {{QStringLiteral("Microsoft Learn: Розробка в ASP.NET Core"), QStringLiteral("https://learn.microsoft.com/aspnet/core/")}},
            false, true
        });
        list.append({
            QStringLiteral("be-sql-db"),
            QStringLiteral("Relational Databases, SQL & Transactions (PostgreSQL)"), QStringLiteral("Реляційні бази даних, SQL та транзакції (PostgreSQL)"),
            QStringLiteral("Databases"), QStringLiteral("Бази даних"),
            models::TopicPriority::High,
            {QStringLiteral("sql"), QStringLiteral("скл"), QStringLiteral("postgres"), QStringLiteral("postgresql"), QStringLiteral("постгрес"), QStringLiteral("database"), QStringLiteral("база даних"), QStringLiteral("індекси"), QStringLiteral("indexes")},
            {{QStringLiteral("PostgreSQL Documentation"), QStringLiteral("https://www.postgresql.org/docs/current/")}},
            {{QStringLiteral("Офіційна документація PostgreSQL"), QStringLiteral("https://www.postgresql.org/docs/current/")}},
            false, true
        });
        list.append({
            QStringLiteral("be-efcore"),
            QStringLiteral("Entity Framework Core & Query Optimization"), QStringLiteral("Entity Framework Core та оптимізація запитів"),
            QStringLiteral("Databases"), QStringLiteral("Бази даних"),
            models::TopicPriority::High,
            {QStringLiteral("ef core"), QStringLiteral("efcore"), QStringLiteral("entity framework"), QStringLiteral("ефкор"), QStringLiteral("ентіті"), QStringLiteral("orm"), QStringLiteral("орм"), QStringLiteral("миграції")},
            {{QStringLiteral("Microsoft Learn: Entity Framework Core"), QStringLiteral("https://learn.microsoft.com/ef/core/")}},
            {{QStringLiteral("Microsoft Learn: Посібник з EF Core"), QStringLiteral("https://learn.microsoft.com/ef/core/")}},
            false, false
        });
        list.append({
            QStringLiteral("be-async"),
            QStringLiteral("Asynchronous & Multithreaded Programming (async/await)"), QStringLiteral("Асинхронне та багатопотокове програмування (async/await)"),
            QStringLiteral("Concurrency"), QStringLiteral("Багатопотоковість"),
            models::TopicPriority::Medium,
            {QStringLiteral("async/await"), QStringLiteral("асинхрон"), QStringLiteral("multithreading"), QStringLiteral("багатопотоковість"), QStringLiteral("tasks"), QStringLiteral("cancellationtoken"), QStringLiteral("deadlock")},
            {{QStringLiteral("Asynchronous Programming in C#"), QStringLiteral("https://learn.microsoft.com/dotnet/csharp/asynchronous-programming/")}},
            {{QStringLiteral("Асинхронне програмування в .NET"), QStringLiteral("https://learn.microsoft.com/dotnet/csharp/asynchronous-programming/")}},
            true, false
        });
        list.append({
            QStringLiteral("be-docker"),
            QStringLiteral("Containerization & Development Environments (Docker)"), QStringLiteral("Контейнеризація сервісів (Docker, Docker Compose)"),
            QStringLiteral("DevOps & Infra"), QStringLiteral("Інфраструктура"),
            models::TopicPriority::Medium,
            {QStringLiteral("docker"), QStringLiteral("докер"), QStringLiteral("container"), QStringLiteral("контейнер"), QStringLiteral("dockerfile"), QStringLiteral("docker compose")},
            {{QStringLiteral("Docker Documentation"), QStringLiteral("https://docs.docker.com/get-started/")}},
            {{QStringLiteral("Посібник з початку роботи з Docker"), QStringLiteral("https://docs.docker.com/get-started/")}},
            false, false
        });
        list.append({
            QStringLiteral("be-testing"),
            QStringLiteral("Automated Testing & Mocking (xUnit, NSubstitute)"), QStringLiteral("Автоматизоване тестування бекенду (xUnit, NSubstitute)"),
            QStringLiteral("Testing"), QStringLiteral("Тестування"),
            models::TopicPriority::Medium,
            {QStringLiteral("xunit"), QStringLiteral("nunit"), QStringLiteral("юніт"), QStringLiteral("інтеграційні тести"), QStringLiteral("тести"), QStringLiteral("unit test"), QStringLiteral("mock"), QStringLiteral("моки")},
            {{QStringLiteral("Unit Testing Best Practices in .NET"), QStringLiteral("https://learn.microsoft.com/dotnet/core/testing/unit-testing-best-practices")}},
            {{QStringLiteral("Практики модульного тестування в .NET"), QStringLiteral("https://learn.microsoft.com/dotnet/core/testing/unit-testing-best-practices")}},
            false, false
        });
        list.append({
            QStringLiteral("be-security"),
            QStringLiteral("API Security, Authentication & JWT Authorization"), QStringLiteral("Безпека API, автентифікація та авторизація (JWT)"),
            QStringLiteral("Security"), QStringLiteral("Безпека"),
            models::TopicPriority::Low,
            {QStringLiteral("jwt"), QStringLiteral("жвт"), QStringLiteral("auth"), QStringLiteral("авторизація"), QStringLiteral("автентифікація"), QStringLiteral("oauth"), QStringLiteral("cors"), QStringLiteral("security")},
            {{QStringLiteral("Overview of ASP.NET Core Security"), QStringLiteral("https://learn.microsoft.com/aspnet/core/security/")}},
            {{QStringLiteral("Огляд безпеки в ASP.NET Core"), QStringLiteral("https://learn.microsoft.com/aspnet/core/security/")}},
            true, false
        });
    } else if (dir == QStringLiteral("devops")) {
        list.append({
            QStringLiteral("do-linux"),
            QStringLiteral("Linux OS Administration & Shell Scripting (Bash)"), QStringLiteral("Адміністрування Linux та скрипти командного рядка (Bash)"),
            QStringLiteral("Operating Systems"), QStringLiteral("Операційні системи"),
            models::TopicPriority::High,
            {QStringLiteral("linux"), QStringLiteral("лінукс"), QStringLiteral("bash"), QStringLiteral("баш"), QStringLiteral("shell"), QStringLiteral("шелл"), QStringLiteral("ssh"), QStringLiteral("systemd")},
            {{QStringLiteral("Ubuntu Command Line Guide"), QStringLiteral("https://ubuntu.com/tutorials/command-line-for-beginners")}},
            {{QStringLiteral("Посібник з командного рядка Linux"), QStringLiteral("https://ubuntu.com/tutorials/command-line-for-beginners")}},
            false, true
        });
        list.append({
            QStringLiteral("do-docker"),
            QStringLiteral("Container Architecture & Optimization (Docker)"), QStringLiteral("Архітектура та оптимізація контейнерів (Docker)"),
            QStringLiteral("Containers"), QStringLiteral("Контейнери"),
            models::TopicPriority::High,
            {QStringLiteral("docker"), QStringLiteral("докер"), QStringLiteral("dockerfile"), QStringLiteral("multi-stage"), QStringLiteral("контейнеризація")},
            {{QStringLiteral("Docker Best Practices"), QStringLiteral("https://docs.docker.com/develop/develop-images/guidelines/")}},
            {{QStringLiteral("Практики створення Docker-образів"), QStringLiteral("https://docs.docker.com/develop/develop-images/guidelines/")}},
            false, true
        });
        list.append({
            QStringLiteral("do-k8s"),
            QStringLiteral("Container Orchestration & Workloads (Kubernetes)"), QStringLiteral("Оркестрація контейнерів (Kubernetes, Helm)"),
            QStringLiteral("Orchestration"), QStringLiteral("Оркестрація"),
            models::TopicPriority::High,
            {QStringLiteral("kubernetes"), QStringLiteral("k8s"), QStringLiteral("кубер"), QStringLiteral("кубернетіс"), QStringLiteral("helm"), QStringLiteral("хелм"), QStringLiteral("pods"), QStringLiteral("deployment")},
            {{QStringLiteral("Kubernetes Core Concepts"), QStringLiteral("https://kubernetes.io/docs/concepts/")}},
            {{QStringLiteral("Основи концепцій Kubernetes"), QStringLiteral("https://kubernetes.io/docs/concepts/")}},
            true, false
        });
        list.append({
            QStringLiteral("do-cicd"),
            QStringLiteral("Continuous Integration & Deployment (GitHub Actions)"), QStringLiteral("Автоматизація CI/CD процесів (GitHub Actions)"),
            QStringLiteral("CI/CD"), QStringLiteral("Автоматизація"),
            models::TopicPriority::High,
            {QStringLiteral("ci/cd"), QStringLiteral("cicd"), QStringLiteral("сіай"), QStringLiteral("github actions"), QStringLiteral("gitlab"), QStringLiteral("гітхаб екшнс"), QStringLiteral("пайплайн"), QStringLiteral("pipeline")},
            {{QStringLiteral("GitHub Actions Documentation"), QStringLiteral("https://docs.github.com/actions")}},
            {{QStringLiteral("Документація GitHub Actions"), QStringLiteral("https://docs.github.com/actions")}},
            false, false
        });
        list.append({
            QStringLiteral("do-iac"),
            QStringLiteral("Infrastructure as Code (Terraform, CloudFormation)"), QStringLiteral("Інфраструктура як код (Terraform)"),
            QStringLiteral("Infrastructure"), QStringLiteral("Інфраструктура"),
            models::TopicPriority::Medium,
            {QStringLiteral("terraform"), QStringLiteral("тераформ"), QStringLiteral("iac"), QStringLiteral("cloudformation"), QStringLiteral("інфраструктура як код")},
            {{QStringLiteral("Terraform Intro Tutorials"), QStringLiteral("https://developer.hashicorp.com/terraform/intro")}},
            {{QStringLiteral("Основи Terraform від HashiCorp"), QStringLiteral("https://developer.hashicorp.com/terraform/intro")}},
            true, false
        });
        list.append({
            QStringLiteral("do-monitoring"),
            QStringLiteral("Observability, Logging & Metrics (Prometheus, Grafana)"), QStringLiteral("Моніторинг та збір метрик (Prometheus, Grafana)"),
            QStringLiteral("Observability"), QStringLiteral("Моніторинг"),
            models::TopicPriority::Low,
            {QStringLiteral("prometheus"), QStringLiteral("прометеус"), QStringLiteral("grafana"), QStringLiteral("графана"), QStringLiteral("loki"), QStringLiteral("моніторинг"), QStringLiteral("метрики")},
            {{QStringLiteral("Prometheus Overview"), QStringLiteral("https://prometheus.io/docs/introduction/overview/")}},
            {{QStringLiteral("Огляд Prometheus"), QStringLiteral("https://prometheus.io/docs/introduction/overview/")}},
            true, false
        });
        list.append({
            QStringLiteral("do-cloud"),
            QStringLiteral("Cloud Providers Fundamentals (AWS / Azure / GCP)"), QStringLiteral("Хмарні провайдери та сервіси (AWS / Azure / GCP)"),
            QStringLiteral("Cloud"), QStringLiteral("Хмарні сервіси"),
            models::TopicPriority::Medium,
            {QStringLiteral("aws"), QStringLiteral("амазон"), QStringLiteral("azure"), QStringLiteral("ажур"), QStringLiteral("gcp"), QStringLiteral("хмара"), QStringLiteral("cloud"), QStringLiteral("s3"), QStringLiteral("ec2")},
            {{QStringLiteral("AWS Cloud Essentials"), QStringLiteral("https://aws.amazon.com/getting-started/")}},
            {{QStringLiteral("Основи роботи з хмарою AWS"), QStringLiteral("https://aws.amazon.com/getting-started/")}},
            false, false
        });
        list.append({
            QStringLiteral("do-networking"),
            QStringLiteral("Network Fundamentals (DNS, TCP/IP, Reverse Proxies)"), QStringLiteral("Мережеві протоколи, DNS та Nginx"),
            QStringLiteral("Networking"), QStringLiteral("Мережі"),
            models::TopicPriority::Medium,
            {QStringLiteral("nginx"), QStringLiteral("енджінікс"), QStringLiteral("dns"), QStringLiteral("днс"), QStringLiteral("ssl"), QStringLiteral("tls"), QStringLiteral("tcp/ip"), QStringLiteral("reverse proxy")},
            {{QStringLiteral("NGINX Beginner's Guide"), QStringLiteral("https://nginx.org/en/docs/beginners_guide.html")}},
            {{QStringLiteral("Посібник для початківців NGINX"), QStringLiteral("https://nginx.org/en/docs/beginners_guide.html")}},
            false, true
        });
    } else if (dir == QStringLiteral("qa")) {
        list.append({
            QStringLiteral("qa-fundamentals"),
            QStringLiteral("Software Testing Principles & Test Levels"), QStringLiteral("Теорія тестування: рівні, види та принципи"),
            QStringLiteral("Fundamentals"), QStringLiteral("Основи"),
            models::TopicPriority::High,
            {QStringLiteral("теорія тестування"), QStringLiteral("testing fundamentals"), QStringLiteral("smoke"), QStringLiteral("regression"), QStringLiteral("регрес"), QStringLiteral("смоук"), QStringLiteral("тест дизайн")},
            {{QStringLiteral("ISTQB Certified Tester Syllabus"), QStringLiteral("https://www.istqb.org/certifications/certified-tester-foundation-level")}},
            {{QStringLiteral("Програма сертифікації ISTQB"), QStringLiteral("https://www.istqb.org/certifications/certified-tester-foundation-level")}},
            false, true
        });
        list.append({
            QStringLiteral("qa-artifacts"),
            QStringLiteral("Test Artifacts (Test Cases, Checklists, Bug Reports)"), QStringLiteral("Тестова документація: тест-кейси, чек-листи, баг-репорти"),
            QStringLiteral("Documentation"), QStringLiteral("Документація"),
            models::TopicPriority::High,
            {QStringLiteral("test case"), QStringLiteral("тест кейс"), QStringLiteral("тест-кейс"), QStringLiteral("bug report"), QStringLiteral("баг репорт"), QStringLiteral("чек-лист"), QStringLiteral("checklist")},
            {{QStringLiteral("Writing Effective Bug Reports"), QStringLiteral("https://www.softwaretestinghelp.com/how-to-write-good-bug-report/")}},
            {{QStringLiteral("Складання ефективних звітів про дефекти"), QStringLiteral("https://www.softwaretestinghelp.com/how-to-write-good-bug-report/")}},
            false, true
        });
        list.append({
            QStringLiteral("qa-api"),
            QStringLiteral("REST API Testing (Postman, Status Codes, JSON Schema)"), QStringLiteral("Тестування REST API (Postman, валідація відповідей)"),
            QStringLiteral("API Testing"), QStringLiteral("Тестування API"),
            models::TopicPriority::High,
            {QStringLiteral("postman"), QStringLiteral("постман"), QStringLiteral("api testing"), QStringLiteral("тестування апі"), QStringLiteral("rest"), QStringLiteral("json schema"), QStringLiteral("swagger")},
            {{QStringLiteral("Postman Learning Center"), QStringLiteral("https://learning.postman.com/docs/introduction/overview/")}},
            {{QStringLiteral("Центр навчання Postman"), QStringLiteral("https://learning.postman.com/docs/introduction/overview/")}},
            false, false
        });
        list.append({
            QStringLiteral("qa-automation"),
            QStringLiteral("UI Automation Frameworks (Playwright, Selenium)"), QStringLiteral("Автоматизація тестування UI (Playwright, Selenium)"),
            QStringLiteral("Automation"), QStringLiteral("Автоматизація"),
            models::TopicPriority::Medium,
            {QStringLiteral("playwright"), QStringLiteral("плейрайт"), QStringLiteral("selenium"), QStringLiteral("селеніум"), QStringLiteral("cypress"), QStringLiteral("сайпрес"), QStringLiteral("автоматизація"), QStringLiteral("page object")},
            {{QStringLiteral("Playwright Getting Started"), QStringLiteral("https://playwright.dev/docs/intro")}},
            {{QStringLiteral("Документація з автоматизації Playwright"), QStringLiteral("https://playwright.dev/docs/intro")}},
            true, false
        });
        list.append({
            QStringLiteral("qa-sql"),
            QStringLiteral("Database Validation & SQL Queries for QA"), QStringLiteral("Перевірка баз даних та SQL-запити для тестувальників"),
            QStringLiteral("Databases"), QStringLiteral("Бази даних"),
            models::TopicPriority::Medium,
            {QStringLiteral("sql"), QStringLiteral("скл"), QStringLiteral("select"), QStringLiteral("join"), QStringLiteral("база даних"), QStringLiteral("перевірка даних")},
            {{QStringLiteral("W3Schools SQL Tutorial"), QStringLiteral("https://www.w3schools.com/sql/")}},
            {{QStringLiteral("Посібник з SQL для тестування"), QStringLiteral("https://www.w3schools.com/sql/")}},
            false, false
        });
        list.append({
            QStringLiteral("qa-cicd"),
            QStringLiteral("CI/CD Integration for Test Automation"), QStringLiteral("Інтеграція автотестів у CI/CD пайплайни"),
            QStringLiteral("DevOps"), QStringLiteral("Інфраструктура"),
            models::TopicPriority::Low,
            {QStringLiteral("ci/cd"), QStringLiteral("сіай"), QStringLiteral("github actions"), QStringLiteral("allure"), QStringLiteral("репорти"), QStringLiteral("запуск тестів")},
            {{QStringLiteral("Continuous Testing in CI/CD"), QStringLiteral("https://docs.github.com/actions/automating-builds-and-tests")}},
            {{QStringLiteral("Автоматизація тестів у GitHub Actions"), QStringLiteral("https://docs.github.com/actions/automating-builds-and-tests")}},
            true, false
        });
        list.append({
            QStringLiteral("qa-jira"),
            QStringLiteral("Agile QA, Jira & Bug Lifecycle Management"), QStringLiteral("Робота в Agile/Scrum та життєвий цикл дефекту в Jira"),
            QStringLiteral("Process"), QStringLiteral("Процеси"),
            models::TopicPriority::Medium,
            {QStringLiteral("jira"), QStringLiteral("джіра"), QStringLiteral("жира"), QStringLiteral("scrum"), QStringLiteral("скрам"), QStringLiteral("bug lifecycle"), QStringLiteral("життєвий цикл дефекту")},
            {{QStringLiteral("Atlassian Agile Coach"), QStringLiteral("https://www.atlassian.com/agile")}},
            {{QStringLiteral("Посібник з гнучких методологій Atlassian"), QStringLiteral("https://www.atlassian.com/agile")}},
            false, false
        });
        list.append({
            QStringLiteral("qa-performance"),
            QStringLiteral("Performance & Load Testing Basics (k6, JMeter)"), QStringLiteral("Основи навантажувального тестування (k6, JMeter)"),
            QStringLiteral("Performance"), QStringLiteral("Продуктивність"),
            models::TopicPriority::Low,
            {QStringLiteral("jmeter"), QStringLiteral("джейметер"), QStringLiteral("k6"), QStringLiteral("навантажувальне"), QStringLiteral("load testing"), QStringLiteral("stress testing")},
            {{QStringLiteral("k6 Documentation"), QStringLiteral("https://grafana.com/docs/k6/latest/")}},
            {{QStringLiteral("Документація інструменту k6"), QStringLiteral("https://grafana.com/docs/k6/latest/")}},
            true, false
        });
    } else { // Project Management (pm)
        list.append({
            QStringLiteral("pm-scrum"),
            QStringLiteral("Agile Frameworks & Scrum Ceremonies"), QStringLiteral("Методології Agile: Scrum, Kanban та церемонії"),
            QStringLiteral("Frameworks"), QStringLiteral("Методології"),
            models::TopicPriority::High,
            {QStringLiteral("scrum"), QStringLiteral("скрам"), QStringLiteral("agile"), QStringLiteral("еджайл"), QStringLiteral("kanban"), QStringLiteral("канбан"), QStringLiteral("daily"), QStringLiteral("дейлі"), QStringLiteral("ретроспектива")},
            {{QStringLiteral("Official Scrum Guide"), QStringLiteral("https://scrumguides.org/scrum-guide.html")}},
            {{QStringLiteral("Офіційний посібник зі Scrum"), QStringLiteral("https://scrumguides.org/scrum-guide.html")}},
            false, true
        });
        list.append({
            QStringLiteral("pm-requirements"),
            QStringLiteral("Requirements Breakdown & User Stories Definition"), QStringLiteral("Аналіз вимог, створення User Stories та критеріїв прийомки"),
            QStringLiteral("Requirements"), QStringLiteral("Вимоги"),
            models::TopicPriority::High,
            {QStringLiteral("user stories"), QStringLiteral("юзер сторі"), QStringLiteral("acceptance criteria"), QStringLiteral("критерії прийомки"), QStringLiteral("вимоги"), QStringLiteral("requirements")},
            {{QStringLiteral("User Stories Guide by Agile Alliance"), QStringLiteral("https://www.agilealliance.org/glossary/user-stories/")}},
            {{QStringLiteral("Посібник зі складання User Stories"), QStringLiteral("https://www.agilealliance.org/glossary/user-stories/")}},
            false, true
        });
        list.append({
            QStringLiteral("pm-planning"),
            QStringLiteral("Sprint Planning, Estimation & Backlog Refinement"), QStringLiteral("Планування спринту, оцінка задач та пріоритизація беклогу"),
            QStringLiteral("Planning"), QStringLiteral("Планування"),
            models::TopicPriority::High,
            {QStringLiteral("backlog"), QStringLiteral("беклог"), QStringLiteral("estimation"), QStringLiteral("оцінка"), QStringLiteral("story points"), QStringLiteral("стори поінти"), QStringLiteral("planning"), QStringLiteral("планування")},
            {{QStringLiteral("Agile Estimation Overview"), QStringLiteral("https://www.atlassian.com/agile/project-management/estimation")}},
            {{QStringLiteral("Оцінювання завдань у Scrum"), QStringLiteral("https://www.atlassian.com/agile/project-management/estimation")}},
            false, false
        });
        list.append({
            QStringLiteral("pm-risk"),
            QStringLiteral("Project Risk Management & Mitigation Strategies"), QStringLiteral("Управління проектними ризиками та шляхи їх зменшення"),
            QStringLiteral("Risk Management"), QStringLiteral("Управління ризиками"),
            models::TopicPriority::Medium,
            {QStringLiteral("risk"), QStringLiteral("ризик"), QStringLiteral("ризики"), QStringLiteral("mitigation"), QStringLiteral("план б"), QStringLiteral("contingency")},
            {{QStringLiteral("PMI Risk Management Insights"), QStringLiteral("https://www.pmi.org/learning/library/risk-management-agile-projects-6126")}},
            {{QStringLiteral("Управління ризиками в проектах"), QStringLiteral("https://www.pmi.org/learning/library/risk-management-agile-projects-6126")}},
            true, false
        });
        list.append({
            QStringLiteral("pm-stakeholders"),
            QStringLiteral("Stakeholder Communication & Status Reporting"), QStringLiteral("Комунікація зі стейкхолдерами та регулярна звітність"),
            QStringLiteral("Communication"), QStringLiteral("Комунікація"),
            models::TopicPriority::High,
            {QStringLiteral("stakeholder"), QStringLiteral("стейкхолдер"), QStringLiteral("клієнт"), QStringLiteral("комунікація"), QStringLiteral("reporting"), QStringLiteral("звітність"), QStringLiteral("презентація")},
            {{QStringLiteral("Stakeholder Engagement Strategies"), QStringLiteral("https://www.pmi.org/learning/library/stakeholder-management-task-relationship-oriented-6745")}},
            {{QStringLiteral("Стратегії залучення стейкхолдерів"), QStringLiteral("https://www.pmi.org/learning/library/stakeholder-management-task-relationship-oriented-6745")}},
            false, false
        });
        list.append({
            QStringLiteral("pm-metrics"),
            QStringLiteral("Agile Delivery Metrics (Velocity, Burndown, Lead Time)"), QStringLiteral("Метрики ефективності команди (Velocity, Burndown, Lead Time)"),
            QStringLiteral("Metrics"), QStringLiteral("Метрики"),
            models::TopicPriority::Medium,
            {QStringLiteral("velocity"), QStringLiteral("велосіті"), QStringLiteral("burndown"), QStringLiteral("берндаун"), QStringLiteral("метрики"), QStringLiteral("lead time"), QStringLiteral("cycle time")},
            {{QStringLiteral("Burndown Charts and Velocity"), QStringLiteral("https://www.atlassian.com/agile/tutorials/burndown-charts")}},
            {{QStringLiteral("Графіки згоряння та швидкість команди"), QStringLiteral("https://www.atlassian.com/agile/tutorials/burndown-charts")}},
            true, false
        });
        list.append({
            QStringLiteral("pm-tools"),
            QStringLiteral("Work Management Tools (Jira, Confluence Setup)"), QStringLiteral("Інструменти управління проектами (Jira, Confluence)"),
            QStringLiteral("Tooling"), QStringLiteral("Інструменти"),
            models::TopicPriority::Medium,
            {QStringLiteral("jira"), QStringLiteral("джіра"), QStringLiteral("жира"), QStringLiteral("confluence"), QStringLiteral("конфлюенс"), QStringLiteral("trello"), QStringLiteral("дошка")},
            {{QStringLiteral("Jira Software Guides"), QStringLiteral("https://www.atlassian.com/software/jira/guides")}},
            {{QStringLiteral("Посібники з використання Jira"), QStringLiteral("https://www.atlassian.com/software/jira/guides")}},
            false, false
        });
        list.append({
            QStringLiteral("pm-conflict"),
            QStringLiteral("Team Dynamics, Conflict Resolution & Motivation"), QStringLiteral("Динаміка команди, вирішення конфліктів та фасилітація"),
            QStringLiteral("Leadership"), QStringLiteral("Лідерство"),
            models::TopicPriority::Low,
            {QStringLiteral("конфлікт"), QStringLiteral("мотивація"), QStringLiteral("фасилітація"), QStringLiteral("1-on-1"), QStringLiteral("one on one"), QStringLiteral("команда"), QStringLiteral("leadership")},
            {{QStringLiteral("Team Dynamics and Tuckman Model"), QStringLiteral("https://www.mindtools.com/pages/article/newLDR_86.htm")}},
            {{QStringLiteral("Розвиток та динаміка команди"), QStringLiteral("https://www.mindtools.com/pages/article/newLDR_86.htm")}},
            false, false
        });
    }

    return list;
}

models::PreparationPlanResult DemoPreparationService::evaluateSynchronously(
    const QString& direction,
    const QString& level,
    const QString& userDescription,
    const QString& language
) const {
    const bool isUk = language.trimmed().toLower() != QStringLiteral("en");
    const QString descLower = userDescription.toLower();

    QVector<TopicDefinition> definitions = getDefinitionsForDirection(direction);
    models::PreparationPlanResult plan;
    plan.totalTopics = static_cast<int>(definitions.size());
    plan.coveredTopics = 0;

    for (const TopicDefinition& def : definitions) {
        models::PreparationTopic topic;
        topic.id = def.id;
        topic.title = isUk ? def.titleUk : def.titleEn;
        topic.category = isUk ? def.categoryUk : def.categoryEn;
        topic.materials = isUk ? def.materialsUk : def.materialsEn;
        topic.priority = adjustPriority(def.basePriority, def.isFundamental, def.isAdvanced, level);

        QString reason;
        topic.status = evaluateStatus(descLower, def.keywords, reason, isUk);
        topic.reason = reason;

        if (topic.status == models::PreparationTopicStatus::Known) {
            plan.coveredTopics++;
        }

        plan.topics.append(topic);
    }

    // Build structured overall summary
    if (plan.coveredTopics == plan.totalTopics) {
        plan.summary = isUk ? QStringLiteral("Чудово! У вашому описі впевнено згадано всі ключові теми для цього напряму.")
                            : QStringLiteral("Excellent! Your description confidently covers all fundamental topics for this track.");
    } else if (plan.coveredTopics > 0) {
        plan.summary = isUk ? QStringLiteral("Проаналізовано опис: покрито %1 з %2 тем. Нижче наведено матеріали для повторення незгаданих розділів.")
                                .arg(plan.coveredTopics).arg(plan.totalTopics)
                            : QStringLiteral("Analysis complete: %1 of %2 topics covered. Below are curated recommendations for the unmentioned areas.")
                                .arg(plan.coveredTopics).arg(plan.totalTopics);
    } else {
        plan.summary = isUk ? QStringLiteral("У вашому описі поки що не виявлено згадок ключових тем обраного напряму. Рекомендуємо почати підготовку за наведеним планом.")
                            : QStringLiteral("No core topic mentions were detected in your description. We recommend following the preparation roadmap below.");
    }

    return plan;
}

} // namespace devdialogue::services
