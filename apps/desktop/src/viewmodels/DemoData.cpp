#include "viewmodels/DemoData.hpp"

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels::demo {

namespace {

using models::InterviewDirection;

bool isUkrainian(const QString& language) {
    return language.compare(QStringLiteral("uk"), Qt::CaseInsensitive) == 0;
}

std::string isoSecondsAgo(qint64 seconds) {
    return QDateTime::currentDateTimeUtc().addSecs(-seconds).toString(Qt::ISODateWithMs).toStdString();
}

std::string utf8(const QString& text) {
    return text.toStdString();
}

// ---------------------------------------------------------------- History

std::vector<models::InterviewDetail>& recorded() {
    static std::vector<models::InterviewDetail> items;
    return items;
}

models::InterviewSummary sample(const char* id, InterviewDirection direction, int score, int questionCount,
                                qint64 secondsAgo) {
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
    item.updatedAt = isoSecondsAgo(secondsAgo - 1200);
    return item;
}

std::vector<models::InterviewSummary> samples() {
    return {
        sample("mock-1", InterviewDirection::Frontend, 88, 2, 2 * 3600),
        sample("mock-2", InterviewDirection::Backend, 77, 2, 26 * 3600),
    };
}

// --------------------------------------------------------- Report helpers

std::vector<models::RecommendationItem> recommendations(const QString& direction, bool uk) {
    if (direction == QLatin1String("backend")) {
        return {
            {uk ? "Use The Index, Luke: індекси в SQL" : "Use The Index, Luke: SQL indexing", "https://use-the-index-luke.com"},
            {"System Design Primer", "https://github.com/donnemartin/system-design-primer"},
        };
    }
    if (direction == QLatin1String("devops")) {
        return {
            {uk ? "Docker: перші кроки" : "Docker: Get started", "https://docs.docker.com/get-started/"},
            {uk ? "Основи Kubernetes" : "Kubernetes basics", "https://kubernetes.io/docs/tutorials/kubernetes-basics/"},
        };
    }
    if (direction == QLatin1String("qa")) {
        return {
            {uk ? "Практична піраміда тестів" : "The Practical Test Pyramid",
             "https://martinfowler.com/articles/practical-test-pyramid.html"},
            {uk ? "Playwright: початок роботи" : "Playwright: Getting started", "https://playwright.dev/docs/intro"},
        };
    }
    if (direction == QLatin1String("pm")) {
        return {
            {uk ? "Посібник зі Scrum" : "The Scrum Guide", "https://scrumguides.org/scrum-guide.html"},
            {"Atlassian Agile Coach", "https://www.atlassian.com/agile"},
        };
    }
    return {
        {uk ? "Документація React" : "React documentation", "https://react.dev/learn"},
        {uk ? "Посібник MDN з JavaScript" : "MDN JavaScript guide", "https://developer.mozilla.org/docs/Web/JavaScript"},
    };
}

models::InterviewFeedbackReport frontendSample(bool uk) {
    models::InterviewFeedbackReport result;
    result.score = 88;
    result.recommendations = recommendations(QStringLiteral("frontend"), uk);
    if (uk) {
        result.summary = "Ви впевнено пояснили, як React керує станом форм і коли компонент перерендерюється. "
                         "Відповіді структуровані, з прикладами з практики.";
        result.strengths = {
            "Чітко розрізняєте контрольовані та неконтрольовані компоненти",
            "Знаєте, коли memo та useCallback справді допомагають",
        };
        result.weaknesses = {"Варто глибше розібратися, як React групує оновлення стану"};
        result.questionScores = {
            {"Контрольовані та неконтрольовані компоненти у формах", 90,
             "Точне пояснення з прикладом валідації форми."},
            {"Як уникнути зайвих ререндерів", 86, "Добре про memo; бракувало вимірювання через React Profiler."},
        };
    } else {
        result.summary = "You explained clearly how React manages form state and when a component re-renders. "
                         "Your answers were structured and backed by examples from your work.";
        result.strengths = {
            "You tell controlled and uncontrolled components apart precisely",
            "You know when memo and useCallback actually help",
        };
        result.weaknesses = {"Look deeper into how React batches state updates"};
        result.questionScores = {
            {"Controlled vs uncontrolled components in forms", 90, "Precise explanation with a form validation example."},
            {"Avoiding unnecessary re-renders", 86, "Good on memo; measuring with the React Profiler was missing."},
        };
    }
    return result;
}

models::InterviewFeedbackReport backendSample(bool uk) {
    models::InterviewFeedbackReport result;
    result.score = 77;
    result.recommendations = recommendations(QStringLiteral("backend"), uk);
    if (uk) {
        result.summary = "Ви добре знаєте транспортний рівень мережі, але про індекси в базах даних відповіли "
                         "поверхово.";
        result.strengths = {"Розумієте різницю між TCP та UDP і де кожен доречний"};
        result.weaknesses = {
            "Вивчіть, як працюють B-tree індекси та чим вони обходяться під час запису",
            "Навчіться читати плани виконання запитів",
        };
        result.questionScores = {
            {"Різниця між TCP та UDP, сценарії використання", 80,
             "Правильна відповідь; бракувало прикладу з реального проєкту."},
            {"Індекси в реляційних БД", 74, "Загальне пояснення без вартості оновлення індексів."},
        };
    } else {
        result.summary = "You know the transport layer well, but your answer on database indexes stayed on the "
                         "surface.";
        result.strengths = {"You understand the difference between TCP and UDP and where each fits"};
        result.weaknesses = {
            "Study how B-tree indexes work and what they cost on writes",
            "Learn to read query execution plans",
        };
        result.questionScores = {
            {"Difference between TCP and UDP, use cases", 80, "Correct answer; a real project example was missing."},
            {"Indexes in relational databases", 74, "General explanation without the cost of index updates."},
        };
    }
    return result;
}

// The question itself is the last sentence of the interviewer's message
// ("Thanks. How do you ...?" -> "How do you ...?").
QString lastSentence(const QString& message) {
    static const QRegularExpression boundary(QStringLiteral("(?<=[.!?])\\s+"));
    const QStringList sentences = message.trimmed().split(boundary, Qt::SkipEmptyParts);
    return sentences.isEmpty() ? message.trimmed() : sentences.last();
}

QString shortQuote(const QString& text) {
    constexpr qsizetype Limit = 70;
    return text.size() <= Limit ? text : text.left(Limit - 1).trimmed() + QStringLiteral("…");
}

int wordCount(const QString& text) {
    static const QRegularExpression spaces(QStringLiteral("\\s+"));
    return static_cast<int>(text.split(spaces, Qt::SkipEmptyParts).size());
}

constexpr int ShortAnswerWords = 12;
constexpr int DetailedAnswerWords = 40;

int scoreForAnswer(int words) {
    return std::clamp(static_cast<int>(std::lround(30 + words * 1.6)), 30, 92);
}

QString commentForAnswer(int words, bool uk) {
    if (words < ShortAnswerWords) {
        return uk ? QString::fromUtf8("Коротка відповідь. Додайте приклад із практики та назвіть компроміси.")
                  : QStringLiteral("Short answer. Add an example from your work and name the trade-offs.");
    }
    if (words < DetailedAnswerWords) {
        return uk ? QString::fromUtf8("Основу пояснено. Розкажіть, чому саме такий підхід і чого він коштує.")
                  : QStringLiteral("The basics are there. Explain why this approach and what it costs.");
    }
    return uk ? QString::fromUtf8("Розгорнута відповідь з деталями.")
              : QStringLiteral("A detailed answer with specifics.");
}

} // namespace

// ------------------------------------------------------------------ History

void recordFinishedInterview(const models::InterviewDetail& interview) {
    auto& items = recorded();
    const auto existing = std::find_if(items.begin(), items.end(),
                                       [&](const models::InterviewDetail& item) { return item.id == interview.id; });
    if (existing != items.end()) {
        *existing = interview;
    } else {
        items.push_back(interview);
    }
}

const models::InterviewDetail* finishedInterview(const QString& interviewId) {
    const std::string id = utf8(interviewId);
    for (const models::InterviewDetail& item : recorded()) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

void clearFinishedInterviews() {
    recorded().clear();
}

std::vector<models::InterviewSummary> interviews() {
    std::vector<models::InterviewSummary> result = samples();
    for (const models::InterviewDetail& item : recorded()) {
        models::InterviewSummary summary = item;
        const models::InterviewFeedbackReport scored = reportFromTranscript(
            support::enumToString(item.direction), item.transcript, support::enumToString(item.language));
        summary.status = models::InterviewStatus::Completed;
        summary.score = scored.score;
        summary.questionCount = static_cast<int>(scored.questionScores.size());
        result.push_back(summary);
    }
    std::sort(result.begin(), result.end(), [](const models::InterviewSummary& a, const models::InterviewSummary& b) {
        return support::parseTimestamp(a.createdAt) > support::parseTimestamp(b.createdAt);
    });
    return result;
}

models::UserDashboardStats dashboardStats() {
    models::UserDashboardStats stats;
    const QDate today = QDate::currentDate();
    int scored = 0;
    double total = 0.0;
    for (const models::InterviewSummary& item : interviews()) {
        if (item.status == models::InterviewStatus::Completed) {
            ++stats.totalInterviews;
        }
        if (item.score.has_value()) {
            ++scored;
            total += *item.score;
        }
        if (support::parseTimestamp(item.createdAt).toLocalTime().date() == today) {
            ++stats.dailyInterviewsCount;
        }
    }
    stats.averageScore = scored > 0 ? std::round(total / scored * 10.0) / 10.0 : 0.0;
    stats.dailyLimit = 10;
    stats.remainingToday = std::max(0, stats.dailyLimit - stats.dailyInterviewsCount);
    return stats;
}

// ------------------------------------------------------------------ Reports

models::InterviewFeedbackReport report(const QString& interviewId, const QString& uiLanguage) {
    const bool uk = isUkrainian(uiLanguage);
    if (interviewId == QLatin1String("mock-1")) {
        return frontendSample(uk);
    }
    if (interviewId == QLatin1String("mock-2")) {
        return backendSample(uk);
    }
    if (const models::InterviewDetail* interview = finishedInterview(interviewId)) {
        return reportFromTranscript(support::enumToString(interview->direction), interview->transcript,
                                    support::enumToString(interview->language));
    }
    return reportFromTranscript(QString(), {}, uiLanguage);
}

models::InterviewFeedbackReport reportFromTranscript(const QString& direction,
                                                     const std::vector<models::InterviewMessage>& transcript,
                                                     const QString& language) {
    const bool uk = isUkrainian(language);
    models::InterviewFeedbackReport result;
    if (!direction.isEmpty()) {
        result.recommendations = recommendations(direction, uk);
    }

    QString question;
    int total = 0;
    for (const models::InterviewMessage& message : transcript) {
        const QString content = QString::fromStdString(message.content);
        if (message.role == "assistant") {
            question = lastSentence(content);
            continue;
        }
        if (message.role != "user" || question.isEmpty()) {
            continue;
        }
        const int words = wordCount(content);
        const int score = scoreForAnswer(words);
        total += score;
        result.questionScores.push_back({utf8(question), score, utf8(commentForAnswer(words, uk))});

        const QString quote = QStringLiteral("“%1”").arg(shortQuote(question));
        if (words >= DetailedAnswerWords && result.strengths.size() < 3) {
            result.strengths.push_back(
                utf8((uk ? QString::fromUtf8("Розгорнута відповідь на %1") : QStringLiteral("Detailed answer to %1"))
                         .arg(quote)));
        } else if (words < ShortAnswerWords && result.weaknesses.size() < 3) {
            result.weaknesses.push_back(
                utf8((uk ? QString::fromUtf8("Коротка відповідь на %1") : QStringLiteral("Short answer to %1"))
                         .arg(quote)));
        }
        question.clear();
    }

    const int answered = static_cast<int>(result.questionScores.size());
    if (answered == 0) {
        result.score = 0;
        result.summary = utf8(uk ? QString::fromUtf8("У цьому інтерв'ю немає відповідей, тож оцінювати нічого.")
                                 : QStringLiteral("This interview has no answers, so there is nothing to score."));
        return result;
    }

    result.score = static_cast<int>(std::lround(static_cast<double>(total) / answered));
    result.summary = utf8(
        (uk ? QString::fromUtf8("Офлайн-демо: без сервера немає AI-оцінювача, тому кожну відповідь оцінено за тим, "
                                "наскільки вона розгорнута, а не за змістом. Відповідей: %1.")
            : QStringLiteral("Offline demo: without the server there is no AI evaluator, so each answer is scored "
                             "by how detailed it is, not by its content. Answers: %1."))
            .arg(answered));
    return result;
}

// ---------------------------------------------- Interview room, question bank

std::vector<models::QuestionBankItem> questions(const QString& language) {
    auto question = [](const char* id, InterviewDirection direction, const char* topic, const char* text) {
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
    };
    if (isUkrainian(language)) {
        return {
            question("q-1", InterviewDirection::Frontend, "React Hooks & State",
                     "Поясніть різницю між controlled та uncontrolled компонентами у формах."),
            question("q-2", InterviewDirection::Backend, "Networking & Protocols",
                     "Поясніть різницю між TCP та UDP. У яких сценаріях UDP є кращим вибором?"),
        };
    }
    return {
        question("q-1", InterviewDirection::Frontend, "React Hooks & State",
                 "Explain the difference between controlled and uncontrolled components in React forms."),
        question("q-2", InterviewDirection::Backend, "Networking & Protocols",
                 "Explain the difference between TCP and UDP. When is UDP the preferred protocol?"),
    };
}

QString chatWelcome(const QString& direction, const QString& level, const QString& language) {
    const QString track = direction == QLatin1String("qa") || direction == QLatin1String("pm")
                              ? direction.toUpper()
                          : direction == QLatin1String("devops") ? QStringLiteral("DevOps")
                                                                 : direction.left(1).toUpper() + direction.mid(1);
    const QString grade = level.left(1).toUpper() + level.mid(1);
    if (isUkrainian(language)) {
        return QString::fromUtf8("Вітаю на технічній співбесіді: %1, рівень %2. Я ваш AI-інтерв'юер. "
                                 "Розкажіть коротко про себе та ваш практичний досвід у цій сфері.")
            .arg(track, grade);
    }
    return QStringLiteral("Welcome to your %1 interview, %2 level. I am your AI interviewer. "
                          "Please introduce yourself briefly and tell me about your hands-on experience.")
        .arg(track, grade);
}

QStringList chatReplies(const QString& direction, const QString& language) {
    const bool uk = isUkrainian(language);
    QStringList replies;
    if (direction == QLatin1String("backend")) {
        replies = uk ? QStringList{
            QString::fromUtf8("Дякую. Як би ви спроєктували REST-ендпоінт, що повертає велику колекцію, і чому саме так?"),
            QString::fromUtf8("Добре. Коли ви додаєте індекс до таблиці і чим він обходиться під час запису?"),
            QString::fromUtf8("Як ви обробляєте збої залежного сервісу, щоб ваш API не блокувався?"),
        } : QStringList{
            QStringLiteral("Thanks. How would you design a REST endpoint that returns a large collection, and why?"),
            QStringLiteral("Good. When would you add an index to a table, and what does it cost on writes?"),
            QStringLiteral("How do you handle a failing downstream service without blocking your own API?"),
        };
    } else if (direction == QLatin1String("devops")) {
        replies = uk ? QStringList{
            QString::fromUtf8("Дякую. Що має бути в доброму Dockerfile для продакшн-образу?"),
            QString::fromUtf8("Добре. Як налаштувати CI-пайплайн, щоб зламана збірка не потрапила в продакшн?"),
            QString::fromUtf8("Як ви дізнаєтеся, що сервіс нездоровий, ще до скарг користувачів?"),
        } : QStringList{
            QStringLiteral("Thanks. What goes into a good Dockerfile for a production image?"),
            QStringLiteral("Good. How would you set up a CI pipeline that keeps a broken build out of production?"),
            QStringLiteral("How do you find out that a service is unhealthy before users report it?"),
        };
    } else if (direction == QLatin1String("qa")) {
        replies = uk ? QStringList{
            QString::fromUtf8("Дякую. Як ви вирішуєте, які тест-кейси автоматизувати першими?"),
            QString::fromUtf8("Добре. Як би ви протестували API-ендпоінт, що створює замовлення?"),
            QString::fromUtf8("Баг не відтворюється на вашій машині. Що ви робите далі?"),
        } : QStringList{
            QStringLiteral("Thanks. How do you decide which test cases to automate first?"),
            QStringLiteral("Good. How would you test an API endpoint that creates an order?"),
            QStringLiteral("A bug does not reproduce on your machine. What do you do next?"),
        };
    } else if (direction == QLatin1String("pm")) {
        replies = uk ? QStringList{
            QString::fromUtf8("Дякую. Як ви пріоритезуєте беклог, коли кожен стейкхолдер вважає свою задачу терміновою?"),
            QString::fromUtf8("Добре. Спринт ризикує не досягти мети. Що ви робите посеред спринту?"),
            QString::fromUtf8("Як ви відстежуєте ризики проєкту і повідомляєте про них команді?"),
        } : QStringList{
            QStringLiteral("Thanks. How do you prioritise a backlog when every stakeholder calls their item urgent?"),
            QStringLiteral("Good. A sprint is at risk of missing its goal. What do you do mid-sprint?"),
            QStringLiteral("How do you track project risks and communicate them to the team?"),
        };
    } else {
        replies = uk ? QStringList{
            QString::fromUtf8("Дякую. Як ви не даєте React-компоненту перерендерюватися, якщо його props не змінилися?"),
            QString::fromUtf8("Добре. Як ви показуєте в інтерфейсі стани завантаження та помилки API-запиту?"),
            QString::fromUtf8("А як зробити так, щоб довгий список елементів прокручувався плавно?"),
        } : QStringList{
            QStringLiteral("Thanks. How do you keep a React component from re-rendering when its props have not changed?"),
            QStringLiteral("Good. How do you show the loading and error states of an API request in the UI?"),
            QStringLiteral("And how would you make a long list of items scroll smoothly?"),
        };
    }
    replies.append(uk ? QString::fromUtf8("Дякую, це було останнє питання. Завершіть інтерв'ю, щоб отримати звіт.")
                      : QStringLiteral("Thank you, that was the last question. Finish the interview to get your report."));
    return replies;
}

} // namespace devdialogue::viewmodels::demo
