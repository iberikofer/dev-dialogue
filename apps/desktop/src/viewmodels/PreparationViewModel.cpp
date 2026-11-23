#include "viewmodels/PreparationViewModel.hpp"
#include <QPointer>
#include <QVariantMap>

namespace devdialogue::viewmodels {

PreparationViewModel::PreparationViewModel(
    std::shared_ptr<services::IPreparationService> service,
    NavigationViewModel* navigation,
    QObject* parent
) : QObject(parent),
    m_service(std::move(service)),
    m_navigation(navigation) {
    updateValidation();
}

void PreparationViewModel::setDirection(const QString& dir) {
    if (m_direction != dir) {
        m_direction = dir;
        emit directionChanged();
    }
}

void PreparationViewModel::setLevel(const QString& lvl) {
    if (m_level != lvl) {
        m_level = lvl;
        emit levelChanged();
    }
}

void PreparationViewModel::setLanguage(const QString& lang) {
    if (m_language != lang) {
        m_language = lang;
        emit languageChanged();
        emit uiLanguageChanged();
        updateValidation();
    }
}

void PreparationViewModel::setUiLanguage(const QString& lang) {
    setLanguage(lang);
}

void PreparationViewModel::setUserDescription(const QString& desc) {
    if (m_userDescription != desc) {
        m_userDescription = desc;
        emit userDescriptionChanged();
        updateValidation();
    }
}

bool PreparationViewModel::isValidInput() const {
    return m_isValidInput;
}

QString PreparationViewModel::validationMessage() const {
    return m_validationMessage;
}

void PreparationViewModel::updateValidation() {
    const int len = charCount();
    const bool isUk = m_language.trimmed().toLower() != QStringLiteral("en");

    bool valid = true;
    QString msg;

    if (len == 0) {
        valid = false;
        msg = isUk ? QStringLiteral("Введіть опис ваших навичок та технологічного стеку.")
                   : QStringLiteral("Enter a description of your skills and tech stack.");
    } else if (len < MinDescriptionLength) {
        valid = false;
        msg = isUk ? QStringLiteral("Опишіть детальніше (ще мінімум %1 символів).").arg(MinDescriptionLength - len)
                   : QStringLiteral("Please describe in more detail (at least %1 more characters).").arg(MinDescriptionLength - len);
    } else if (len > MaxDescriptionLength) {
        valid = false;
        msg = isUk ? QStringLiteral("Перевищено ліміт на %1 символів (максимум %2).").arg(len - MaxDescriptionLength).arg(MaxDescriptionLength)
                   : QStringLiteral("Exceeded limit by %1 characters (max %2).").arg(len - MaxDescriptionLength).arg(MaxDescriptionLength);
    }

    if (m_isValidInput != valid || m_validationMessage != msg) {
        m_isValidInput = valid;
        m_validationMessage = msg;
        emit validationChanged();
    }
}

void PreparationViewModel::setAnalyzing(bool analyzing) {
    if (m_isAnalyzing != analyzing) {
        m_isAnalyzing = analyzing;
        emit analyzingChanged();
    }
}

void PreparationViewModel::setErrorMessage(const QString& msg) {
    if (m_errorMessage != msg) {
        m_errorMessage = msg;
        emit errorMessageChanged();
    }
}

void PreparationViewModel::analyze() {
    if (!m_isValidInput || m_isAnalyzing || !m_service) {
        return;
    }

    setAnalyzing(true);
    setErrorMessage(QString());

    const uint64_t reqId = ++m_currentRequestId;
    QPointer<PreparationViewModel> guard(this);

    m_service->analyzeProfile(m_direction, m_level, m_userDescription, m_language,
        [guard, reqId](const models::PreparationPlanResult& result, const QString& error) {
            if (!guard) {
                return; // ViewModel was deleted
            }
            if (guard->m_currentRequestId != reqId) {
                return; // Outdated / superseded analysis request
            }
            guard->onAnalysisCompleted(result, error);
        });
}

void PreparationViewModel::onAnalysisCompleted(const models::PreparationPlanResult& result, const QString& error) {
    setAnalyzing(false);

    if (!error.isEmpty()) {
        setErrorMessage(error);
        m_hasResult = false;
        emit resultChanged();
        return;
    }

    m_summaryText = result.summary;
    m_totalTopicsCount = result.totalTopics;
    m_coveredTopicsCount = result.coveredTopics;

    QVariantList recList;
    QVariantList covList;

    const bool isUk = m_language.trimmed().toLower() != QStringLiteral("en");

    for (const auto& topic : result.topics) {
        QVariantMap map;
        map[QStringLiteral("id")] = topic.id;
        map[QStringLiteral("title")] = topic.title;
        map[QStringLiteral("category")] = topic.category;
        map[QStringLiteral("reason")] = topic.reason;

        QString priorityStr;
        QString priorityText;
        switch (topic.priority) {
        case models::TopicPriority::High:
            priorityStr = QStringLiteral("high");
            priorityText = isUk ? QStringLiteral("Високий пріоритет") : QStringLiteral("High priority");
            break;
        case models::TopicPriority::Medium:
            priorityStr = QStringLiteral("medium");
            priorityText = isUk ? QStringLiteral("Середній пріоритет") : QStringLiteral("Medium priority");
            break;
        case models::TopicPriority::Low:
            priorityStr = QStringLiteral("low");
            priorityText = isUk ? QStringLiteral("Базовий пріоритет") : QStringLiteral("Low priority");
            break;
        }
        map[QStringLiteral("priority")] = priorityStr;
        map[QStringLiteral("priorityText")] = priorityText;

        QString statusStr;
        QString statusText;
        switch (topic.status) {
        case models::PreparationTopicStatus::Known:
            statusStr = QStringLiteral("known");
            statusText = isUk ? QStringLiteral("Уже покрито") : QStringLiteral("Already covered");
            break;
        case models::PreparationTopicStatus::Unclear:
            statusStr = QStringLiteral("unclear");
            statusText = isUk ? QStringLiteral("Потребує повторення") : QStringLiteral("Needs review");
            break;
        case models::PreparationTopicStatus::NotMentioned:
            statusStr = QStringLiteral("not_mentioned");
            statusText = isUk ? QStringLiteral("У вашому описі це не згадано") : QStringLiteral("Not mentioned in your description");
            break;
        }
        map[QStringLiteral("status")] = statusStr;
        map[QStringLiteral("statusText")] = statusText;

        QVariantList matList;
        for (const auto& mat : topic.materials) {
            QVariantMap matMap;
            matMap[QStringLiteral("title")] = mat.title;
            matMap[QStringLiteral("url")] = mat.url;
            matList.append(matMap);
        }
        map[QStringLiteral("materials")] = matList;

        if (topic.status == models::PreparationTopicStatus::Known) {
            covList.append(map);
        } else {
            recList.append(map);
        }
    }

    m_recommendedTopics = recList;
    m_coveredTopics = covList;
    m_hasResult = true;

    emit resultChanged();
}

void PreparationViewModel::refineDescription() {
    m_hasResult = false;
    setErrorMessage(QString());
    emit resultChanged();
}

void PreparationViewModel::reset() {
    m_currentRequestId++;
    if (m_isAnalyzing) {
        m_isAnalyzing = false;
        emit analyzingChanged();
    }
    m_hasResult = false;
    m_userDescription.clear();
    setErrorMessage(QString());
    m_recommendedTopics.clear();
    m_coveredTopics.clear();
    m_summaryText.clear();
    m_totalTopicsCount = 0;
    m_coveredTopicsCount = 0;
    emit userDescriptionChanged();
    updateValidation();
    emit resultChanged();
}

} // namespace devdialogue::viewmodels
