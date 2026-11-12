#include <QtTest>

#include <memory>

#include "models/InterviewListModel.hpp"
#include "models/MessageListModel.hpp"
#include "models/QuestionListModel.hpp"
#include "network/ApiClient.hpp"
#include "services/AdminService.hpp"
#include "services/AuthService.hpp"
#include "services/InterviewService.hpp"
#include "viewmodels/AdminViewModel.hpp"
#include "viewmodels/AuthViewModel.hpp"
#include "viewmodels/ChatViewModel.hpp"
#include "viewmodels/DashboardViewModel.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/ReportViewModel.hpp"
#include "viewmodels/SetupViewModel.hpp"

namespace models = devdialogue::models;
namespace network = devdialogue::network;
using devdialogue::models::InterviewDirection;
using devdialogue::models::InterviewLanguage;
using devdialogue::models::InterviewLevel;
using devdialogue::viewmodels::AdminViewModel;
using devdialogue::viewmodels::AuthViewModel;
using devdialogue::viewmodels::ChatViewModel;
using devdialogue::viewmodels::DashboardViewModel;
using devdialogue::viewmodels::NavigationViewModel;
using devdialogue::viewmodels::ReportViewModel;
using devdialogue::viewmodels::SetupViewModel;

class TestViewModels : public QObject {
  Q_OBJECT

private slots:
  // Navigation ---------------------------------------------------------
  void navigationStartsOnLanding();
  void navigationSwitchesBetweenViews();
  void navigationTracksTheActiveInterview();

  // Authentication -----------------------------------------------------
  void adminViewToggleEmitsSignal();
  void mockLoginPopulatesTheProfile();
  void logoutClearsTheSession();
  void loginWithEmailReportsTheDelegatedFlow();

  // Interview flow -----------------------------------------------------
  void setupCreatesAnInterviewAndOpensTheChat();
  void chatAppendsTheAnswerAndTheReply();
  void completeInterviewOpensTheReport();

  // Dashboard ----------------------------------------------------------
  void dashboardLoadsLocalStatsAndHistory();

  // Administration -----------------------------------------------------
  void adminCreatesTogglesAndDeletesQuestions();
};

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void TestViewModels::navigationStartsOnLanding() {
  NavigationViewModel navigation;

  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::Landing));
  QVERIFY(navigation.activeInterviewId().isEmpty());
}

void TestViewModels::navigationSwitchesBetweenViews() {
  NavigationViewModel navigation;
  QSignalSpy spy(&navigation, &NavigationViewModel::currentViewIndexChanged);

  navigation.navigateToDashboard();
  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::Dashboard));

  navigation.navigateToSetup();
  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::Setup));

  navigation.navigateToAdmin();
  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::Admin));

  QCOMPARE(spy.count(), 3);
}

void TestViewModels::navigationTracksTheActiveInterview() {
  NavigationViewModel navigation;
  QSignalSpy spy(&navigation, &NavigationViewModel::activeInterviewIdChanged);

  navigation.navigateToChat("interview-42");

  QCOMPARE(navigation.activeInterviewId(), QStringLiteral("interview-42"));
  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::ChatRoom));
  QCOMPARE(spy.count(), 1);
}

// ---------------------------------------------------------------------------
// Authentication
// ---------------------------------------------------------------------------

void TestViewModels::adminViewToggleEmitsSignal() {
  auto authService = std::make_shared<devdialogue::services::AuthService>(
      nullptr, nullptr, nullptr);
  AuthViewModel viewModel(authService);

  QSignalSpy spy(&viewModel, &AuthViewModel::adminViewChanged);
  QVERIFY(viewModel.isAdminViewActive());

  viewModel.toggleAdminView();
  QCOMPARE(spy.count(), 1);
  QVERIFY(!viewModel.isAdminViewActive());

  viewModel.toggleAdminView();
  QCOMPARE(spy.count(), 2);
  QVERIFY(viewModel.isAdminViewActive());
}

void TestViewModels::mockLoginPopulatesTheProfile() {
  auto authService = std::make_shared<devdialogue::services::AuthService>(
      nullptr, nullptr, nullptr);
  AuthViewModel viewModel(authService);

  QSignalSpy spy(&viewModel, &AuthViewModel::authStateChanged);

  viewModel.mockLogin("admin");

  QVERIFY(viewModel.isAuthenticated());
  QVERIFY(viewModel.isAdmin());
  QVERIFY(viewModel.isDemoSession());
  QCOMPARE(viewModel.userFullName(), QStringLiteral("Yaroslav (Admin)"));
  QCOMPARE(viewModel.userInitials(), QStringLiteral("YA"));
  QCOMPARE(spy.count(), 1);

  viewModel.mockLogin("candidate");
  QVERIFY(viewModel.isAuthenticated());
  QVERIFY(!viewModel.isAdmin());
}

void TestViewModels::logoutClearsTheSession() {
  auto authService = std::make_shared<devdialogue::services::AuthService>(
      nullptr, nullptr, nullptr);
  AuthViewModel viewModel(authService);

  viewModel.mockLogin("admin");
  QVERIFY(viewModel.isAuthenticated());

  viewModel.logout();
  QVERIFY(!viewModel.isAuthenticated());
  QVERIFY(!viewModel.isDemoSession());
  QVERIFY(viewModel.userFullName().isEmpty());
}

void TestViewModels::loginWithEmailReportsTheDelegatedFlow() {
  auto authService = std::make_shared<devdialogue::services::AuthService>(
      nullptr, nullptr, nullptr);
  AuthViewModel viewModel(authService);

  viewModel.loginWithEmail(QStringLiteral("user@example.com"),
                            QStringLiteral("secret"));

  QVERIFY(!viewModel.errorMessage().isEmpty());
  QVERIFY(!viewModel.isAuthenticated());

  viewModel.clearError();
  QVERIFY(viewModel.errorMessage().isEmpty());
}

// ---------------------------------------------------------------------------
// Interview flow
// ---------------------------------------------------------------------------

void TestViewModels::setupCreatesAnInterviewAndOpensTheChat() {
  NavigationViewModel navigation;
  auto interviewService =
      std::make_shared<devdialogue::services::InterviewService>(nullptr,
                                                                nullptr);
  SetupViewModel setup(interviewService, &navigation);

  QSignalSpy created(&setup, &SetupViewModel::interviewCreated);

  setup.setDirection("backend");
  setup.setLevel("middle");
  setup.setLanguage("en");
  setup.startInterview();

  QCOMPARE(created.count(), 1);
  QVERIFY(!created.at(0).at(0).toString().isEmpty());
  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::ChatRoom));
  QVERIFY(!setup.isBusy());
  QVERIFY(setup.errorMessage().isEmpty());
}

void TestViewModels::chatAppendsTheAnswerAndTheReply() {
  NavigationViewModel navigation;
  auto interviewService =
      std::make_shared<devdialogue::services::InterviewService>(nullptr,
                                                                nullptr);
  ChatViewModel chat(interviewService, &navigation);

  // An unknown interview cannot be resolved offline.
  chat.loadInterview(QStringLiteral("local-unknown"));
  QVERIFY(!chat.errorMessage().isEmpty());

  QString interviewId;
  interviewService->createInterview(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En,
      [&interviewId](const std::variant<models::InterviewDetail,
                                        network::ApiError> &result) {
        const auto *created = std::get_if<models::InterviewDetail>(&result);
        if (created)
          interviewId = QString::fromStdString(created->id);
      });

  chat.loadInterview(interviewId);
  QVERIFY(chat.errorMessage().isEmpty());
  QCOMPARE(chat.messagesModel()->count(), 1);

  chat.sendAnswer(QStringLiteral(
      "Microtasks are drained after the current task completes."));

  QCOMPARE(chat.messagesModel()->count(), 3);
  QCOMPARE(chat.candidateAnswersCount(), 1);
  QCOMPARE(chat.questionCount(), 2);
  QVERIFY(!chat.isCompleted());
}

void TestViewModels::completeInterviewOpensTheReport() {
  NavigationViewModel navigation;
  auto interviewService =
      std::make_shared<devdialogue::services::InterviewService>(nullptr,
                                                                nullptr);
  ChatViewModel chat(interviewService, &navigation);

  models::InterviewDetail created;
  interviewService->createInterview(
      InterviewDirection::Frontend, InterviewLevel::Junior,
      InterviewLanguage::En,
      [&created](const std::variant<models::InterviewDetail,
                                     network::ApiError> &result) {
        const auto *detail = std::get_if<models::InterviewDetail>(&result);
        if (detail)
          created = *detail;
      });

  chat.loadInterview(QString::fromStdString(created.id));
  chat.sendAnswer(QStringLiteral("A reasonably detailed answer about React."));
  chat.completeInterview();

  QVERIFY(chat.isCompleted());
  QCOMPARE(navigation.currentViewIndex(),
           static_cast<int>(NavigationViewModel::ViewIndex::Report));

  ReportViewModel report(interviewService, &navigation);
  report.loadReport(QString::fromStdString(created.id));

  QVERIFY(report.hasReport());
  QVERIFY(report.score() > 0);
  QVERIFY(!report.summary().isEmpty());
  QVERIFY(!report.strengths().isEmpty());
  QVERIFY(!report.questionScores().isEmpty());
}

// ---------------------------------------------------------------------------
// Dashboard
// ---------------------------------------------------------------------------

void TestViewModels::dashboardLoadsLocalStatsAndHistory() {
  NavigationViewModel navigation;
  auto interviewService =
      std::make_shared<devdialogue::services::InterviewService>(nullptr,
                                                                nullptr);
  DashboardViewModel dashboard(interviewService, &navigation);

  dashboard.refresh();

  QVERIFY(!dashboard.isBusy());
  QVERIFY(dashboard.errorMessage().isEmpty());
  QCOMPARE(dashboard.dailyLimit(), 10);
  QVERIFY(dashboard.interviewsModel()->count() >= 0);
}

// ---------------------------------------------------------------------------
// Administration
// ---------------------------------------------------------------------------

void TestViewModels::adminCreatesTogglesAndDeletesQuestions() {
  auto adminService =
      std::make_shared<devdialogue::services::AdminService>(nullptr, nullptr);
  AdminViewModel admin(adminService);

  admin.refresh();
  const int initialCount = admin.questionsModel()->count();
  QVERIFY(initialCount > 0);

  admin.createQuestion("frontend", "hard_skills", "middle", "Test topic",
                       "Test question text?");
  QCOMPARE(admin.successMessage(),
           QStringLiteral("Question added to the bank."));
  QCOMPARE(admin.questionsModel()->count(), initialCount + 1);

  // The bank is sorted by creation date, but look the row up by topic so the
  // assertion does not depend on the ordering.
  QModelIndex created;
  for (int row = 0; row < admin.questionsModel()->rowCount(); ++row) {
    const QModelIndex candidate = admin.questionsModel()->index(row, 0);
    if (admin.questionsModel()
            ->data(candidate, devdialogue::models::QuestionListModel::TopicRole)
            .toString() == QStringLiteral("Test topic")) {
      created = candidate;
      break;
    }
  }
  QVERIFY(created.isValid());

  const QString newId =
      admin.questionsModel()
          ->data(created, devdialogue::models::QuestionListModel::IdRole)
          .toString();
  QVERIFY(!newId.isEmpty());

  admin.setQuestionActive(newId, false);
  QCOMPARE(admin.successMessage(), QStringLiteral("Question deactivated."));
  QVERIFY(!admin.questionsModel()
               ->data(created, devdialogue::models::QuestionListModel::IsActiveRole)
               .toBool());

  admin.setQuestionActive(newId, true);
  QCOMPARE(admin.successMessage(), QStringLiteral("Question activated."));
  QVERIFY(admin.questionsModel()
              ->data(created, devdialogue::models::QuestionListModel::IsActiveRole)
              .toBool());

  admin.deleteQuestion(newId);
  QCOMPARE(admin.successMessage(), QStringLiteral("Question deleted."));
  QCOMPARE(admin.questionsModel()->count(), initialCount);
}

QTEST_GUILESS_MAIN(TestViewModels)

#include "TestViewModels.moc"
