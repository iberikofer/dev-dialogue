#pragma once

#include <QString>
#include <QVector>

namespace devdialogue::models {

enum class PreparationTopicStatus {
    Known,
    NotMentioned,
    Unclear
};

enum class TopicPriority {
    High,
    Medium,
    Low
};

struct StudyMaterial {
    QString title;
    QString url;
};

struct PreparationTopic {
    QString id;
    QString title;
    QString category;
    PreparationTopicStatus status{PreparationTopicStatus::NotMentioned};
    QString reason;
    TopicPriority priority{TopicPriority::Medium};
    QVector<StudyMaterial> materials;
};

struct PreparationPlanResult {
    QString summary;
    QVector<PreparationTopic> topics;
    int totalTopics{0};
    int coveredTopics{0};
};

} // namespace devdialogue::models
