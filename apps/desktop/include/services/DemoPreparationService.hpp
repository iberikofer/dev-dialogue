#pragma once

#include <QObject>
#include <QStringList>
#include "services/IPreparationService.hpp"

namespace devdialogue::services {

class DemoPreparationService : public QObject, public IPreparationService {
    Q_OBJECT

public:
    explicit DemoPreparationService(QObject* parent = nullptr);
    ~DemoPreparationService() override = default;

    void analyzeProfile(
        const QString& direction,
        const QString& level,
        const QString& userDescription,
        const QString& language,
        std::function<void(const models::PreparationPlanResult& result, const QString& error)> callback
    ) override;

    // Helper for direct deterministic synchronous evaluation (useful for unit tests without waiting)
    models::PreparationPlanResult evaluateSynchronously(
        const QString& direction,
        const QString& level,
        const QString& userDescription,
        const QString& language
    ) const;

private:
    struct TopicDefinition {
        QString id;
        QString titleEn;
        QString titleUk;
        QString categoryEn;
        QString categoryUk;
        models::TopicPriority basePriority;
        QStringList keywords;
        QVector<models::StudyMaterial> materialsEn;
        QVector<models::StudyMaterial> materialsUk;
        bool isAdvanced;
        bool isFundamental;
    };

    QVector<TopicDefinition> getDefinitionsForDirection(const QString& direction) const;
    models::PreparationTopicStatus evaluateStatus(
        const QString& descriptionLower,
        const QStringList& keywords,
        QString& outReason,
        bool isUk
    ) const;

    models::TopicPriority adjustPriority(
        models::TopicPriority base,
        bool isFundamental,
        bool isAdvanced,
        const QString& level
    ) const;
};

} // namespace devdialogue::services
