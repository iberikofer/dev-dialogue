#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "services/InterviewService.hpp"
#include "viewmodels/NavigationViewModel.hpp"

namespace devdialogue::viewmodels {

// Interview configuration screen (port of the web InterviewSetup.tsx):
// holds the chosen track, level and language and starts the session.
class SetupViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QString level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit SetupViewModel(std::shared_ptr<services::InterviewService> interviewService,
                            NavigationViewModel* navigation, QObject* parent = nullptr);

    QString direction() const { return m_direction; }
    void setDirection(const QString& direction);
    QString level() const { return m_level; }
    void setLevel(const QString& level);
    QString language() const { return m_language; }
    void setLanguage(const QString& language);
    bool isBusy() const { return m_isBusy; }
    QString errorMessage() const { return m_errorMessage; }

    // Creates the interview and opens the chat room. If the backend is
    // unreachable a local demo session is started instead, like on the web.
    Q_INVOKABLE void start();

signals:
    void directionChanged();
    void levelChanged();
    void languageChanged();
    void busyChanged();
    void errorMessageChanged();
    // Emitted right before navigating to the chat room. `firstQuestion` is
    // empty and `offline` is true for a local demo session.
    void interviewStarted(const QString& interviewId, const QString& direction, const QString& level,
                          const QString& language, const QString& firstQuestion, bool offline);

private:
    void setBusy(bool busy);
    void setErrorMessage(const QString& message);
    void launch(const QString& interviewId, const QString& firstQuestion, bool offline);

    std::shared_ptr<services::InterviewService> m_interviewService;
    NavigationViewModel* m_navigation;
    QString m_direction{QStringLiteral("frontend")};
    QString m_level{QStringLiteral("junior")};
    QString m_language{QStringLiteral("uk")};
    QString m_errorMessage;
    bool m_isBusy{false};
};

} // namespace devdialogue::viewmodels
