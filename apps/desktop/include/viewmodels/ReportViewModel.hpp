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
    // Each entry: { question, score, comment }
    Q_PROPERTY(QVariantList questionScores READ questionScores NOTIFY reportChanged)
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

signals:
    void loadingChanged();
    void reportChanged();
    void uiLanguageChanged();

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
    int m_generation{0};
    bool m_loading{false};
    bool m_hasReport{false};
    bool m_isDemoData{false};
    // A sample report of the demo history follows the interface language.
    bool m_followsUiLanguage{false};
};

} // namespace devdialogue::viewmodels
