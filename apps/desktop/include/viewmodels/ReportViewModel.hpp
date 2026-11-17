#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <memory>

#include "services/InterviewService.hpp"
#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

// Performance report of a finished interview (port of the data side of the
// web PerformanceReport.tsx).
class ReportViewModel : public QObject, public support::BackendAware {
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasReport READ hasReport NOTIFY reportChanged)
    Q_PROPERTY(bool isDemoData READ isDemoData NOTIFY reportChanged)
    Q_PROPERTY(int score READ score NOTIFY reportChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY reportChanged)
    Q_PROPERTY(QStringList strengths READ strengths NOTIFY reportChanged)
    Q_PROPERTY(QStringList weaknesses READ weaknesses NOTIFY reportChanged)
    // Each entry: { topic, resource }
    Q_PROPERTY(QVariantList recommendations READ recommendations NOTIFY reportChanged)
    // Each entry: { question, score, comment, responseSeconds, userAnswer, accuracy, completeness, terminology, clarity, hasCriteria }
    Q_PROPERTY(QVariantList questionScores READ questionScores NOTIFY reportChanged)
    Q_PROPERTY(int averageResponseSeconds READ averageResponseSeconds NOTIFY reportChanged)
    Q_PROPERTY(int totalDurationSeconds READ totalDurationSeconds NOTIFY reportChanged)
    Q_PROPERTY(QString timingAnalysis READ timingAnalysis NOTIFY reportChanged)
    Q_PROPERTY(bool hasVisualAssessment READ hasVisualAssessment NOTIFY reportChanged)
    Q_PROPERTY(int visualScore READ visualScore NOTIFY reportChanged)
    Q_PROPERTY(int visualPresence READ visualPresence NOTIFY reportChanged)
    Q_PROPERTY(int visualHands READ visualHands NOTIFY reportChanged)
    Q_PROPERTY(int visualLighting READ visualLighting NOTIFY reportChanged)
    Q_PROPERTY(int visualAppearance READ visualAppearance NOTIFY reportChanged)
    Q_PROPERTY(int visualPosture READ visualPosture NOTIFY reportChanged)
    Q_PROPERTY(int visualEmotionScore READ visualEmotionScore NOTIFY reportChanged)
    Q_PROPERTY(QString visualDominantEmotion READ visualDominantEmotion NOTIFY reportChanged)
    Q_PROPERTY(bool visualIncludedInScore READ visualIncludedInScore NOTIFY reportChanged)
    Q_PROPERTY(QString visualSummary READ visualSummary NOTIFY reportChanged)
    Q_PROPERTY(QString visualModel READ visualModel NOTIFY reportChanged)
    Q_PROPERTY(int visualSamples READ visualSamples NOTIFY reportChanged)
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY exportChanged)
    Q_PROPERTY(QString exportError READ exportError NOTIFY exportChanged)
    // Which interview the report is about: "frontend", "junior", "16.11.2026  10:49".
    Q_PROPERTY(QString direction READ direction NOTIFY reportChanged)
    Q_PROPERTY(QString level READ level NOTIFY reportChanged)
    Q_PROPERTY(QString dateText READ dateText NOTIFY reportChanged)
    // Why the report of a server interview could not be loaded; empty otherwise.
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY reportChanged)
    // Language of the sample report shown when the backend is unreachable.
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY uiLanguageChanged)

public:
    explicit ReportViewModel(std::shared_ptr<services::InterviewService> interviewService,
                             QObject* parent = nullptr);

    bool loading() const { return m_loading; }
    bool hasReport() const { return m_hasReport; }
    bool isDemoData() const { return m_isDemoData; }
    int score() const { return m_report.score; }
    QString summary() const { return QString::fromStdString(m_report.summary); }
    QStringList strengths() const;
    QStringList weaknesses() const;
    QVariantList recommendations() const;
    QVariantList questionScores() const;
    int averageResponseSeconds() const { return m_report.averageResponseSeconds; }
    int totalDurationSeconds() const { return m_report.totalDurationSeconds; }
    QString timingAnalysis() const { return QString::fromStdString(m_report.timingAnalysis); }
    bool hasVisualAssessment() const { return m_report.visualAssessment.has_value(); }
    int visualScore() const { return m_report.visualAssessment ? m_report.visualAssessment->overallScore : 0; }
    int visualPresence() const { return m_report.visualAssessment ? m_report.visualAssessment->presenceScore : 0; }
    int visualHands() const { return m_report.visualAssessment ? m_report.visualAssessment->handsScore : 0; }
    int visualLighting() const { return m_report.visualAssessment ? m_report.visualAssessment->lightingScore : 0; }
    int visualAppearance() const { return m_report.visualAssessment ? m_report.visualAssessment->appearanceScore : 0; }
    int visualPosture() const { return m_report.visualAssessment ? m_report.visualAssessment->postureScore : 0; }
    int visualEmotionScore() const { return m_report.visualAssessment ? m_report.visualAssessment->emotionScore : 0; }
    QString visualDominantEmotion() const { return m_report.visualAssessment ? QString::fromStdString(m_report.visualAssessment->dominantEmotion) : QStringLiteral("neutral"); }
    bool visualIncludedInScore() const { return m_report.visualAssessment ? m_report.visualAssessment->includedInScore : false; }
    QString visualSummary() const { return m_report.visualAssessment ? QString::fromStdString(m_report.visualAssessment->summary) : QString(); }
    QString visualModel() const { return m_report.visualAssessment ? QString::fromStdString(m_report.visualAssessment->model) : QString(); }
    int visualSamples() const { return m_report.visualAssessment ? m_report.visualAssessment->samplesAnalyzed : 0; }
    QString lastExportPath() const { return m_lastExportPath; }
    QString exportError() const { return m_exportError; }
    QString direction() const { return m_direction; }
    QString level() const { return m_level; }
    QString dateText() const { return m_dateText; }
    QString errorMessage() const { return m_errorMessage; }
    QString uiLanguage() const { return m_uiLanguage; }
    void setUiLanguage(const QString& language);

    // Loads the feedback of the given interview. Demo and offline interviews
    // come from the demo history; an interview the server has not evaluated
    // yet is scored from its transcript. A server interview that cannot be
    // fetched sets errorMessage instead of showing a report.
    Q_INVOKABLE void load(const QString& interviewId);
    Q_INVOKABLE QString exportPdf(const QString& targetPath = QString());
    Q_INVOKABLE void openLastPdf() const;

signals:
    void loadingChanged();
    void reportChanged();
    void uiLanguageChanged();
    void exportChanged();
    void pdfExported(const QString& filePath);
    void pdfExportFailed(const QString& error);

private:
    void setLoading(bool value);
    void apply(const models::InterviewFeedbackReport& report, bool isDemo);
    void applyDemo(const QString& interviewId);
    void fail(const QString& message);
    void setContext(const models::InterviewSummary& interview);

    std::shared_ptr<services::InterviewService> m_interviewService;
    models::InterviewFeedbackReport m_report;
    QString m_uiLanguage{QStringLiteral("en")};
    QString m_interviewId;
    QString m_direction;
    QString m_level;
    QString m_dateText;
    QString m_errorMessage;
    QString m_lastExportPath;
    QString m_exportError;
    int m_generation{0};
    bool m_loading{false};
    bool m_hasReport{false};
    bool m_isDemoData{false};
    // A sample report of the demo history follows the interface language.
    bool m_followsUiLanguage{false};
};

} // namespace devdialogue::viewmodels
