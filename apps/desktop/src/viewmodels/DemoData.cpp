#include "viewmodels/DemoData.hpp"

#include <QDateTime>

namespace devdialogue::viewmodels::demo {

namespace {

bool isUkrainian(const QString& language) {
    return language.compare(QStringLiteral("uk"), Qt::CaseInsensitive) == 0;
}

std::string isoSecondsAgo(qint64 seconds) {
    return QDateTime::currentDateTimeUtc().addSecs(-seconds).toString(Qt::ISODateWithMs).toStdString();
}

models::InterviewSummary summary(const char* id, models::InterviewDirection direction, int score,
                                 int questionCount, qint64 secondsAgo) {
    models::InterviewSummary item;
    item.id = id;
    item.userId = "u1";
    item.direction = direction;
    item.level = models::InterviewLevel::Junior;
    item.language = models::InterviewLanguage::Uk;
    item.stage = models::InterviewStage::HardSkills;
    item.status = models::InterviewStatus::Completed;
    item.score = score;
    item.questionCount = questionCount;
    item.createdAt = isoSecondsAgo(secondsAgo);
    item.updatedAt = isoSecondsAgo(0);
    return item;
}

models::QuestionBankItem question(const char* id, models::InterviewDirection direction,
                                  const char* topic, const char* text) {
    models::QuestionBankItem item;
    item.id = id;
    item.direction = direction;
    item.level = models::InterviewLevel::Junior;
    item.stage = models::InterviewStage::HardSkills;
    item.topic = topic;
    item.questionText = text;
    item.isActive = true;
    item.createdAt = isoSecondsAgo(0);
    return item;
}

} // namespace

models::UserDashboardStats dashboardStats() {
    models::UserDashboardStats stats;
    stats.totalInterviews = 4;
    stats.averageScore = 82.5;
    stats.dailyInterviewsCount = 2;
    stats.dailyLimit = 10;
    stats.remainingToday = 8;
    return stats;
}

std::vector<models::InterviewSummary> interviews() {
    return {
        summary("mock-1", models::InterviewDirection::Frontend, 88, 5, 2 * 3600),
        summary("mock-2", models::InterviewDirection::Backend, 77, 6, 24 * 3600),
    };
}

models::InterviewFeedbackReport report(const QString& language) {
    models::InterviewFeedbackReport result;
    result.score = 84;
    result.recommendations = {
        {"Database Indexing & B-Trees", "https://use-the-index-luke.com"},
        {"Distributed Systems & Concurrency", "https://github.com/donnemartin/system-design-primer"},
        {"Modern Web Architecture & Patterns", "https://martinfowler.com/architecture"},
    };

    if (isUkrainian(language)) {
        result.summary =
            "Кандидат продемонстрував ґрунтовні знання ключових веб-стандартів, асинхронності та "
            "структури компонентів. Відповіді структуровані, логічні та підкріплені аргументацією.";
        result.strengths = {
            "Впевнене володіння базовою термінологією та архітектурними паттернами",
            "Чітке розуміння протоколів взаємодії, REST та життєвого циклу запитів",
            "Здатність аргументувати вибір технологій та підходів під навантаженням",
        };
        result.weaknesses = {
            "Потребує глибшого вивчення внутрішніх механізмів оптимізації та індексації баз даних",
            "Варто приділити більше уваги крайовим випадкам при розподіленій обробці помилок",
        };
        result.questionScores = {
            {"Різниця між TCP та UDP, сценарії використання", 90,
             "Відмінне розуміння протоколів транспортного рівня та надійності передачі."},
            {"Оптимізація та індекси в реляційних БД", 78,
             "Добра відповідь, проте не вистачило глибини щодо B-Tree та вартості оновлення індексів."},
        };
    } else {
        result.summary =
            "The candidate demonstrated solid command of modern web standards, asynchronous programming, "
            "and component lifecycle principles. Answers were well-structured, logical, and substantiated "
            "with technical tradeoffs.";
        result.strengths = {
            "Confident grasp of architectural patterns, state managers, and React fiber",
            "Clear understanding of network communication protocols, RESTful APIs, and request lifecycles",
            "Strong ability to articulate architectural decisions and performance tradeoffs under pressure",
        };
        result.weaknesses = {
            "Deeper study into database indexing mechanisms, B-Tree lookups, and query plans is advised",
            "Could pay closer attention to edge cases in distributed fault handling and circuit breakers",
        };
        result.questionScores = {
            {"Difference between TCP and UDP, common real-world use cases", 90,
             "Great understanding of transport layer reliability, handshakes, and packet streaming."},
            {"Relational DB query optimization and composite indexes", 78,
             "Solid overall explanation, but missed subtleties regarding index write penalty and lock contention."},
        };
    }
    return result;
}

std::vector<models::QuestionBankItem> questions(const QString& language) {
    if (isUkrainian(language)) {
        return {
            question("q-1", models::InterviewDirection::Frontend, "React Hooks & State",
                     "Поясніть різницю між controlled та uncontrolled компонентами у формах."),
            question("q-2", models::InterviewDirection::Backend, "Networking & Protocols",
                     "Поясніть різницю між TCP та UDP. У яких сценаріях UDP є кращим вибором?"),
        };
    }
    return {
        question("q-1", models::InterviewDirection::Frontend, "React Hooks & State",
                 "Explain the difference between controlled and uncontrolled components in React forms."),
        question("q-2", models::InterviewDirection::Backend, "Networking & Protocols",
                 "Explain the difference between TCP and UDP. When is UDP the preferred protocol?"),
    };
}

QString chatWelcome(const QString& direction, const QString& level, const QString& language) {
    if (isUkrainian(language)) {
        return QString::fromUtf8(
                   "Вітаю на технічній співбесіді за напрямком %1 (%2)! Я ваш AI-інтерв'юер. Давайте "
                   "розпочнемо. Розкажіть коротко про себе та ваш практичний досвід у цій сфері.")
            .arg(direction, level);
    }
    return QStringLiteral(
               "Welcome to your %1 (%2) technical interview! I am your AI interviewer. Let's begin. "
               "Could you please briefly introduce yourself and share your relevant background?")
        .arg(direction, level);
}

QStringList chatReplies(const QString& language) {
    if (isUkrainian(language)) {
        return {
            QString::fromUtf8("Дякую за відповідь! Скажіть, будь ласка, як саме ви оптимізуєте продуктивність та "
                              "уникаєте зайвих ререндерів у ваших компонентах?"),
            QString::fromUtf8("Чудово. А які патерни обробки помилок та асинхронних запитів ви зазвичай "
                              "використовуєте на практиці?"),
            QString::fromUtf8("Цікавий підхід. Чи доводилося вам налаштовувати CI/CD або працювати з Docker "
                              "контейнерами для розгортання?"),
            QString::fromUtf8("Дякую за ґрунтовні відповіді! Усі питання цієї сесії вичерпано. Тепер ви можете "
                              "натиснути кнопку завершення, щоб отримати детальний скоркарт."),
        };
    }
    return {
        QStringLiteral("Thank you for your answer! Could you elaborate on how you optimize performance and "
                       "prevent unnecessary re-renders in your code?"),
        QStringLiteral("Great point. What patterns do you typically apply for handling async states and API "
                       "error boundaries in production?"),
        QStringLiteral("Interesting approach. Have you had hands-on experience configuring Docker or CI/CD "
                       "pipelines?"),
        QStringLiteral("Thank you for articulating your reasoning. All questions for this session are complete! "
                       "You can now finish the interview to view your full scorecard."),
    };
}

} // namespace devdialogue::viewmodels::demo
