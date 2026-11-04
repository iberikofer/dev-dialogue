# Document 05: Offline Mock Engine & Internationalization

This document details the C++ port of the offline AI simulation service (`apps/api/Services/AI/MockAiService.cs`) and the internationalization architecture replacing `apps/web/src/lib/translations.ts` with **Qt Linguist** (`QTranslator`).

---

## 1. Embedded C++ Offline Mock AI Engine (`MockAiEngine`)

The original web application falls back to basic canned responses when disconnected, while the .NET backend contains a structured `MockAiService.cs`. In the C++ desktop application, this logic is ported to native C++ (`MockAiEngine`), enabling **100% offline air-gapped operation** with zero external network dependencies.

### Architecture: `include/services/MockAiEngine.hpp`

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "models/ApiContracts.hpp"

namespace devdialogue::services {

class MockAiEngine {
public:
    MockAiEngine();

    // Core Generation APIs
    std::string generateFirstQuestion(
        models::InterviewDirection direction,
        models::InterviewLevel level,
        models::InterviewLanguage language
    );

    std::string generateNextQuestion(
        models::InterviewDirection direction,
        models::InterviewLevel level,
        models::InterviewLanguage language,
        int currentQuestionCount,
        const std::string& candidateAnswer
    );

    models::InterviewFeedbackReport evaluateInterview(
        models::InterviewDirection direction,
        models::InterviewLevel level,
        models::InterviewLanguage language,
        const std::vector<models::InterviewMessage>& transcript
    );

private:
    struct QuestionPool {
        std::vector<std::string> questionsUk;
        std::vector<std::string> questionsEn;
    };

    // [Direction][Level] -> QuestionPool
    std::unordered_map<int, std::unordered_map<int, QuestionPool>> m_questionBank;

    void initializeQuestionBank();
    int calculateAnswerScore(const std::string& answer);
};

} // namespace devdialogue::services
```

### Keyword Analysis & Scorecard Generation: `src/services/MockAiEngine.cpp`

```cpp
#include "services/MockAiEngine.hpp"
#include <algorithm>
#include <sstream>

namespace devdialogue::services {

MockAiEngine::MockAiEngine() {
    initializeQuestionBank();
}

int MockAiEngine::calculateAnswerScore(const std::string& answer) {
    if (answer.length() < 20) return 45; // Terse / trivial answer
    if (answer.length() < 80) return 65; // Moderate answer

    int score = 75;
    // Reward depth and technical markers
    if (answer.find("because") != std::string::npos || answer.find("тому що") != std::string::npos) score += 5;
    if (answer.find("for example") != std::string::npos || answer.find("наприклад") != std::string::npos) score += 5;
    if (answer.length() > 200) score += 10;

    return std::min(score, 98);
}

models::InterviewFeedbackReport MockAiEngine::evaluateInterview(
    models::InterviewDirection direction,
    models::InterviewLevel /*level*/,
    models::InterviewLanguage language,
    const std::vector<models::InterviewMessage>& transcript
) {
    models::InterviewFeedbackReport report;
    std::vector<models::QuestionScoreItem> qScores;
    int totalScore = 0;
    int questionIndex = 1;

    for (size_t i = 1; i < transcript.size(); i += 2) {
        if (i + 1 < transcript.size() || transcript[i].role == "user") {
            const auto& questionMsg = transcript[i - 1];
            const auto& answerMsg = transcript[i];

            int itemScore = calculateAnswerScore(answerMsg.content);
            totalScore += itemScore;

            models::QuestionScoreItem item;
            item.question = questionMsg.content;
            item.score = itemScore;
            item.comment = (language == models::InterviewLanguage::Uk)
                ? "Відповідь логічна, продемонстровано базове володіння концепціями."
                : "Structured explanation with clear grasp of underlying fundamentals.";
            qScores.push_back(item);
            questionIndex++;
        }
    }

    int finalScore = qScores.empty() ? 78 : (totalScore / static_cast<int>(qScores.size()));
    report.score = finalScore;
    report.questionScores = qScores;

    if (language == models::InterviewLanguage::Uk) {
        report.summary = "Кандидат продемонстрував стійкі знання архітектурних концепцій та стек-специфічних технологій.";
        report.strengths = {
            "Впевнене володіння термінологією та базовими патернами",
            "Чітке розуміння життєвого циклу запитів та асинхронності",
            "Логічне формулювання технічних компромісів"
        };
        report.weaknesses = {
            "Варто глибше дослідити оптимізацію пам'яті та профілювання",
            "Звернути увагу на крайові сценарії обробки відмов"
        };
        report.recommendations = {
            {"System Design Primer", "https://github.com/donnemartin/system-design-primer"},
            {"Database Indexing Guide", "https://use-the-index-luke.com"}
        };
    } else {
        report.summary = "The candidate demonstrated solid command of architectural patterns and core technologies.";
        report.strengths = {
            "Confident grasp of core language semantics and design patterns",
            "Clear understanding of request lifecycles and concurrency",
            "Well-reasoned articulation of architectural trade-offs"
        };
        report.weaknesses = {
            "Further exploration of profiling and memory footprint optimization is advised",
            "Pay closer attention to edge cases in distributed fault scenarios"
        };
        report.recommendations = {
            {"System Design Primer", "https://github.com/donnemartin/system-design-primer"},
            {"Use The Index, Luke", "https://use-the-index-luke.com"}
        };
    }

    return report;
}

void MockAiEngine::initializeQuestionBank() {
    // Frontend Track - Junior
    auto& feJunior = m_questionBank[static_cast<int>(models::InterviewDirection::Frontend)][static_cast<int>(models::InterviewLevel::Junior)];
    feJunior.questionsUk = {
        "Вітаю на співбесіді Frontend (Junior)! Розкажіть коротко про себе та ваш досвід із JavaScript/TypeScript.",
        "Як працює Virtual DOM у React і в чому його перевага над прямими маніпуляціями з DOM?",
        "Поясніть різницю між useMemo та useCallback. У яких випадках їхнє використання є надлишковим?",
        "Які способи оптимізації рендерингу великих списків у браузері ви знаєте?",
        "Чудово! Усі питання цієї сесії вичерпано. Натисніть кнопку завершення для скоркарту."
    };
    feJunior.questionsEn = {
        "Welcome to the Frontend (Junior) interview! Please briefly introduce yourself and your JavaScript/TypeScript experience.",
        "How does the Virtual DOM work in React, and what benefits does it provide over direct DOM mutations?",
        "Explain the distinction between useMemo and useCallback. When does memoization add unnecessary overhead?",
        "What techniques do you apply to optimize rendering of large data lists in modern web browsers?",
        "Thank you! All questions for this session are complete. Click finish to inspect your detailed scorecard."
    };

    // Backend, DevOps, QA, PM question banks populated identically...
}

} // namespace devdialogue::services
```

---

## 2. Dynamic Internationalization (`QTranslator` & I18nManager)

In Next.js, translations reside in a static TypeScript dictionary (`translations.ts`) managed by Zustand. In the C++ desktop app, translations are compiled into high-performance binary string tables (`.qm` files) managed by `QTranslator`:

### Workflow
1. Mark all QML UI strings with `qsTr("...")` or `qsTranslate()`.
2. Extract strings to XML format:
   ```bash
   lupdate apps/desktop/qml -ts resources/i18n/devdialogue_en.ts resources/i18n/devdialogue_uk.ts
   ```
3. Compile XML catalogs into binary files:
   ```bash
   lrelease resources/i18n/devdialogue_*.ts
   ```

### Dynamic Runtime Language Switching (Zero-Restart)

```cpp
void I18nManager::setLanguage(const QString& langCode) {
    if (m_currentLanguage == langCode) return;

    QGuiApplication::removeTranslator(&m_translator);

    const QString qmPath = QString(":/resources/i18n/devdialogue_%1.qm").arg(langCode);
    if (m_translator.load(qmPath)) {
        QGuiApplication::installTranslator(&m_translator);
        m_currentLanguage = langCode;
        
        // Notify QML engine to re-evaluate all qsTr() bindings dynamically
        m_qmlEngine->retranslate();
        emit languageChanged(m_currentLanguage);
    }
}
```

This ensures instantaneous, seamless switching between Ukrainian and English across every open screen without reloading or losing the user's active interview state.

