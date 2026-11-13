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
    QString uiLanguage() const { return m_uiLanguage; }
    void setUiLanguage(const QString& language);

    // Loads the feedback of the given interview. Without a backend, or for
    // an interview that has no feedback yet, the sample report is shown.
    Q_INVOKABLE void load(const QString& interviewId);

signals:
    void loadingChanged();
    void reportChanged();
    void uiLanguageChanged();

private:
    void setLoading(bool value);
    void apply(const models::InterviewFeedbackReport& report, bool isDemo);

    std::shared_ptr<services::InterviewService> m_interviewService;
    models::InterviewFeedbackReport m_report;
    QString m_uiLanguage{QStringLiteral("en")};
    int m_generation{0};
    bool m_loading{false};
    bool m_hasReport{false};
    bool m_isDemoData{false};
};

} // namespace devdialogue::viewmodels
