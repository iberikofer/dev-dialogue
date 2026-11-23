#pragma once

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <optional>

#include "services/InterviewService.hpp"
#include "viewmodels/MessageListModel.hpp"
#include "viewmodels/NavigationViewModel.hpp"
#include "viewmodels/ViewModelSupport.hpp"

namespace devdialogue::viewmodels {

// Interview room controller (port of the web ChatRoom.tsx): the transcript,
// sending answers, the session clock and finishing the interview.
class ChatViewModel : public QObject, public support::BackendAware {
    Q_OBJECT
    Q_PROPERTY(QString interviewId READ interviewId NOTIFY sessionChanged)
    Q_PROPERTY(QString currentDirection READ currentDirection NOTIFY sessionChanged)
    Q_PROPERTY(QString currentLevel READ currentLevel NOTIFY sessionChanged)
    Q_PROPERTY(QString sessionLanguage READ sessionLanguage NOTIFY sessionChanged)
    Q_PROPERTY(QAbstractItemModel* messagesModel READ messagesModel CONSTANT)
    Q_PROPERTY(int candidateAnswersCount READ candidateAnswersCount NOTIFY messagesChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool isCompleting READ isCompleting NOTIFY completingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString elapsedText READ elapsedText NOTIFY elapsedChanged)
    // True for a session that runs locally without the backend.
    Q_PROPERTY(bool isLocalSession READ isLocalSession NOTIFY sessionChanged)
    // A session is open and not finished yet (drives the sidebar entry).
    Q_PROPERTY(bool hasActiveSession READ hasActiveSession NOTIFY sessionChanged)
    // True after the interviewer scores an answer and offers follow-up choices instead of free text.
    Q_PROPERTY(bool awaitingNextAction READ awaitingNextAction NOTIFY actionStateChanged)
    Q_PROPERTY(QStringList suggestedActions READ suggestedActions NOTIFY actionStateChanged)

    // 3D Avatar state & controls (interview room + Debug wardrobe/voice panel).
    Q_PROPERTY(QString avatarEmotion READ avatarEmotion NOTIFY avatarEmotionChanged)
    Q_PROPERTY(qreal avatarVolume READ avatarVolume WRITE setAvatarVolume NOTIFY avatarVolumeChanged)
    Q_PROPERTY(bool avatarMuted READ avatarMuted WRITE setAvatarMuted NOTIFY avatarMutedChanged)
    Q_PROPERTY(QString avatarVoice READ avatarVoice NOTIFY avatarVoiceChanged)
    Q_PROPERTY(QString avatarOutputMode READ avatarOutputMode NOTIFY avatarOutputModeChanged)
    Q_PROPERTY(QString avatarSoundEffect READ avatarSoundEffect NOTIFY avatarSoundEffectChanged)
    Q_PROPERTY(QString avatarGender READ avatarGender NOTIFY avatarGenderChanged)
    Q_PROPERTY(QString avatarPreset READ avatarPreset NOTIFY avatarPresetChanged)
    Q_PROPERTY(bool hatVisible READ hatVisible NOTIFY avatarWardrobeChanged)
    Q_PROPERTY(bool glassesVisible READ glassesVisible NOTIFY avatarWardrobeChanged)
    Q_PROPERTY(bool tieVisible READ tieVisible NOTIFY avatarWardrobeChanged)
    Q_PROPERTY(bool hornsVisible READ hornsVisible NOTIFY avatarWardrobeChanged)
    Q_PROPERTY(bool haloVisible READ haloVisible NOTIFY avatarWardrobeChanged)
    Q_PROPERTY(QVariantList availableVoices READ availableVoices CONSTANT)
    Q_PROPERTY(QVariantList availablePresets READ availablePresets CONSTANT)
    Q_PROPERTY(QVariantList availableSoundEffects READ availableSoundEffects CONSTANT)

public:
    explicit ChatViewModel(std::shared_ptr<services::InterviewService> interviewService,
                           NavigationViewModel* navigation, QObject* parent = nullptr);

    QString interviewId() const { return m_interviewId; }
    QString currentDirection() const { return m_direction; }
    QString currentLevel() const { return m_level; }
    QString sessionLanguage() const { return m_language; }
    QAbstractItemModel* messagesModel() { return &m_messages; }
    int candidateAnswersCount() const { return m_answeredCount; }
    bool isBusy() const { return m_isBusy; }
    bool isCompleting() const { return m_isCompleting; }
    QString errorMessage() const { return m_errorMessage; }
    // Session time as "mm:ss".
    QString elapsedText() const;
    bool isLocalSession() const { return m_offline; }
    bool hasActiveSession() const { return !m_interviewId.isEmpty() && !m_completed; }
    bool awaitingNextAction() const { return m_awaitingNextAction; }
    QStringList suggestedActions() const { return m_suggestedActions; }

    QString avatarEmotion() const { return m_avatarEmotion; }
    qreal avatarVolume() const { return m_avatarVolume; }
    bool avatarMuted() const { return m_avatarMuted; }
    QString avatarVoice() const { return m_avatarVoice; }
    QString avatarOutputMode() const { return m_avatarOutputMode; }
    QString avatarSoundEffect() const { return m_avatarSoundEffect; }
    QString avatarGender() const { return m_avatarGender; }
    QString avatarPreset() const { return m_avatarPreset; }
    bool hatVisible() const { return m_hatVisible; }
    bool glassesVisible() const { return m_glassesVisible; }
    bool tieVisible() const { return m_tieVisible; }
    bool hornsVisible() const { return m_hornsVisible; }
    bool haloVisible() const { return m_haloVisible; }
    QVariantList availableVoices() const;
    QVariantList availablePresets() const;
    QVariantList availableSoundEffects() const;

    Q_INVOKABLE void setAvatarVolume(qreal volume);
    Q_INVOKABLE void setAvatarMuted(bool muted);
    Q_INVOKABLE void toggleAvatarMute();
    Q_INVOKABLE void stopAvatarSpeech();
    Q_INVOKABLE void setAvatarVoice(const QString& voiceId);
    Q_INVOKABLE void setAvatarOutputMode(const QString& mode);
    Q_INVOKABLE void toggleAvatarOutputMode();
    Q_INVOKABLE void setAvatarSoundEffect(const QString& soundFile);
    Q_INVOKABLE void cycleAvatarSoundEffect();
    Q_INVOKABLE void setAvatarGender(const QString& gender);
    Q_INVOKABLE void setAvatarPreset(const QString& presetId);
    Q_INVOKABLE void setAccessoryVisible(const QString& name, bool visible);
    Q_INVOKABLE void toggleAccessory(const QString& name);
    Q_INVOKABLE void triggerTestEmotion(const QString& emotion);

    // Posts the candidate's answer and appends the interviewer's reply. If
    // the backend is unreachable a scripted demo reply is used instead.
    Q_INVOKABLE void sendAnswer(const QString& text);

    // Executes one of the post-evaluation actions (explain_reference, explain_answer, next_question).
    Q_INVOKABLE void triggerFollowUpAction(const QString& action);
    Q_INVOKABLE bool hasSuggestedAction(const QString& action) const { return m_suggestedActions.contains(action); }

    // Finishes the interview and opens the report.
    Q_INVOKABLE void completeInterview();

    // Resumes an existing interview by loading its transcript.
    Q_INVOKABLE void openSession(const QString& interviewId);

    // Milliseconds before a scripted demo reply appears (tests shorten it).
    void setFallbackReplyDelay(int milliseconds) { m_fallbackReplyDelayMs = milliseconds; }

    using VisualAssessmentProbe = std::function<std::optional<models::VisualAssessment>()>;
    void setVisualAssessmentProbe(VisualAssessmentProbe probe) { m_visualAssessmentProbe = std::move(probe); }

    using VisualContextProbe = std::function<std::optional<models::VisualContext>()>;
    void setVisualContextProbe(VisualContextProbe probe) { m_visualContextProbe = std::move(probe); }

    void setEnsureServicesProbe(std::function<void()> probe) { m_ensureServicesProbe = std::move(probe); }
    void setLocalModelReadyProbe(std::function<bool()> probe) { m_localModelReadyProbe = std::move(probe); }

public slots:
    // Starts a freshly created interview (wired to SetupViewModel::interviewStarted).
    void beginSession(const QString& interviewId, const QString& direction, const QString& level,
                      const QString& language, const QString& firstQuestion, bool offline);

signals:
    void sessionChanged();
    void messagesChanged();
    void busyChanged();
    void completingChanged();
    void errorMessageChanged();
    void elapsedChanged();
    void actionStateChanged();
    void interviewCompleted(const QString& interviewId);
    void localModelRequired();
    void avatarEmotionChanged();
    void avatarVolumeChanged();
    void avatarMutedChanged();
    void avatarVoiceChanged();
    void avatarOutputModeChanged();
    void avatarSoundEffectChanged();
    void avatarGenderChanged();
    void avatarPresetChanged();
    void avatarWardrobeChanged();
    void avatarSpeakRequested(const QString& text, const QString& emotion,
                              const QStringList& actionsSequence, const QJsonArray& uiCommands);
    void avatarReactRequested(const QString& emotion,
                              const QStringList& actionsSequence, const QJsonArray& uiCommands);
    void avatarSpeechStopRequested();

private:
    void applyAvatarResponse(const QString& textToSpeak, const QString& emotion,
                             const std::vector<std::string>& actionsSequence,
                             const std::vector<models::AvatarUiCommand>& uiCommands);
    void syncUiCommandState(const models::AvatarUiCommand& cmd);
    void setActionState(bool awaiting, const QStringList& actions);
    void resetSession(const QString& interviewId, const QString& direction, const QString& level,
                      const QString& language, bool offline);
    void appendMessage(const QString& role, const QString& content);
    void appendWelcome();
    void scheduleFallbackReply(int answersBefore);
    // Keeps a finished local interview in the demo history.
    void recordLocalInterview() const;
    void setBusy(bool busy);
    void setCompleting(bool completing);
    void setErrorMessage(const QString& message);

    std::shared_ptr<services::InterviewService> m_interviewService;
    NavigationViewModel* m_navigation;
    VisualAssessmentProbe m_visualAssessmentProbe;
    VisualContextProbe m_visualContextProbe;
    std::function<void()> m_ensureServicesProbe;
    std::function<bool()> m_localModelReadyProbe;
    MessageListModel m_messages;
    QTimer m_clock;

    QString m_interviewId;
    QString m_direction;
    QString m_level;
    QString m_language;
    QString m_errorMessage;
    std::string m_startedAt;
    // Incremented for every new session so late replies of an old one are ignored.
    int m_sessionToken{0};
    int m_elapsedSeconds{0};
    int m_fallbackReplyDelayMs{900};
    bool m_offline{false};
    bool m_completed{false};
    bool m_isBusy{false};
    bool m_isCompleting{false};
    bool m_awaitingNextAction{false};
    int m_answeredCount{0};
    QStringList m_suggestedActions;

    QString m_avatarEmotion{QStringLiteral("smile")};
    qreal m_avatarVolume{0.85};
    bool m_avatarMuted{false};
    QString m_avatarVoice{QStringLiteral("uk-UA-OstapNeural")};
    QString m_avatarOutputMode{QStringLiteral("voice")};
    QString m_avatarSoundEffect{QStringLiteral("dialogue_blip_soft.wav")};
    QString m_avatarGender{QStringLiteral("Male")};
    QString m_avatarPreset{QStringLiteral("realistic")};
    bool m_hatVisible{false};
    bool m_glassesVisible{false};
    bool m_tieVisible{false};
    bool m_hornsVisible{false};
    bool m_haloVisible{false};
};

} // namespace devdialogue::viewmodels
