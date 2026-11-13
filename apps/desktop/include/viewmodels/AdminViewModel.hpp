#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "services/AdminService.hpp"
#include "services/AuthService.hpp"
#include "viewmodels/QuestionListModel.hpp"
#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

// Question bank administration (port of the data side of the web
// AdminPanel.tsx): filtered and paged question list, create, delete and the
// active / inactive switch.
class AdminViewModel : public QObject, public support::BackendAware {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* questions READ questions CONSTANT)
    Q_PROPERTY(int questionCount READ questionCount NOTIFY questionsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool isSubmitting READ isSubmitting NOTIFY submittingChanged)
    Q_PROPERTY(bool isDemoData READ isDemoData NOTIFY questionsChanged)
    // "" (all tracks) or a track id such as "frontend".
    Q_PROPERTY(QString directionFilter READ directionFilter WRITE setDirectionFilter NOTIFY directionFilterChanged)
    Q_PROPERTY(int page READ page NOTIFY pageChanged)
    Q_PROPERTY(int totalPages READ totalPages NOTIFY pageChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY pageChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    // Language of the sample questions shown when the backend is unreachable.
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY uiLanguageChanged)

public:
    explicit AdminViewModel(std::shared_ptr<services::AdminService> adminService,
                            std::shared_ptr<services::AuthService> authService, QObject* parent = nullptr);

    QAbstractItemModel* questions() { return &m_questions; }
    int questionCount() const { return m_questions.count(); }
    bool loading() const { return m_loading; }
    bool isSubmitting() const { return m_isSubmitting; }
    bool isDemoData() const { return m_isDemoData; }
    QString directionFilter() const { return m_directionFilter; }
    void setDirectionFilter(const QString& direction);
    int page() const { return m_page; }
    int totalPages() const { return m_totalPages; }
    int totalCount() const { return m_totalCount; }
    QString errorMessage() const { return m_errorMessage; }
    QString uiLanguage() const { return m_uiLanguage; }
    void setUiLanguage(const QString& language);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void previousPage();

    // Adds a question; emits questionCreated() when it is in the list.
    Q_INVOKABLE void createQuestion(const QString& direction, const QString& level, const QString& topic,
                                    const QString& questionText);
    Q_INVOKABLE void deleteQuestion(const QString& id);
    Q_INVOKABLE void setQuestionActive(const QString& id, bool active);

signals:
    void questionsChanged();
    void loadingChanged();
    void submittingChanged();
    void directionFilterChanged();
    void pageChanged();
    void errorMessageChanged();
    void uiLanguageChanged();
    void questionCreated();

private:
    void applyDemoQuestions();
    void setLoading(bool value);
    void setSubmitting(bool submitting);
    void setErrorMessage(const QString& message);
    void setPaging(int page, int totalPages, int totalCount);

    std::shared_ptr<services::AdminService> m_adminService;
    std::shared_ptr<services::AuthService> m_authService;
    QuestionListModel m_questions;
    QString m_directionFilter;
    QString m_errorMessage;
    QString m_uiLanguage{QStringLiteral("en")};
    int m_page{1};
    int m_totalPages{1};
    int m_totalCount{0};
    int m_generation{0};
    bool m_loading{false};
    bool m_isSubmitting{false};
    bool m_isDemoData{false};
};

} // namespace devdialogue::viewmodels
