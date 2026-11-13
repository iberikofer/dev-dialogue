#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSettings>
#include <QStringList>
#include <QTemporaryDir>
#include <QThread>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <ostream>

#include <nlohmann/json.hpp>

#include "core/AppConfig.hpp"
#include "network/ApiClient.hpp"
#include "services/AdminService.hpp"
#include "services/AuthService.hpp"
#include "services/InMemorySecureStorage.hpp"
#include "services/InterviewService.hpp"
#include "tests/helpers/MockHttpServer.hpp"
#include "viewmodels/AdminViewModel.hpp"
#include "viewmodels/BackendStatusViewModel.hpp"
#include "viewmodels/ChatViewModel.hpp"
#include "viewmodels/DashboardViewModel.hpp"
#include "viewmodels/DemoData.hpp"
#include "viewmodels/InterviewListModel.hpp"
#include "viewmodels/MessageListModel.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/QuestionListModel.hpp"
#include "viewmodels/ReportViewModel.hpp"
#include "viewmodels/SettingsViewModel.hpp"
#include "viewmodels/SetupViewModel.hpp"

using namespace devdialogue;
using namespace devdialogue::viewmodels;

// Lets GoogleTest print QString values as text when an expectation fails.
inline void PrintTo(const QString& text, std::ostream* out) {
    *out << '"' << text.toStdString() << '"';
}

namespace {

using Request = test::MockHttpServer::HttpRequest;
using Response = test::MockHttpServer::HttpResponse;

Response jsonResponse(int status, const nlohmann::json& body) {
    return {status, status < 400 ? "OK" : "Error", QByteArray::fromStdString(body.dump()), "application/json", {}};
}

// Spins the event loop until the condition holds or the time runs out.
bool waitUntil(const std::function<bool()>& condition, int timeoutMs = 8000) {
    QElapsedTimer timer;
    timer.start();
    while (!condition()) {
        if (timer.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(2);
    }
    return true;
}

QString cell(QAbstractItemModel* model, int row, int role) {
    return model->data(model->index(row, 0), role).toString();
}

// Services pointed at a local mock HTTP server, as in the service tests.
class ViewModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(server.start());

        config = std::make_shared<core::AppConfig>();
        config->setApiBaseUrl(server.url().toStdString());
        config->setSupabaseUrl(server.url().toStdString());
        config->setSupabaseAnonKey("test-anon-key");

        storage = std::make_shared<services::InMemorySecureStorage>();
        client = std::make_shared<network::ApiClient>(config, storage);
        interviewService = std::make_shared<services::InterviewService>(client);
        adminService = std::make_shared<services::AdminService>(client);
        authService = std::make_shared<services::AuthService>(client, storage, config);
    }

    void TearDown() override { server.stop(); }

    // Installs a request handler and records every request line it sees.
    void serve(std::function<Response(const Request&)> handler) {
        server.setHandler([this, handler](const Request& request) {
            requests.append(request.method + " " + request.path);
            return handler(request);
        });
    }

    // Makes every request fail at the transport level.
    void takeBackendDown() { server.stop(); }

    test::MockHttpServer server;
    QStringList requests;
    std::shared_ptr<core::AppConfig> config;
    std::shared_ptr<services::InMemorySecureStorage> storage;
    std::shared_ptr<network::ApiClient> client;
    std::shared_ptr<services::InterviewService> interviewService;
    std::shared_ptr<services::AdminService> adminService;
    std::shared_ptr<services::AuthService> authService;
};

models::InterviewSummary completedInterview(const std::string& id, int score) {
    models::InterviewSummary item;
    item.id = id;
    item.userId = "u-1";
    item.direction = models::InterviewDirection::Qa;
    item.level = models::InterviewLevel::Middle;
    item.status = models::InterviewStatus::Completed;
    item.score = score;
    item.questionCount = 5;
    item.createdAt = "2026-11-04T12:00:00Z";
    item.updatedAt = "2026-11-04T12:30:00Z";
    return item;
}

models::QuestionBankItem bankQuestion(const std::string& id, models::InterviewDirection direction) {
    models::QuestionBankItem item;
    item.id = id;
    item.direction = direction;
    item.level = models::InterviewLevel::Junior;
    item.stage = models::InterviewStage::HardSkills;
    item.topic = "Topic " + id;
    item.questionText = "Text of " + id;
    item.isActive = true;
    item.createdAt = "2026-11-04T12:00:00Z";
    return item;
}

} // namespace

// ===========================================================================
// DashboardViewModel
// ===========================================================================

TEST_F(ViewModelTest, DashboardShowsStatsAndHistoryFromTheApi) {
    serve([](const Request& request) {
        if (request.path == "/api/interviews/stats") {
            models::UserDashboardStats stats;
            stats.totalInterviews = 12;
            stats.averageScore = 91.25;
            stats.dailyInterviewsCount = 3;
            stats.dailyLimit = 10;
            stats.remainingToday = 7;
            return jsonResponse(200, stats);
        }
        models::PagedResult<models::InterviewSummary> page;
        page.items = {completedInterview("i-1", 95), completedInterview("i-2", 87), completedInterview("i-3", 80)};
        page.totalCount = 3;
        return jsonResponse(200, page);
    });

    DashboardViewModel dashboard(interviewService);
    int loadingChanges = 0;
    QObject::connect(&dashboard, &DashboardViewModel::loadingChanged, &dashboard, [&]() { ++loadingChanges; });

    dashboard.refresh();
    EXPECT_TRUE(dashboard.loading());
    ASSERT_TRUE(waitUntil([&]() { return !dashboard.loading(); }));

    EXPECT_EQ(loadingChanges, 2);
    EXPECT_FALSE(dashboard.isDemoData());
    EXPECT_EQ(dashboard.totalInterviews(), 12);
    EXPECT_EQ(dashboard.averageScoreText().toStdString(), "91.3");
    EXPECT_EQ(dashboard.remainingToday(), 7);
    EXPECT_EQ(dashboard.interviewCount(), 3);
    EXPECT_EQ(cell(dashboard.interviews(), 0, InterviewListModel::InterviewIdRole).toStdString(), "i-1");
    EXPECT_EQ(cell(dashboard.interviews(), 0, InterviewListModel::DirectionRole).toStdString(), "qa");
}

TEST_F(ViewModelTest, DashboardFallsBackToSampleDataWhenTheApiFails) {
    serve([](const Request&) { return jsonResponse(500, {{"message", "boom"}}); });

    DashboardViewModel dashboard(interviewService);
    dashboard.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !dashboard.loading(); }));

    EXPECT_TRUE(dashboard.isDemoData());
    EXPECT_EQ(dashboard.totalInterviews(), demo::dashboardStats().totalInterviews);
    EXPECT_EQ(dashboard.averageScoreText().toStdString(), "82.5");
    EXPECT_EQ(dashboard.interviewCount(), static_cast<int>(demo::interviews().size()));
}

TEST_F(ViewModelTest, DashboardSkipsTheNetworkWhileTheBackendIsKnownToBeDown) {
    serve([](const Request&) { return jsonResponse(200, nlohmann::json::object()); });

    DashboardViewModel dashboard(interviewService);
    dashboard.setBackendOnlineProbe([]() { return false; });
    dashboard.refresh();

    // Sample data is applied synchronously and nothing is requested.
    EXPECT_FALSE(dashboard.loading());
    EXPECT_TRUE(dashboard.isDemoData());
    QCoreApplication::processEvents();
    EXPECT_TRUE(requests.isEmpty());
}

TEST_F(ViewModelTest, DashboardAverageScoreTextIsEmptyWithoutScores) {
    DashboardViewModel dashboard(interviewService);
    EXPECT_TRUE(dashboard.averageScoreText().isEmpty());
}

// ===========================================================================
// SetupViewModel
// ===========================================================================

TEST_F(ViewModelTest, SetupStartsTheInterviewAndOpensTheChatRoom) {
    nlohmann::json sentBody;
    serve([&sentBody](const Request& request) {
        sentBody = nlohmann::json::parse(request.body.toStdString(), nullptr, false);
        models::CreateInterviewResponse created;
        created.interviewId = "interview-42";
        created.firstQuestion = "What is a closure?";
        return jsonResponse(200, created);
    });

    NavigationViewModel navigation;
    SetupViewModel setup(interviewService, &navigation);
    setup.setDirection("backend");
    setup.setLevel("middle");
    setup.setLanguage("en");

    QString startedId, startedQuestion;
    bool startedOffline = true;
    QObject::connect(&setup, &SetupViewModel::interviewStarted, &setup,
                     [&](const QString& id, const QString&, const QString&, const QString&, const QString& question,
                         bool offline) {
                         startedId = id;
                         startedQuestion = question;
                         startedOffline = offline;
                     });

    setup.start();
    EXPECT_TRUE(setup.isBusy());
    ASSERT_TRUE(waitUntil([&]() { return !startedId.isEmpty(); }));

    EXPECT_FALSE(setup.isBusy());
    EXPECT_EQ(startedId.toStdString(), "interview-42");
    EXPECT_EQ(startedQuestion.toStdString(), "What is a closure?");
    EXPECT_FALSE(startedOffline);
    EXPECT_EQ(navigation.currentViewIndex(), NavigationViewModel::ChatRoom);
    EXPECT_EQ(navigation.activeInterviewId().toStdString(), "interview-42");

    // The chosen options reach the API as contract strings.
    ASSERT_EQ(requests.size(), 1);
    EXPECT_EQ(requests.first().toStdString(), "POST /api/interviews");
    EXPECT_EQ(sentBody.value("direction", ""), "backend");
    EXPECT_EQ(sentBody.value("level", ""), "middle");
    EXPECT_EQ(sentBody.value("language", ""), "en");
}

TEST_F(ViewModelTest, SetupReportsARequestTheServerRejected) {
    serve([](const Request&) { return jsonResponse(429, {{"message", "Daily interview limit reached"}}); });

    NavigationViewModel navigation;
    SetupViewModel setup(interviewService, &navigation);
    bool started = false;
    QObject::connect(&setup, &SetupViewModel::interviewStarted, &setup, [&]() { started = true; });

    setup.start();
    ASSERT_TRUE(waitUntil([&]() { return !setup.isBusy(); }));

    EXPECT_EQ(setup.errorMessage().toStdString(), "Daily interview limit reached");
    EXPECT_FALSE(started);
    EXPECT_EQ(navigation.currentViewIndex(), NavigationViewModel::Landing);
}

TEST_F(ViewModelTest, SetupStartsALocalDemoSessionWhenTheBackendIsUnreachable) {
    takeBackendDown();

    NavigationViewModel navigation;
    SetupViewModel setup(interviewService, &navigation);
    QString startedId, startedQuestion;
    bool startedOffline = false;
    QObject::connect(&setup, &SetupViewModel::interviewStarted, &setup,
                     [&](const QString& id, const QString&, const QString&, const QString&, const QString& question,
                         bool offline) {
                         startedId = id;
                         startedQuestion = question;
                         startedOffline = offline;
                     });

    setup.start();
    ASSERT_TRUE(waitUntil([&]() { return !startedId.isEmpty(); }, 20000));

    EXPECT_TRUE(startedId.startsWith("session-"));
    EXPECT_EQ(startedId.size(), 15);
    EXPECT_TRUE(startedQuestion.isEmpty());
    EXPECT_TRUE(startedOffline);
    EXPECT_TRUE(setup.errorMessage().isEmpty());
    EXPECT_EQ(navigation.currentViewIndex(), NavigationViewModel::ChatRoom);
}

// ===========================================================================
// ChatViewModel
// ===========================================================================

TEST_F(ViewModelTest, ChatSendsTheAnswerAndAppendsTheInterviewersReply) {
    nlohmann::json sentBody;
    serve([&sentBody](const Request& request) {
        sentBody = nlohmann::json::parse(request.body.toStdString(), nullptr, false);
        models::SendMessageResponse reply;
        reply.message = "How would you test that?";
        reply.questionCount = 2;
        return jsonResponse(200, reply);
    });

    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.beginSession("interview-42", "backend", "middle", "en", "What is a closure?", false);

    ASSERT_EQ(chat.messagesModel()->rowCount(), 1);
    EXPECT_EQ(cell(chat.messagesModel(), 0, MessageListModel::ContentRole).toStdString(), "What is a closure?");
    EXPECT_EQ(chat.currentDirection().toStdString(), "backend");
    EXPECT_EQ(chat.elapsedText().toStdString(), "00:00");

    chat.sendAnswer("  A function with its scope.  ");
    EXPECT_TRUE(chat.isBusy());
    EXPECT_EQ(chat.candidateAnswersCount(), 1);
    ASSERT_TRUE(waitUntil([&]() { return !chat.isBusy(); }));

    ASSERT_EQ(chat.messagesModel()->rowCount(), 3);
    EXPECT_EQ(cell(chat.messagesModel(), 1, MessageListModel::RoleRole).toStdString(), "user");
    EXPECT_EQ(cell(chat.messagesModel(), 1, MessageListModel::ContentRole).toStdString(), "A function with its scope.");
    EXPECT_EQ(cell(chat.messagesModel(), 2, MessageListModel::RoleRole).toStdString(), "assistant");
    EXPECT_EQ(cell(chat.messagesModel(), 2, MessageListModel::ContentRole).toStdString(), "How would you test that?");

    ASSERT_EQ(requests.size(), 1);
    EXPECT_EQ(requests.first().toStdString(), "POST /api/interviews/interview-42/message");
    EXPECT_EQ(sentBody.value("content", ""), "A function with its scope.");
}

TEST_F(ViewModelTest, ChatIgnoresBlankAnswersAndAnswersWithoutASession) {
    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);

    chat.sendAnswer("No session yet");
    EXPECT_EQ(chat.messagesModel()->rowCount(), 0);

    chat.beginSession("session-local", "frontend", "junior", "en", QString(), true);
    chat.sendAnswer("   \n  ");
    EXPECT_EQ(chat.candidateAnswersCount(), 0);
    EXPECT_FALSE(chat.isBusy());
}

TEST_F(ViewModelTest, ChatOfflineSessionUsesTheScriptedRepliesInOrder) {
    serve([](const Request&) { return jsonResponse(200, nlohmann::json::object()); });

    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.setFallbackReplyDelay(5);
    chat.beginSession("session-local", "devops", "junior", "en", QString(), true);

    // Without a first question the interviewer opens with the welcome line.
    ASSERT_EQ(chat.messagesModel()->rowCount(), 1);
    EXPECT_EQ(cell(chat.messagesModel(), 0, MessageListModel::ContentRole),
              demo::chatWelcome("devops", "junior", "en"));

    const QStringList replies = demo::chatReplies("en");
    chat.sendAnswer("First answer");
    ASSERT_TRUE(waitUntil([&]() { return !chat.isBusy(); }));
    EXPECT_EQ(cell(chat.messagesModel(), 2, MessageListModel::ContentRole), replies.at(0));

    chat.sendAnswer("Second answer");
    ASSERT_TRUE(waitUntil([&]() { return !chat.isBusy(); }));
    EXPECT_EQ(cell(chat.messagesModel(), 4, MessageListModel::ContentRole), replies.at(1));

    EXPECT_EQ(chat.candidateAnswersCount(), 2);
    // A local session never talks to the backend.
    EXPECT_TRUE(requests.isEmpty());
}

TEST_F(ViewModelTest, ChatKeepsTheLastScriptedReplyOnceTheScriptIsExhausted) {
    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.setFallbackReplyDelay(1);
    chat.beginSession("session-local", "frontend", "junior", "uk", QString(), true);

    const QStringList replies = demo::chatReplies("uk");
    for (int i = 0; i < replies.size() + 2; ++i) {
        chat.sendAnswer(QStringLiteral("Answer %1").arg(i));
        ASSERT_TRUE(waitUntil([&]() { return !chat.isBusy(); }));
    }

    const int lastRow = chat.messagesModel()->rowCount() - 1;
    EXPECT_EQ(cell(chat.messagesModel(), lastRow, MessageListModel::ContentRole), replies.last());
}

TEST_F(ViewModelTest, ChatFallsBackToAScriptedReplyWhenSendingFails) {
    serve([](const Request&) { return jsonResponse(500, {{"message", "model unavailable"}}); });

    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.setFallbackReplyDelay(5);
    chat.beginSession("interview-42", "frontend", "junior", "en", "First question", false);

    chat.sendAnswer("My answer");
    ASSERT_TRUE(waitUntil([&]() { return !chat.isBusy(); }));

    ASSERT_EQ(chat.messagesModel()->rowCount(), 3);
    EXPECT_EQ(cell(chat.messagesModel(), 2, MessageListModel::ContentRole), demo::chatReplies("en").at(0));
    EXPECT_EQ(requests.size(), 1);
}

TEST_F(ViewModelTest, ChatCompletionOpensTheReport) {
    serve([](const Request&) {
        models::CompleteInterviewResponse done;
        done.score = 80;
        return jsonResponse(200, done);
    });

    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.beginSession("interview-42", "frontend", "junior", "en", "First question", false);

    QString completedId;
    QObject::connect(&chat, &ChatViewModel::interviewCompleted, &chat, [&](const QString& id) { completedId = id; });

    chat.completeInterview();
    EXPECT_TRUE(chat.isCompleting());
    ASSERT_TRUE(waitUntil([&]() { return !completedId.isEmpty(); }));

    EXPECT_FALSE(chat.isCompleting());
    EXPECT_EQ(completedId.toStdString(), "interview-42");
    EXPECT_EQ(navigation.currentViewIndex(), NavigationViewModel::Report);
    EXPECT_EQ(navigation.activeInterviewId().toStdString(), "interview-42");
    EXPECT_EQ(requests.first().toStdString(), "POST /api/interviews/interview-42/complete");
}

TEST_F(ViewModelTest, ChatResumesAnInterviewFromItsTranscript) {
    serve([](const Request&) {
        models::InterviewDetail detail;
        detail.id = "interview-7";
        detail.direction = models::InterviewDirection::Pm;
        detail.level = models::InterviewLevel::Trainee;
        detail.language = models::InterviewLanguage::En;
        detail.transcript = {
            {"system", "hidden prompt", "2026-11-04T12:00:00Z"},
            {"assistant", "Describe a backlog.", "2026-11-04T12:00:01Z"},
            {"user", "A prioritised list.", "2026-11-04T12:01:00Z"},
        };
        return jsonResponse(200, detail);
    });

    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.openSession("interview-7");
    ASSERT_TRUE(waitUntil([&]() { return chat.messagesModel()->rowCount() == 2; }));

    EXPECT_EQ(chat.currentDirection().toStdString(), "pm");
    EXPECT_EQ(chat.currentLevel().toStdString(), "trainee");
    EXPECT_EQ(chat.sessionLanguage().toStdString(), "en");
    EXPECT_EQ(chat.candidateAnswersCount(), 1);
    EXPECT_EQ(requests.first().toStdString(), "GET /api/interviews/interview-7");
}

TEST_F(ViewModelTest, ChatANewSessionDiscardsThePreviousTranscript) {
    NavigationViewModel navigation;
    ChatViewModel chat(interviewService, &navigation);
    chat.setFallbackReplyDelay(1);
    chat.beginSession("session-a", "frontend", "junior", "en", QString(), true);
    chat.sendAnswer("Answer in the first session");
    ASSERT_TRUE(waitUntil([&]() { return !chat.isBusy(); }));

    chat.beginSession("session-b", "backend", "middle", "en", "Fresh question", true);

    ASSERT_EQ(chat.messagesModel()->rowCount(), 1);
    EXPECT_EQ(cell(chat.messagesModel(), 0, MessageListModel::ContentRole).toStdString(), "Fresh question");
    EXPECT_EQ(chat.candidateAnswersCount(), 0);
    EXPECT_EQ(chat.interviewId().toStdString(), "session-b");
}

// ===========================================================================
// ReportViewModel
// ===========================================================================

TEST_F(ViewModelTest, ReportShowsTheFeedbackOfTheInterview) {
    serve([](const Request&) {
        models::InterviewDetail detail;
        detail.id = "interview-42";
        models::InterviewFeedbackReport feedback;
        feedback.summary = "Solid answers.";
        feedback.score = 73;
        feedback.strengths = {"Clear structure", "Good examples"};
        feedback.weaknesses = {"Shallow on indexes"};
        feedback.recommendations = {{"SQL indexes", "https://use-the-index-luke.com"}};
        feedback.questionScores = {{"Explain ACID", 70, "Missed isolation levels."}};
        detail.feedback = feedback;
        return jsonResponse(200, detail);
    });

    ReportViewModel report(interviewService);
    EXPECT_FALSE(report.hasReport());

    report.load("interview-42");
    EXPECT_TRUE(report.loading());
    ASSERT_TRUE(waitUntil([&]() { return !report.loading(); }));

    EXPECT_TRUE(report.hasReport());
    EXPECT_FALSE(report.isDemoData());
    EXPECT_EQ(report.score(), 73);
    EXPECT_EQ(report.summary().toStdString(), "Solid answers.");
    EXPECT_EQ(report.strengths(), QStringList({"Clear structure", "Good examples"}));
    EXPECT_EQ(report.weaknesses(), QStringList({"Shallow on indexes"}));

    ASSERT_EQ(report.recommendations().size(), 1);
    const QVariantMap recommendation = report.recommendations().first().toMap();
    EXPECT_EQ(recommendation.value("topic").toString().toStdString(), "SQL indexes");
    EXPECT_EQ(recommendation.value("resource").toString().toStdString(), "https://use-the-index-luke.com");

    ASSERT_EQ(report.questionScores().size(), 1);
    const QVariantMap question = report.questionScores().first().toMap();
    EXPECT_EQ(question.value("question").toString().toStdString(), "Explain ACID");
    EXPECT_EQ(question.value("score").toInt(), 70);
    EXPECT_EQ(question.value("comment").toString().toStdString(), "Missed isolation levels.");
}

TEST_F(ViewModelTest, ReportOfALocalDemoSessionNeedsNoBackend) {
    serve([](const Request&) { return jsonResponse(200, nlohmann::json::object()); });

    ReportViewModel report(interviewService);
    report.load("session-k3x9p0a");

    EXPECT_FALSE(report.loading());
    EXPECT_TRUE(report.hasReport());
    EXPECT_TRUE(report.isDemoData());
    EXPECT_EQ(report.score(), demo::report("en").score);
    QCoreApplication::processEvents();
    EXPECT_TRUE(requests.isEmpty());
}

TEST_F(ViewModelTest, ReportUsesTheSampleWhenTheInterviewHasNoFeedbackYet) {
    serve([](const Request&) {
        models::InterviewDetail detail;
        detail.id = "interview-42";
        return jsonResponse(200, detail);
    });

    ReportViewModel report(interviewService);
    report.load("interview-42");
    ASSERT_TRUE(waitUntil([&]() { return !report.loading(); }));

    EXPECT_TRUE(report.isDemoData());
    EXPECT_EQ(report.strengths().size(), static_cast<qsizetype>(demo::report("en").strengths.size()));
}

TEST_F(ViewModelTest, ReportSampleFollowsTheInterfaceLanguage) {
    ReportViewModel report(interviewService);
    report.load("session-local");
    const QString english = report.summary();

    int reportChanges = 0;
    QObject::connect(&report, &ReportViewModel::reportChanged, &report, [&]() { ++reportChanges; });
    report.setUiLanguage("uk");

    EXPECT_EQ(reportChanges, 1);
    EXPECT_NE(report.summary(), english);
    EXPECT_EQ(report.summary(), QString::fromStdString(demo::report("uk").summary));
}

// ===========================================================================
// AdminViewModel
// ===========================================================================

TEST_F(ViewModelTest, AdminLoadsAPageOfQuestionsAndDerivesThePageCount) {
    serve([](const Request&) {
        models::PagedResult<models::QuestionBankItem> page;
        page.items = {bankQuestion("q-1", models::InterviewDirection::Frontend),
                      bankQuestion("q-2", models::InterviewDirection::Backend)};
        page.page = 1;
        page.pageSize = 20;
        page.totalCount = 45;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    EXPECT_FALSE(admin.isDemoData());
    EXPECT_EQ(admin.questionCount(), 2);
    EXPECT_EQ(admin.totalCount(), 45);
    EXPECT_EQ(admin.totalPages(), 3);
    EXPECT_EQ(cell(admin.questions(), 1, QuestionListModel::DirectionRole).toStdString(), "backend");
    EXPECT_EQ(requests.first().toStdString(), "GET /api/admin/questions?page=1&pageSize=20");
}

TEST_F(ViewModelTest, AdminTrackFilterReloadsFromTheFirstPage) {
    serve([](const Request&) {
        models::PagedResult<models::QuestionBankItem> page;
        page.items = {bankQuestion("q-2", models::InterviewDirection::Backend)};
        page.totalCount = 1;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.setDirectionFilter("backend");
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    EXPECT_EQ(admin.directionFilter().toStdString(), "backend");
    EXPECT_EQ(admin.page(), 1);
    ASSERT_EQ(requests.size(), 1);
    EXPECT_EQ(requests.first().toStdString(), "GET /api/admin/questions?direction=backend&page=1&pageSize=20");
}

TEST_F(ViewModelTest, AdminShowsFilteredSampleQuestionsWhenTheApiFails) {
    serve([](const Request&) { return jsonResponse(500, {{"message", "boom"}}); });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));
    EXPECT_TRUE(admin.isDemoData());
    EXPECT_EQ(admin.questionCount(), static_cast<int>(demo::questions("en").size()));

    admin.setDirectionFilter("backend");
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));
    ASSERT_EQ(admin.questionCount(), 1);
    EXPECT_EQ(cell(admin.questions(), 0, QuestionListModel::DirectionRole).toStdString(), "backend");

    admin.setDirectionFilter("qa");
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));
    EXPECT_EQ(admin.questionCount(), 0);
}

TEST_F(ViewModelTest, AdminCreatedQuestionAppearsFirstInTheList) {
    nlohmann::json sentBody;
    serve([&sentBody](const Request& request) {
        if (request.method == "POST") {
            sentBody = nlohmann::json::parse(request.body.toStdString(), nullptr, false);
            models::QuestionBankItem created = bankQuestion("q-server", models::InterviewDirection::DevOps);
            created.topic = "Containers";
            return jsonResponse(201, created);
        }
        models::PagedResult<models::QuestionBankItem> page;
        page.items = {bankQuestion("q-1", models::InterviewDirection::Frontend)};
        page.totalCount = 1;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    int created = 0;
    QObject::connect(&admin, &AdminViewModel::questionCreated, &admin, [&]() { ++created; });
    admin.createQuestion("devops", "middle", "  Containers  ", "  Why multi-stage builds?  ");
    EXPECT_TRUE(admin.isSubmitting());
    ASSERT_TRUE(waitUntil([&]() { return created == 1; }));

    EXPECT_FALSE(admin.isSubmitting());
    ASSERT_EQ(admin.questionCount(), 2);
    EXPECT_EQ(cell(admin.questions(), 0, QuestionListModel::QuestionIdRole).toStdString(), "q-server");
    EXPECT_EQ(sentBody.value("direction", ""), "devops");
    EXPECT_EQ(sentBody.value("level", ""), "middle");
    EXPECT_EQ(sentBody.value("stage", ""), "hard_skills");
    EXPECT_EQ(sentBody.value("topic", ""), "Containers");
    EXPECT_EQ(sentBody.value("questionText", ""), "Why multi-stage builds?");
}

TEST_F(ViewModelTest, AdminKeepsANewQuestionLocallyWhenSavingFails) {
    serve([](const Request& request) {
        if (request.method == "POST") {
            return jsonResponse(500, {{"message", "boom"}});
        }
        models::PagedResult<models::QuestionBankItem> page;
        page.totalCount = 0;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    int created = 0;
    QObject::connect(&admin, &AdminViewModel::questionCreated, &admin, [&]() { ++created; });
    admin.createQuestion("qa", "junior", "Test design", "What is boundary value analysis?");
    ASSERT_TRUE(waitUntil([&]() { return created == 1; }));

    ASSERT_EQ(admin.questionCount(), 1);
    EXPECT_TRUE(cell(admin.questions(), 0, QuestionListModel::QuestionIdRole).startsWith("q-"));
    EXPECT_EQ(cell(admin.questions(), 0, QuestionListModel::TopicRole).toStdString(), "Test design");
    EXPECT_TRUE(admin.questions()->data(admin.questions()->index(0, 0), QuestionListModel::IsActiveRole).toBool());
}

TEST_F(ViewModelTest, AdminRejectsQuestionsWithoutTopicOrText) {
    AdminViewModel admin(adminService, authService);
    int created = 0;
    QObject::connect(&admin, &AdminViewModel::questionCreated, &admin, [&]() { ++created; });

    admin.createQuestion("qa", "junior", "   ", "Some text");
    admin.createQuestion("qa", "junior", "Topic", "");

    EXPECT_EQ(created, 0);
    EXPECT_EQ(admin.questionCount(), 0);
    EXPECT_FALSE(admin.isSubmitting());
}

TEST_F(ViewModelTest, AdminDeleteRemovesTheRowAndTellsTheServer) {
    serve([](const Request& request) {
        if (request.method == "DELETE") {
            return jsonResponse(200, nlohmann::json::object());
        }
        models::PagedResult<models::QuestionBankItem> page;
        page.items = {bankQuestion("q-1", models::InterviewDirection::Frontend),
                      bankQuestion("q-2", models::InterviewDirection::Backend)};
        page.totalCount = 2;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    admin.deleteQuestion("q-1");
    ASSERT_EQ(admin.questionCount(), 1);
    EXPECT_EQ(cell(admin.questions(), 0, QuestionListModel::QuestionIdRole).toStdString(), "q-2");
    ASSERT_TRUE(waitUntil([&]() { return requests.contains("DELETE /api/admin/questions/q-1"); }));
}

TEST_F(ViewModelTest, AdminActiveSwitchIsRevertedWhenTheServerRefuses) {
    serve([](const Request& request) {
        if (request.method == "PUT") {
            return jsonResponse(400, {{"message", "Question is used by an active interview"}});
        }
        models::PagedResult<models::QuestionBankItem> page;
        page.items = {bankQuestion("q-1", models::InterviewDirection::Frontend)};
        page.totalCount = 1;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    const auto isActive = [&]() {
        return admin.questions()->data(admin.questions()->index(0, 0), QuestionListModel::IsActiveRole).toBool();
    };

    admin.setQuestionActive("q-1", false);
    EXPECT_FALSE(isActive()); // switched optimistically
    ASSERT_TRUE(waitUntil([&]() { return !admin.errorMessage().isEmpty(); }));

    EXPECT_TRUE(isActive());
    EXPECT_EQ(admin.errorMessage().toStdString(), "Question is used by an active interview");
    EXPECT_TRUE(requests.contains("PUT /api/admin/questions/q-1"));
}

TEST_F(ViewModelTest, AdminActiveSwitchSendsOnlyTheChangedField) {
    nlohmann::json sentBody;
    serve([&sentBody](const Request& request) {
        if (request.method == "PUT") {
            sentBody = nlohmann::json::parse(request.body.toStdString(), nullptr, false);
            models::QuestionBankItem updated = bankQuestion("q-1", models::InterviewDirection::Frontend);
            updated.isActive = false;
            return jsonResponse(200, updated);
        }
        models::PagedResult<models::QuestionBankItem> page;
        page.items = {bankQuestion("q-1", models::InterviewDirection::Frontend)};
        page.totalCount = 1;
        return jsonResponse(200, page);
    });

    AdminViewModel admin(adminService, authService);
    admin.refresh();
    ASSERT_TRUE(waitUntil([&]() { return !admin.loading(); }));

    admin.setQuestionActive("q-1", false);
    ASSERT_TRUE(waitUntil([&]() { return !sentBody.is_null(); }));

    EXPECT_EQ(sentBody.size(), 1u);
    EXPECT_EQ(sentBody.value("isActive", true), false);
    EXPECT_TRUE(admin.errorMessage().isEmpty());
}

// ===========================================================================
// BackendStatusViewModel
// ===========================================================================

TEST_F(ViewModelTest, BackendStatusFollowsTheHealthCheck) {
    serve([](const Request&) { return jsonResponse(200, {{"status", "ok"}}); });

    BackendStatusViewModel status(client);
    ASSERT_TRUE(waitUntil([&]() { return requests.contains("GET /api/health/ping"); }));
    EXPECT_FALSE(status.isOffline());

    int statusChanges = 0;
    QObject::connect(&status, &BackendStatusViewModel::statusChanged, &status, [&]() { ++statusChanges; });

    takeBackendDown();
    status.checkHealth();
    EXPECT_TRUE(status.isChecking());
    ASSERT_TRUE(waitUntil([&]() { return status.isOffline(); }, 20000));
    EXPECT_EQ(statusChanges, 1);
    EXPECT_FALSE(status.isChecking());
}

TEST_F(ViewModelTest, BackendStatusBannerCanBeDismissedAndReopened) {
    BackendStatusViewModel status(client);
    int dismissedChanges = 0;
    QObject::connect(&status, &BackendStatusViewModel::dismissedChanged, &status, [&]() { ++dismissedChanges; });

    status.dismissBanner();
    status.dismissBanner();
    EXPECT_TRUE(status.isDismissed());
    EXPECT_EQ(dismissedChanges, 1);

    status.reopenBanner();
    EXPECT_FALSE(status.isDismissed());
    EXPECT_EQ(dismissedChanges, 2);
}

// ===========================================================================
// SettingsViewModel
// ===========================================================================

// Keeps the preferences of these tests in a temporary INI file instead of
// the user's real settings.
class SettingsViewModelTest : public ViewModelTest {
protected:
    void SetUp() override {
        ViewModelTest::SetUp();
        ASSERT_TRUE(settingsDir.isValid());
        QCoreApplication::setOrganizationName("DevDialogueTests");
        QCoreApplication::setApplicationName("ViewModels");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    }

    QTemporaryDir settingsDir;
};

TEST_F(SettingsViewModelTest, ProfileComesFromTheSignedInUser) {
    SettingsViewModel settings(authService);
    EXPECT_TRUE(settings.displayName().isEmpty());
    EXPECT_TRUE(settings.linkedProviders().isEmpty());

    int profileChanges = 0;
    QObject::connect(&settings, &SettingsViewModel::profileChanged, &settings, [&]() { ++profileChanges; });
    authService->mockLogin(models::UserRole::User);

    EXPECT_EQ(profileChanges, 1);
    EXPECT_EQ(settings.displayName().toStdString(), "Mock Candidate");
    EXPECT_EQ(settings.email().toStdString(), "candidate@devdialogue.local");
    EXPECT_EQ(settings.role().toStdString(), "user");
    EXPECT_TRUE(settings.avatarUrl().isEmpty());
}

TEST_F(SettingsViewModelTest, SavedProfileOverridesTheSignedInProfileAndPersists) {
    authService->mockLogin(models::UserRole::User);
    {
        SettingsViewModel settings(authService);
        settings.saveProfile("  dev_nick  ", "nick@example.com", "https://example.com/a.png");

        EXPECT_EQ(settings.displayName().toStdString(), "dev_nick");
        EXPECT_EQ(settings.email().toStdString(), "nick@example.com");
        EXPECT_EQ(settings.avatarUrl().toStdString(), "https://example.com/a.png");
    }

    // A new instance (next app start) still sees the saved values.
    SettingsViewModel reopened(authService);
    EXPECT_EQ(reopened.displayName().toStdString(), "dev_nick");

    // Clearing a field falls back to the signed-in profile.
    reopened.saveProfile("", "", "");
    EXPECT_EQ(reopened.displayName().toStdString(), "Mock Candidate");
    EXPECT_EQ(reopened.email().toStdString(), "candidate@devdialogue.local");
}

TEST_F(SettingsViewModelTest, LinkedProvidersCanBeToggled) {
    authService->mockLogin(models::UserRole::User);
    SettingsViewModel settings(authService);
    EXPECT_EQ(settings.linkedProviders(), QStringList({"email", "github"}));

    int providerChanges = 0;
    QObject::connect(&settings, &SettingsViewModel::linkedProvidersChanged, &settings, [&]() { ++providerChanges; });

    EXPECT_TRUE(settings.toggleProvider("google"));
    EXPECT_TRUE(settings.linkedProviders().contains("google"));
    EXPECT_FALSE(settings.toggleProvider("github"));
    EXPECT_FALSE(settings.linkedProviders().contains("github"));
    EXPECT_EQ(providerChanges, 2);
}

TEST_F(SettingsViewModelTest, PlatformPreferencesHaveDefaultsAndPersist) {
    {
        SettingsViewModel settings(authService);
        EXPECT_EQ(settings.theme().toStdString(), "oled");
        EXPECT_TRUE(settings.soundEnabled());
        EXPECT_EQ(settings.aiPersona().toStdString(), "balanced");

        int themeChanges = 0;
        QObject::connect(&settings, &SettingsViewModel::themeChanged, &settings, [&]() { ++themeChanges; });
        settings.setTheme("slate");
        settings.setTheme("slate");
        settings.setSoundEnabled(false);
        settings.setAiPersona("strict");
        EXPECT_EQ(themeChanges, 1);
    }

    SettingsViewModel reopened(authService);
    EXPECT_EQ(reopened.theme().toStdString(), "slate");
    EXPECT_FALSE(reopened.soundEnabled());
    EXPECT_EQ(reopened.aiPersona().toStdString(), "strict");
}

TEST_F(SettingsViewModelTest, NothingIsStoredWithoutASignedInUser) {
    SettingsViewModel settings(authService);
    settings.saveProfile("ghost", "ghost@example.com", "");
    EXPECT_FALSE(settings.toggleProvider("github"));

    EXPECT_TRUE(settings.displayName().isEmpty());
    EXPECT_TRUE(settings.linkedProviders().isEmpty());
}
