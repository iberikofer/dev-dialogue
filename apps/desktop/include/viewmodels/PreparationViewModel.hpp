#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <memory>
#include "services/IPreparationService.hpp"
#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

class PreparationViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QString level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY uiLanguageChanged)
    Q_PROPERTY(QString userDescription READ userDescription WRITE setUserDescription NOTIFY userDescriptionChanged)
    Q_PROPERTY(int charCount READ charCount NOTIFY userDescriptionChanged)
    Q_PROPERTY(int minCharCount READ minCharCount CONSTANT)
    Q_PROPERTY(int maxCharCount READ maxCharCount CONSTANT)
    Q_PROPERTY(bool isValidInput READ isValidInput NOTIFY validationChanged)
    Q_PROPERTY(QString validationMessage READ validationMessage NOTIFY validationChanged)
    Q_PROPERTY(bool isAnalyzing READ isAnalyzing NOTIFY analyzingChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString summaryText READ summaryText NOTIFY resultChanged)
    Q_PROPERTY(int totalTopicsCount READ totalTopicsCount NOTIFY resultChanged)
    Q_PROPERTY(int coveredTopicsCount READ coveredTopicsCount NOTIFY resultChanged)
    Q_PROPERTY(QVariantList recommendedTopics READ recommendedTopics NOTIFY resultChanged)
    Q_PROPERTY(QVariantList coveredTopics READ coveredTopics NOTIFY resultChanged)

public:
    static constexpr int MinDescriptionLength = 30;
    static constexpr int MaxDescriptionLength = 3000;

    explicit PreparationViewModel(
        std::shared_ptr<services::IPreparationService> service,
        NavigationViewModel* navigation = nullptr,
        QObject* parent = nullptr
    );
    ~PreparationViewModel() override = default;

    QString direction() const { return m_direction; }
    void setDirection(const QString& dir);

    QString level() const { return m_level; }
    void setLevel(const QString& lvl);

    QString language() const { return m_language; }
    void setLanguage(const QString& lang);

    QString uiLanguage() const { return language(); }
    void setUiLanguage(const QString& lang);

    QString userDescription() const { return m_userDescription; }
    void setUserDescription(const QString& desc);

    int charCount() const { return static_cast<int>(m_userDescription.trimmed().length()); }
    int minCharCount() const { return MinDescriptionLength; }
    int maxCharCount() const { return MaxDescriptionLength; }

    bool isValidInput() const;
    QString validationMessage() const;

    bool isAnalyzing() const { return m_isAnalyzing; }
    bool hasResult() const { return m_hasResult; }
    QString errorMessage() const { return m_errorMessage; }
    QString summaryText() const { return m_summaryText; }
    int totalTopicsCount() const { return m_totalTopicsCount; }
    int coveredTopicsCount() const { return m_coveredTopicsCount; }

    QVariantList recommendedTopics() const { return m_recommendedTopics; }
    QVariantList coveredTopics() const { return m_coveredTopics; }

    Q_INVOKABLE void analyze();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void refineDescription();

signals:
    void directionChanged();
    void levelChanged();
    void languageChanged();
    void uiLanguageChanged();
    void userDescriptionChanged();
    void validationChanged();
    void analyzingChanged();
    void resultChanged();
    void errorMessageChanged();

private:
    void updateValidation();
    void setAnalyzing(bool analyzing);
    void setErrorMessage(const QString& msg);
    void onAnalysisCompleted(const models::PreparationPlanResult& result, const QString& error);

    std::shared_ptr<services::IPreparationService> m_service;
    NavigationViewModel* m_navigation{nullptr};

    QString m_direction{QStringLiteral("frontend")};
    QString m_level{QStringLiteral("junior")};
    QString m_language{QStringLiteral("uk")};
    QString m_userDescription;

    bool m_isValidInput{false};
    QString m_validationMessage;

    bool m_isAnalyzing{false};
    bool m_hasResult{false};
    QString m_errorMessage;
    QString m_summaryText;
    int m_totalTopicsCount{0};
    int m_coveredTopicsCount{0};

    QVariantList m_recommendedTopics;
    QVariantList m_coveredTopics;

    uint64_t m_currentRequestId{0};
};

} // namespace devdialogue::viewmodels
