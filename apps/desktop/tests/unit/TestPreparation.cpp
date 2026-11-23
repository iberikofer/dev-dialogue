#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <ostream>

#include "models/PreparationModels.hpp"
#include "services/DemoPreparationService.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/PreparationViewModel.hpp"

using namespace devdialogue;
using namespace devdialogue::models;
using namespace devdialogue::services;
using namespace devdialogue::viewmodels;

namespace {

inline void PrintTo(const QString& text, std::ostream* out) {
    *out << '"' << text.toStdString() << '"';
}

bool waitUntil(const std::function<bool()>& condition, int timeoutMs = 4000) {
    QElapsedTimer timer;
    timer.start();
    while (!condition()) {
        if (timer.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// DemoPreparationService Tests
// ---------------------------------------------------------------------------

TEST(TestPreparationService, TopicBankCompletenessAndValidUrls) {
    DemoPreparationService service;
    const QStringList directions = {
        QStringLiteral("Frontend"),
        QStringLiteral("Backend"),
        QStringLiteral("DevOps"),
        QStringLiteral("QA"),
        QStringLiteral("PM")
    };

    for (const auto& dir : directions) {
        auto result = service.evaluateSynchronously(dir, QStringLiteral("junior"),
            QStringLiteral("General experience with programming."), QStringLiteral("en"));
        EXPECT_GE(result.topics.size(), 8) << "Track " << dir.toStdString() << " should have at least 8 topics";
        EXPECT_LE(result.topics.size(), 10) << "Track " << dir.toStdString() << " should have at most 10 topics";

        for (const auto& topic : result.topics) {
            EXPECT_FALSE(topic.id.isEmpty());
            EXPECT_FALSE(topic.title.isEmpty());
            EXPECT_FALSE(topic.category.isEmpty());
            EXPECT_FALSE(topic.materials.isEmpty()) << "Topic " << topic.id.toStdString() << " has no materials";

            for (const auto& mat : topic.materials) {
                EXPECT_FALSE(mat.title.isEmpty());
                EXPECT_TRUE(mat.url.startsWith(QStringLiteral("https://")))
                    << "Material URL must start with https://, got: " << mat.url.toStdString();
            }
        }
    }
}

TEST(TestPreparationService, DetectsKnownKeywordsEnglish) {
    DemoPreparationService service;
    const QString description = QStringLiteral(
        "I have solid experience with React, Redux toolkit, TypeScript, HTML semantic tags, and Tailwind CSS.");
    auto result = service.evaluateSynchronously(QStringLiteral("Frontend"), QStringLiteral("junior"), description, QStringLiteral("en"));

    int knownCount = 0;
    for (const auto& topic : result.topics) {
        if (topic.id == "fe-react-core" || topic.id == "fe-state-mgmt" || topic.id == "fe-js-ts" || topic.id == "fe-html-css") {
            EXPECT_EQ(topic.status, PreparationTopicStatus::Known)
                << "Expected topic " << topic.id.toStdString() << " to be Known";
            knownCount++;
        }
    }
    EXPECT_GE(knownCount, 4);
}

TEST(TestPreparationService, DetectsKnownKeywordsUkrainian) {
    DemoPreparationService service;
    const QString description = QStringLiteral(
        "Пишу на C# та ASP.NET Core, використовую Entity Framework та PostgreSQL, проектую REST API.");
    auto result = service.evaluateSynchronously(QStringLiteral("Backend"), QStringLiteral("junior"), description, QStringLiteral("uk"));

    int knownCount = 0;
    for (const auto& topic : result.topics) {
        if (topic.id == "be-csharp" || topic.id == "be-aspnet" || topic.id == "be-sql-db" || topic.id == "be-efcore") {
            EXPECT_EQ(topic.status, PreparationTopicStatus::Known)
                << "Expected topic " << topic.id.toStdString() << " to be Known in Ukrainian text";
            knownCount++;
        }
    }
    EXPECT_GE(knownCount, 3);
}

TEST(TestPreparationService, NegationPreventsKnownStatus) {
    DemoPreparationService service;

    // Negations in Ukrainian: "не знаю", "не маю досвіду"
    {
        const QString ukDesc = QStringLiteral(
            "Працюю з Linux та CI/CD, але не знаю Docker і ніколи не працював з Kubernetes. Також не маю досвіду з Terraform.");
        auto result = service.evaluateSynchronously(QStringLiteral("DevOps"), QStringLiteral("junior"), ukDesc, QStringLiteral("uk"));

        for (const auto& topic : result.topics) {
            if (topic.id == "do-docker" || topic.id == "do-k8s" || topic.id == "do-iac") {
                EXPECT_NE(topic.status, PreparationTopicStatus::Known)
                    << "Topic " << topic.id.toStdString() << " should NOT be marked Known due to Ukrainian negation";
            }
            if (topic.id == "do-linux" || topic.id == "do-cicd") {
                EXPECT_EQ(topic.status, PreparationTopicStatus::Known);
            }
        }
    }

    // Negations in English: "never used", "don't know", "haven't touched"
    {
        const QString enDesc = QStringLiteral(
            "I know Linux and CI/CD pipelines, but I never used Docker, do not know Kubernetes, and haven't used Terraform.");
        auto result = service.evaluateSynchronously(QStringLiteral("DevOps"), QStringLiteral("junior"), enDesc, QStringLiteral("en"));

        for (const auto& topic : result.topics) {
            if (topic.id == "do-docker" || topic.id == "do-k8s" || topic.id == "do-iac") {
                EXPECT_NE(topic.status, PreparationTopicStatus::Known)
                    << "Topic " << topic.id.toStdString() << " should NOT be marked Known due to English negation";
            }
        }
    }
}

TEST(TestPreparationService, QualifiersMarkAsUnclear) {
    DemoPreparationService service;

    // Ukrainian qualifier: "трохи чув", "базово"
    {
        const QString desc = QStringLiteral("Трохи знаю Docker і базово чув про Kubernetes.");
        auto result = service.evaluateSynchronously(QStringLiteral("DevOps"), QStringLiteral("junior"), desc, QStringLiteral("uk"));

        for (const auto& topic : result.topics) {
            if (topic.id == "do-docker" || topic.id == "do-k8s") {
                EXPECT_EQ(topic.status, PreparationTopicStatus::Unclear)
                    << "Topic " << topic.id.toStdString() << " should be marked Unclear with qualifier words";
            }
        }
    }

    // English qualifier: "basic knowledge of", "heard of"
    {
        const QString desc = QStringLiteral("I have basic knowledge of Docker and only heard of Kubernetes.");
        auto result = service.evaluateSynchronously(QStringLiteral("DevOps"), QStringLiteral("junior"), desc, QStringLiteral("en"));

        for (const auto& topic : result.topics) {
            if (topic.id == "do-docker" || topic.id == "do-k8s") {
                EXPECT_EQ(topic.status, PreparationTopicStatus::Unclear)
                    << "Topic " << topic.id.toStdString() << " should be marked Unclear with English qualifier";
            }
        }
    }
}

TEST(TestPreparationService, DeterministicEvaluation) {
    DemoPreparationService service;
    const QString desc = QStringLiteral(
        "Knowledge of manual testing, test design techniques like equivalence partitioning, Postman, SQL, and Git.");

    auto result1 = service.evaluateSynchronously(QStringLiteral("QA"), QStringLiteral("junior"), desc, QStringLiteral("en"));
    auto result2 = service.evaluateSynchronously(QStringLiteral("QA"), QStringLiteral("junior"), desc, QStringLiteral("en"));

    ASSERT_EQ(result1.topics.size(), result2.topics.size());
    for (qsizetype i = 0; i < result1.topics.size(); ++i) {
        EXPECT_EQ(result1.topics[i].id, result2.topics[i].id);
        EXPECT_EQ(result1.topics[i].status, result2.topics[i].status);
        EXPECT_EQ(result1.topics[i].priority, result2.topics[i].priority);
    }
}

TEST(TestPreparationService, AsyncAnalyzeProfileCallbackWorks) {
    auto service = std::make_shared<DemoPreparationService>();
    bool completed = false;
    PreparationPlanResult captured;

    service->analyzeProfile(QStringLiteral("Backend"), QStringLiteral("junior"),
        QStringLiteral("Writing REST services in ASP.NET Core with PostgreSQL."), QStringLiteral("en"),
        [&](const PreparationPlanResult& res, const QString&) {
            captured = res;
            completed = true;
        });

    EXPECT_TRUE(waitUntil([&]() { return completed; }, 3000));
    EXPECT_FALSE(captured.topics.empty());
}

// ---------------------------------------------------------------------------
// PreparationViewModel Tests
// ---------------------------------------------------------------------------

TEST(TestPreparationViewModel, InitialState) {
    auto service = std::make_shared<DemoPreparationService>();
    PreparationViewModel vm(service);

    EXPECT_EQ(vm.direction(), QStringLiteral("frontend"));
    EXPECT_EQ(vm.level(), QStringLiteral("junior"));
    EXPECT_EQ(vm.language(), QStringLiteral("uk"));
    EXPECT_EQ(vm.uiLanguage(), QStringLiteral("uk"));
    vm.setUiLanguage(QStringLiteral("en"));
    EXPECT_EQ(vm.uiLanguage(), QStringLiteral("en"));
    EXPECT_EQ(vm.language(), QStringLiteral("en"));
    EXPECT_EQ(vm.charCount(), 0);
    EXPECT_EQ(vm.minCharCount(), 30);
    EXPECT_EQ(vm.maxCharCount(), 3000);
    EXPECT_FALSE(vm.isValidInput());
    EXPECT_FALSE(vm.isAnalyzing());
    EXPECT_FALSE(vm.hasResult());
    EXPECT_TRUE(vm.errorMessage().isEmpty());
}

TEST(TestPreparationViewModel, ValidationRules) {
    auto service = std::make_shared<DemoPreparationService>();
    PreparationViewModel vm(service);

    // Below min length
    vm.setUserDescription(QStringLiteral("Short text"));
    EXPECT_EQ(vm.charCount(), 10);
    EXPECT_FALSE(vm.isValidInput());
    EXPECT_FALSE(vm.validationMessage().isEmpty());

    // Valid range
    vm.setUserDescription(QStringLiteral("I know React, TypeScript, modern CSS and standard web APIs."));
    EXPECT_GT(vm.charCount(), 30);
    EXPECT_LE(vm.charCount(), 3000);
    EXPECT_TRUE(vm.isValidInput());

    // Exceeding maximum
    QString hugeText(3005, 'x');
    vm.setUserDescription(hugeText);
    EXPECT_EQ(vm.charCount(), 3005);
    EXPECT_FALSE(vm.isValidInput());
}

TEST(TestPreparationViewModel, AnalyzeLifecycleAndResultExposure) {
    auto service = std::make_shared<DemoPreparationService>();
    PreparationViewModel vm(service);

    vm.setDirection(QStringLiteral("Frontend"));
    vm.setLevel(QStringLiteral("junior"));
    vm.setLanguage(QStringLiteral("en"));
    vm.setUserDescription(QStringLiteral("I am confident in React, JavaScript, HTML, CSS, but I need to learn Next.js and testing."));

    int analyzingSignalCount = 0;
    int hasResultSignalCount = 0;
    QObject::connect(&vm, &PreparationViewModel::analyzingChanged, [&]() { analyzingSignalCount++; });
    QObject::connect(&vm, &PreparationViewModel::resultChanged, [&]() { hasResultSignalCount++; });

    EXPECT_TRUE(vm.isValidInput());
    vm.analyze();

    EXPECT_TRUE(vm.isAnalyzing());
    EXPECT_EQ(analyzingSignalCount, 1);

    // Wait for async analysis to settle
    EXPECT_TRUE(waitUntil([&]() { return vm.hasResult(); }, 3000));
    EXPECT_FALSE(vm.isAnalyzing());
    EXPECT_EQ(hasResultSignalCount, 1);

    // Recommended topics and covered topics should be populated
    QVariantList recList = vm.recommendedTopics();
    QVariantList covList = vm.coveredTopics();
    EXPECT_FALSE(recList.isEmpty());
    EXPECT_FALSE(covList.isEmpty());

    // Inspect recommended topic shape
    QVariantMap firstRec = recList.first().toMap();
    EXPECT_FALSE(firstRec["id"].toString().isEmpty());
    EXPECT_FALSE(firstRec["title"].toString().isEmpty());
    EXPECT_FALSE(firstRec["priorityText"].toString().isEmpty());
    EXPECT_FALSE(firstRec["statusText"].toString().isEmpty());
    QVariantList materials = firstRec["materials"].toList();
    EXPECT_FALSE(materials.isEmpty());
    QVariantMap firstMat = materials.first().toMap();
    EXPECT_TRUE(firstMat["url"].toString().startsWith("https://"));

    // Refine description preserves text but clears hasResult
    vm.refineDescription();
    EXPECT_FALSE(vm.hasResult());
    EXPECT_FALSE(vm.userDescription().isEmpty());

    // Reset clears everything
    vm.reset();
    EXPECT_FALSE(vm.hasResult());
    EXPECT_TRUE(vm.userDescription().isEmpty());
    EXPECT_EQ(vm.charCount(), 0);
}

TEST(TestPreparationViewModel, DiscardsStaleRequestsOnReset) {
    auto service = std::make_shared<DemoPreparationService>();
    PreparationViewModel vm(service);

    vm.setUserDescription(QStringLiteral("Valid description for testing cancellation of pending requests."));
    vm.analyze();
    EXPECT_TRUE(vm.isAnalyzing());

    // Reset while analysis is in progress
    vm.reset();
    EXPECT_FALSE(vm.isAnalyzing());
    EXPECT_FALSE(vm.hasResult());

    // Wait past the simulated 1-second delay
    QThread::msleep(1200);
    QCoreApplication::processEvents();

    // The late callback must have been ignored
    EXPECT_FALSE(vm.hasResult());
    EXPECT_TRUE(vm.userDescription().isEmpty());
}

TEST(TestPreparationViewModel, SafeAgainstViewModelDestructionDuringCallback) {
    auto service = std::make_shared<DemoPreparationService>();
    {
        auto vm = std::make_unique<PreparationViewModel>(service);
        vm->setUserDescription(QStringLiteral("Valid description for testing premature destruction safety."));
        vm->analyze();
        EXPECT_TRUE(vm->isAnalyzing());
        // vm is destroyed here while service has pending timer
    }

    // Process events until the service timer fires; should not crash
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 1300) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(20);
    }
    SUCCEED();
}
