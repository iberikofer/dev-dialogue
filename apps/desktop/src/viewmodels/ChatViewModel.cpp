#include "viewmodels/ChatViewModel.hpp"

#include <QJsonObject>
#include <QPointer>
#include <algorithm>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

const QString RoleAssistant = QStringLiteral("assistant");
const QString RoleUser = QStringLiteral("user");
const QString DefaultDirection = QStringLiteral("frontend");
const QString DefaultLevel = QStringLiteral("junior");
const QString DefaultLanguage = QStringLiteral("uk");

} // namespace

ChatViewModel::ChatViewModel(std::shared_ptr<services::InterviewService> interviewService,
                             NavigationViewModel* navigation, QObject* parent)
    : QObject(parent), m_interviewService(std::move(interviewService)), m_navigation(navigation) {
    m_clock.setInterval(1000);
    connect(&m_clock, &QTimer::timeout, this, [this]() {
        ++m_elapsedSeconds;
        emit elapsedChanged();
    });
}

QString ChatViewModel::elapsedText() const {
    return QStringLiteral("%1:%2")
        .arg(m_elapsedSeconds / 60, 2, 10, QLatin1Char('0'))
        .arg(m_elapsedSeconds % 60, 2, 10, QLatin1Char('0'));
}

void ChatViewModel::beginSession(const QString& interviewId, const QString& direction, const QString& level,
                                 const QString& language, const QString& firstQuestion, bool offline) {
    resetSession(interviewId, direction, level, language, offline);
    const bool uk = m_language == QLatin1String("uk");
    const QString expectedDefaultVoice = uk
        ? (m_avatarGender == QLatin1String("Female") ? QStringLiteral("uk-UA-PolinaNeural") : QStringLiteral("uk-UA-OstapNeural"))
        : (m_avatarGender == QLatin1String("Female") ? QStringLiteral("en-US-AvaNeural") : QStringLiteral("en-US-AndrewNeural"));
    if (m_avatarVoice != expectedDefaultVoice &&
        (m_avatarVoice.startsWith(QLatin1String("uk-UA-")) || m_avatarVoice.startsWith(QLatin1String("en-US-")))) {
        m_avatarVoice = expectedDefaultVoice;
        emit avatarVoiceChanged();
    }

    QString spoken = firstQuestion;
    if (firstQuestion.isEmpty()) {
        appendWelcome();
        spoken = demo::chatWelcome(m_direction, m_level, m_language);
    } else {
        appendMessage(RoleAssistant, firstQuestion);
    }
    applyAvatarResponse(spoken, QStringLiteral("smile"), {"Smile"}, {});
}

void ChatViewModel::openSession(const QString& interviewId) {
    if (m_ensureServicesProbe) {
        m_ensureServicesProbe();
    }
    if (m_localModelReadyProbe && !m_localModelReadyProbe()) {
        emit localModelRequired();
    }

    resetSession(interviewId, DefaultDirection, DefaultLevel, DefaultLanguage, false);

    if (!m_interviewService || !backendLikelyOnline()) {
        m_offline = true;
        emit sessionChanged();
        appendWelcome();
        return;
    }

    const int token = m_sessionToken;
    QPointer<ChatViewModel> self(this);
    m_interviewService->getInterview(
        interviewId, [self, token](const std::variant<models::InterviewDetail, models::ApiError>& result) {
            if (!self || token != self->m_sessionToken) {
                return;
            }
            if (const auto* detail = std::get_if<models::InterviewDetail>(&result)) {
                self->m_direction = support::enumToString(detail->direction);
                self->m_level = support::enumToString(detail->level);
                self->m_language = support::enumToString(detail->language);
                emit self->sessionChanged();

                self->m_messages.setMessages(detail->transcript);
                self->m_answeredCount = self->m_messages.userMessageCount();
                emit self->messagesChanged();
                if (self->m_messages.count() == 0) {
                    self->appendWelcome();
                } else if (!detail->transcript.empty() && detail->transcript.back().role == "assistant") {
                    const QString lastMsg = QString::fromStdString(detail->transcript.back().content);
                    if (lastMsg.startsWith(QString::fromUtf8("Оцінка:")) || lastMsg.startsWith(QLatin1String("Score:"))
                        || lastMsg.startsWith(QString::fromUtf8("Еталонна відповідь:")) || lastMsg.startsWith(QLatin1String("Reference answer:"))
                        || lastMsg.startsWith(QString::fromUtf8("Розбір балу")) || lastMsg.startsWith(QLatin1String("Score breakdown"))) {
                        QStringList restoredActions{QStringLiteral("explain_reference"), QStringLiteral("explain_answer")};
                        if (self->m_answeredCount < 5) {
                            restoredActions.append(QStringLiteral("next_question"));
                        }
                        self->setActionState(true, restoredActions);
                    }
                }
                return;
            }
            self->m_offline = true;
            emit self->sessionChanged();
            self->appendWelcome();
        });
}

void ChatViewModel::sendAnswer(const QString& text) {
    const QString answer = text.trimmed();
    if (answer.isEmpty() || m_isBusy || m_isCompleting || m_awaitingNextAction || m_interviewId.isEmpty()) {
        return;
    }

    const int answersBefore = m_answeredCount;
    ++m_answeredCount;
    appendMessage(RoleUser, answer);
    setErrorMessage(QString());
    setBusy(true);

    if (m_offline || !m_interviewService) {
        scheduleFallbackReply(answersBefore);
        return;
    }

    const int token = m_sessionToken;
    QPointer<ChatViewModel> self(this);
    const auto liveVisualContext = m_visualContextProbe ? m_visualContextProbe() : std::nullopt;
    m_interviewService->sendMessage(
        m_interviewId, answer,
        [self, token, answersBefore](const std::variant<models::SendMessageResponse, models::ApiError>& result) {
            if (!self || token != self->m_sessionToken) {
                return;
            }
            if (const auto* reply = std::get_if<models::SendMessageResponse>(&result)) {
                QString replyText;
                if (!reply->message.empty()) {
                    replyText = QString::fromStdString(reply->message);
                    self->appendMessage(RoleAssistant, replyText);
                }
                QStringList actions;
                for (const std::string& act : reply->suggestedActions) {
                    actions.append(QString::fromStdString(act));
                }
                self->setActionState(reply->awaitingNextAction, actions);
                self->applyAvatarResponse(replyText, QString::fromStdString(reply->emotion),
                                          reply->actionsSequence, reply->uiCommands);
                self->setBusy(false);
                return;
            }
            // Keep the conversation going without the backend; once the
            // network is down the rest of the session stays local.
            if (std::get<models::ApiError>(result).isNetworkDown && !self->m_offline) {
                self->m_offline = true;
                emit self->sessionChanged();
            }
            self->scheduleFallbackReply(answersBefore);
        },
        QString(),
        liveVisualContext);
}

void ChatViewModel::triggerFollowUpAction(const QString& action) {
    const QString trimmed = action.trimmed();
    if (trimmed.isEmpty() || !m_awaitingNextAction || m_isBusy || m_isCompleting || m_interviewId.isEmpty()) {
        return;
    }

    const bool uk = m_language == QLatin1String("uk");
    QString userPromptText = trimmed;
    if (trimmed == QLatin1String("explain_reference")) {
        userPromptText = uk ? QString::fromUtf8("Поясни еталонну відповідь") : QStringLiteral("Explain the reference answer");
    } else if (trimmed == QLatin1String("explain_answer")) {
        userPromptText = uk ? QString::fromUtf8("Поясни мою відповідь і чому саме такий бал") : QStringLiteral("Explain my answer and score");
    } else if (trimmed == QLatin1String("next_question")) {
        userPromptText = uk ? QString::fromUtf8("Перейти до наступного питання") : QStringLiteral("Move to the next question");
    }

    appendMessage(RoleUser, userPromptText);
    setErrorMessage(QString());
    setBusy(true);

    if (m_offline || !m_interviewService) {
        if (trimmed == QLatin1String("next_question")) {
            setActionState(false, {});
            m_messages.clear();
            emit messagesChanged();
            scheduleFallbackReply(m_answeredCount);
        } else {
            appendMessage(RoleAssistant, uk
                ? QString::fromUtf8("Офлайн-демо: детальний розбір і оцінка моделі доступні при підключеному сервері.")
                : QStringLiteral("Offline demo: detailed AI breakdown is available when connected to the server."));
            QStringList remaining = m_suggestedActions;
            remaining.removeAll(trimmed);
            setActionState(!remaining.isEmpty(), remaining);
            setBusy(false);
        }
        return;
    }

    const int token = m_sessionToken;
    QPointer<ChatViewModel> self(this);
    m_interviewService->sendMessage(
        m_interviewId, userPromptText,
        [self, token, trimmed](const std::variant<models::SendMessageResponse, models::ApiError>& result) {
            if (!self || token != self->m_sessionToken) {
                return;
            }
            if (const auto* reply = std::get_if<models::SendMessageResponse>(&result)) {
                if (trimmed == QLatin1String("next_question") && !reply->awaitingNextAction) {
                    // One chat = one question: clear the previous question thread and show only the fresh question.
                    self->m_messages.clear();
                    emit self->messagesChanged();
                }
                QString replyText;
                if (!reply->message.empty()) {
                    replyText = QString::fromStdString(reply->message);
                    self->appendMessage(RoleAssistant, replyText);
                }
                QStringList actions;
                for (const std::string& act : reply->suggestedActions) {
                    actions.append(QString::fromStdString(act));
                }
                self->setActionState(reply->awaitingNextAction, actions);
                self->applyAvatarResponse(replyText, QString::fromStdString(reply->emotion),
                                          reply->actionsSequence, reply->uiCommands);
                self->setBusy(false);
                return;
            }
            self->setErrorMessage(QString::fromStdString(std::get<models::ApiError>(result).message));
            self->setBusy(false);
        },
        trimmed);
}

void ChatViewModel::completeInterview() {
    if (m_isCompleting || m_interviewId.isEmpty()) {
        return;
    }
    setCompleting(true);
    m_clock.stop();
    emit avatarSpeechStopRequested();

    const QString interviewId = m_interviewId;
    QPointer<ChatViewModel> self(this);
    // The report opens whether or not the server call succeeds; the report
    // screen has its own fallback, exactly like the web client.
    auto finish = [self, interviewId]() {
        if (!self) {
            return;
        }
        self->setCompleting(false);
        self->m_completed = true;
        emit self->sessionChanged();
        emit self->interviewCompleted(interviewId);
        if (self->m_navigation) {
            self->m_navigation->navigateToReport(interviewId);
        }
    };

    if (m_offline || !m_interviewService) {
        recordLocalInterview();
        finish();
        return;
    }
    const auto visual = m_visualAssessmentProbe ? m_visualAssessmentProbe() : std::nullopt;
    m_interviewService->completeInterview(
        interviewId,
        [finish](const std::variant<models::CompleteInterviewResponse, models::ApiError>&) { finish(); },
        visual);
}

void ChatViewModel::resetSession(const QString& interviewId, const QString& direction, const QString& level,
                                 const QString& language, bool offline) {
    ++m_sessionToken;
    m_interviewId = interviewId;
    m_direction = direction;
    m_level = level;
    m_language = language;
    m_offline = offline;
    m_completed = false;
    m_startedAt = support::nowIso();
    emit sessionChanged();

    m_messages.clear();
    m_answeredCount = 0;
    emit messagesChanged();

    setBusy(false);
    setCompleting(false);
    setErrorMessage(QString());
    setActionState(false, {});

    m_elapsedSeconds = 0;
    emit elapsedChanged();
    m_clock.start();
}

void ChatViewModel::appendMessage(const QString& role, const QString& content) {
    m_messages.append(role, content);
    emit messagesChanged();
}

void ChatViewModel::appendWelcome() {
    appendMessage(RoleAssistant, demo::chatWelcome(m_direction, m_level, m_language));
}

void ChatViewModel::scheduleFallbackReply(int answersBefore) {
    const int token = m_sessionToken;
    QTimer::singleShot(m_fallbackReplyDelayMs, this, [this, token, answersBefore]() {
        if (token != m_sessionToken) {
            return;
        }
        const QStringList replies = demo::chatReplies(m_direction, m_language);
        const qsizetype index = std::min<qsizetype>(answersBefore, replies.size() - 1);
        const QString replyText = replies.at(index);
        appendMessage(RoleAssistant, replyText);
        applyAvatarResponse(replyText, QStringLiteral("thinking"), {"Thinking"}, {});
        setBusy(false);
    });
}

void ChatViewModel::recordLocalInterview() const {
    models::InterviewDetail interview;
    interview.id = m_interviewId.toStdString();
    interview.direction = support::enumFromString<models::InterviewDirection>(m_direction);
    interview.level = support::enumFromString<models::InterviewLevel>(m_level);
    interview.language = support::enumFromString<models::InterviewLanguage>(m_language);
    interview.status = models::InterviewStatus::Completed;
    interview.createdAt = m_startedAt;
    interview.updatedAt = support::nowIso();
    interview.transcript = m_messages.messages();
    if (m_visualAssessmentProbe) {
        if (const auto vis = m_visualAssessmentProbe(); vis.has_value()) {
            models::InterviewFeedbackReport rep = demo::reportFromTranscript(m_direction, interview.transcript, m_language);
            rep.visualAssessment = vis;
            interview.feedback = rep;
        }
    }
    demo::recordFinishedInterview(interview);
}

void ChatViewModel::setBusy(bool busy) {
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit busyChanged();
    }
}

void ChatViewModel::setCompleting(bool completing) {
    if (m_isCompleting != completing) {
        m_isCompleting = completing;
        emit completingChanged();
    }
}

void ChatViewModel::setErrorMessage(const QString& message) {
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

void ChatViewModel::setActionState(bool awaiting, const QStringList& actions) {
    if (m_awaitingNextAction != awaiting || m_suggestedActions != actions) {
        m_awaitingNextAction = awaiting;
        m_suggestedActions = actions;
        emit actionStateChanged();
    }
}

QVariantList ChatViewModel::availableVoices() const {
    auto item = [](const QString& id, const QString& label, const QString& gender) {
        return QVariantMap{{QStringLiteral("id"), id},
                           {QStringLiteral("label"), label},
                           {QStringLiteral("gender"), gender}};
    };
    return {
        item(QStringLiteral("uk-UA-OstapNeural"), QString::fromUtf8("🇺🇦 Остап (UA, Чол.)"), QStringLiteral("Male")),
        item(QStringLiteral("uk-UA-PolinaNeural"), QString::fromUtf8("🇺🇦 Поліна (UA, Жін.)"), QStringLiteral("Female")),
        item(QStringLiteral("en-US-AndrewNeural"), QString::fromUtf8("🇺🇸 Andrew (EN-US, Чол.)"), QStringLiteral("Male")),
        item(QStringLiteral("en-US-AvaNeural"), QString::fromUtf8("🇺🇸 Ava (EN-US, Жін.)"), QStringLiteral("Female")),
        item(QStringLiteral("en-US-BrianNeural"), QString::fromUtf8("🇺🇸 Brian (EN-US, Баритон)"), QStringLiteral("Male")),
        item(QStringLiteral("en-US-EmmaMultilingualNeural"), QString::fromUtf8("🇺🇸 Emma (Multilingual)"), QStringLiteral("Female")),
        item(QStringLiteral("en-GB-RyanNeural"), QString::fromUtf8("🇬🇧 Ryan (EN-GB, Чол.)"), QStringLiteral("Male")),
        item(QStringLiteral("en-GB-SoniaNeural"), QString::fromUtf8("🇬🇧 Sonia (EN-GB, Жін.)"), QStringLiteral("Female")),
        item(QStringLiteral("pl-PL-MarekNeural"), QString::fromUtf8("🇵🇱 Marek (PL, Чол.)"), QStringLiteral("Male")),
        item(QStringLiteral("de-DE-KatjaNeural"), QString::fromUtf8("🇩🇪 Katja (DE, Жін.)"), QStringLiteral("Female")),
        item(QStringLiteral("fr-FR-DeniseNeural"), QString::fromUtf8("🇫🇷 Denise (FR, Жін.)"), QStringLiteral("Female"))
    };
}

QVariantList ChatViewModel::availablePresets() const {
    auto item = [](const QString& id, const QString& label) {
        return QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("label"), label}};
    };
    return {
        item(QStringLiteral("realistic"), QStringLiteral("Realistic")),
        item(QStringLiteral("warm_olive"), QStringLiteral("Warm Olive")),
        item(QStringLiteral("freckled"), QStringLiteral("Freckled")),
        item(QStringLiteral("deep_ebony"), QStringLiteral("Deep Ebony")),
        item(QStringLiteral("golden_asian"), QStringLiteral("Golden")),
        item(QStringLiteral("cyber"), QStringLiteral("Cyber")),
        item(QStringLiteral("noir"), QStringLiteral("Noir")),
        item(QStringLiteral("alien"), QStringLiteral("Alien"))
    };
}

QVariantList ChatViewModel::availableSoundEffects() const {
    auto item = [](const QString& id, const QString& label) {
        return QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("label"), label}};
    };
    return {
        item(QStringLiteral("dialogue_blip_soft.wav"), QString::fromUtf8("Діалоговий бліп (RPG)")),
        item(QStringLiteral("dialogue_blip_synth.wav"), QString::fromUtf8("Синтетичний бліп")),
        item(QStringLiteral("text_print_soft.wav"), QString::fromUtf8("М'який друк тексту")),
        item(QStringLiteral("typewriter_key.wav"), QString::fromUtf8("Клавіша машинки")),
        item(QStringLiteral("tech_ui_chirp.wav"), QString::fromUtf8("Футуристичний чирп")),
        item(QStringLiteral("whisper_murmur.wav"), QString::fromUtf8("Шепіт / шелест")),
        item(QStringLiteral("chime_done.wav"), QString::fromUtf8("М'який дзвіночок"))
    };
}

void ChatViewModel::setAvatarVolume(qreal volume) {
    const qreal clamped = std::clamp(volume, 0.0, 1.0);
    if (!qFuzzyCompare(1.0 + m_avatarVolume, 1.0 + clamped)) {
        m_avatarVolume = clamped;
        emit avatarVolumeChanged();
    }
}

void ChatViewModel::setAvatarMuted(bool muted) {
    if (m_avatarMuted != muted) {
        m_avatarMuted = muted;
        emit avatarMutedChanged();
    }
}

void ChatViewModel::toggleAvatarMute() {
    setAvatarMuted(!m_avatarMuted);
}

void ChatViewModel::stopAvatarSpeech() {
    emit avatarSpeechStopRequested();
}

void ChatViewModel::setAvatarVoice(const QString& voiceId) {
    const QString trimmed = voiceId.trimmed();
    if (!trimmed.isEmpty() && m_avatarVoice != trimmed) {
        m_avatarVoice = trimmed;
        emit avatarVoiceChanged();
    }
}

void ChatViewModel::setAvatarOutputMode(const QString& mode) {
    const QString clean = mode.trimmed().toLower();
    if ((clean == QLatin1String("voice") || clean == QLatin1String("sound") || clean == QLatin1String("silent"))
        && m_avatarOutputMode != clean) {
        m_avatarOutputMode = clean;
        emit avatarOutputModeChanged();
    }
}

void ChatViewModel::toggleAvatarOutputMode() {
    setAvatarOutputMode(m_avatarOutputMode == QLatin1String("sound")
                            ? QStringLiteral("voice")
                            : QStringLiteral("sound"));
}

void ChatViewModel::setAvatarSoundEffect(const QString& soundFile) {
    const QString trimmed = soundFile.trimmed();
    if (!trimmed.isEmpty() && m_avatarSoundEffect != trimmed) {
        m_avatarSoundEffect = trimmed;
        emit avatarSoundEffectChanged();
    }
}

void ChatViewModel::cycleAvatarSoundEffect() {
    const QVariantList list = availableSoundEffects();
    if (list.isEmpty()) return;
    int idx = 0;
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).toMap().value(QStringLiteral("id")).toString() == m_avatarSoundEffect) {
            idx = (i + 1) % list.size();
            break;
        }
    }
    setAvatarSoundEffect(list.at(idx).toMap().value(QStringLiteral("id")).toString());
    if (m_avatarOutputMode != QLatin1String("sound")) {
        setAvatarOutputMode(QStringLiteral("sound"));
    }
}

void ChatViewModel::setAvatarGender(const QString& gender) {
    if ((gender == QLatin1String("Male") || gender == QLatin1String("Female")) && m_avatarGender != gender) {
        m_avatarGender = gender;
        emit avatarGenderChanged();
    }
}

void ChatViewModel::setAvatarPreset(const QString& presetId) {
    const QString trimmed = presetId.trimmed();
    if (!trimmed.isEmpty() && m_avatarPreset != trimmed) {
        m_avatarPreset = trimmed;
        emit avatarPresetChanged();
    }
}

void ChatViewModel::setAccessoryVisible(const QString& name, bool visible) {
    bool changed = false;
    auto updateFlag = [&](bool& slot) {
        if (slot != visible) {
            slot = visible;
            changed = true;
        }
    };
    if (name == QLatin1String("Hat")) updateFlag(m_hatVisible);
    else if (name == QLatin1String("Glasses")) updateFlag(m_glassesVisible);
    else if (name == QLatin1String("Tie")) updateFlag(m_tieVisible);
    else if (name == QLatin1String("Horns")) updateFlag(m_hornsVisible);
    else if (name == QLatin1String("Halo")) updateFlag(m_haloVisible);

    if (changed) {
        emit avatarWardrobeChanged();
    }
}

void ChatViewModel::toggleAccessory(const QString& name) {
    if (name == QLatin1String("Hat")) setAccessoryVisible(name, !m_hatVisible);
    else if (name == QLatin1String("Glasses")) setAccessoryVisible(name, !m_glassesVisible);
    else if (name == QLatin1String("Tie")) setAccessoryVisible(name, !m_tieVisible);
    else if (name == QLatin1String("Horns")) setAccessoryVisible(name, !m_hornsVisible);
    else if (name == QLatin1String("Halo")) setAccessoryVisible(name, !m_haloVisible);
}

void ChatViewModel::triggerTestEmotion(const QString& emotion) {
    const QString clean = emotion.trimmed().toLower();
    if (clean.isEmpty()) return;
    if (m_avatarEmotion != clean) {
        m_avatarEmotion = clean;
        emit avatarEmotionChanged();
    }
    const QString clip = clean.left(1).toUpper() + clean.mid(1);
    emit avatarReactRequested(clean, {clip}, QJsonArray());
}

void ChatViewModel::syncUiCommandState(const models::AvatarUiCommand& cmd) {
    const QString action = QString::fromStdString(cmd.action);
    const QString target = QString::fromStdString(cmd.target);
    if (action == QLatin1String("accessory_on")) {
        setAccessoryVisible(target, true);
    } else if (action == QLatin1String("accessory_off")) {
        setAccessoryVisible(target, false);
    } else if (action == QLatin1String("accessory_toggle")) {
        toggleAccessory(target);
    } else if (action == QLatin1String("set_gender")) {
        setAvatarGender(target);
    }
}

void ChatViewModel::applyAvatarResponse(const QString& textToSpeak, const QString& emotion,
                                          const std::vector<std::string>& actionsSequence,
                                          const std::vector<models::AvatarUiCommand>& uiCommands) {
    const QString nextEmotion = emotion.trimmed().isEmpty() ? QStringLiteral("neutral") : emotion.trimmed().toLower();
    if (m_avatarEmotion != nextEmotion) {
        m_avatarEmotion = nextEmotion;
        emit avatarEmotionChanged();
    }

    QStringList seqList;
    seqList.reserve(static_cast<qsizetype>(actionsSequence.size()));
    for (const std::string& a : actionsSequence) {
        seqList.append(QString::fromStdString(a));
    }

    QJsonArray uiArr;
    for (const auto& cmd : uiCommands) {
        syncUiCommandState(cmd);
        QJsonObject obj;
        obj.insert(QStringLiteral("action"), QString::fromStdString(cmd.action));
        obj.insert(QStringLiteral("target"), QString::fromStdString(cmd.target));
        uiArr.append(obj);
    }

    QString cleanSpeech = textToSpeak;
    cleanSpeech.remove(QLatin1Char('*'));
    cleanSpeech = cleanSpeech.trimmed();
    if (!cleanSpeech.isEmpty()) {
        emit avatarSpeakRequested(cleanSpeech, nextEmotion, seqList, uiArr);
    } else {
        emit avatarReactRequested(nextEmotion, seqList, uiArr);
    }
}

} // namespace devdialogue::viewmodels
