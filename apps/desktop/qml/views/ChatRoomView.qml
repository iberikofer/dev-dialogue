pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import DevDialogue.App

// Zoom-style Interview Room:
// - When Chat is closed: Sidebar visible on left; 50/50 Zoom tiles (Vladyslav + You).
// - When Chat is opened: Sidebar hides (Main.qml), Chat Drawer opens on the LEFT
//   for maximum space, and BOTH Video Tiles (Vladyslav + your icon/webcam) stay visible.
AppPage {
    id: page

    property bool active_: StackLayout.isCurrentItem
    property bool avatarAllowed: true
    property bool sidebarExpanded: true
    property bool chatOpen: false
    property bool devicePromptShownThisRun: false
    signal debugRequested()

    function syncAvatarViewport() {
        if (!avatarAllowed || !active_ || deviceSettings.open || avatarSlot.width <= 10 || avatarSlot.height <= 10) {
            avatarHost.setViewport(0, 0, 0, 0, false)
            return
        }
        const pt = avatarSlot.mapToItem(null, 0, 0)
        avatarHost.setViewport(pt.x, pt.y, avatarSlot.width, avatarSlot.height, true)
    }

    function handleMicPress() {
        if (micService.transcribing || chatVM.isBusy || chatVM.isCompleting) {
            return
        }
        if (!micService.permissionGranted || !micService.osPermissionAllowed) {
            deviceSettings.pendingAction = "mic"
            deviceSettings.open = true
            return
        }
        if (micService.muted) {
            return
        }
        if (settingsVM.micMode === "toggle") {
            if (micService.recording) {
                micService.stopAndTranscribe()
            } else {
                chatVM.stopAvatarSpeech()
                micService.startRecording()
            }
            return
        }
        if (!micService.recording) {
            chatVM.stopAvatarSpeech()
            micService.startRecording()
        }
    }

    function handleMicRelease() {
        if (settingsVM.micMode === "hold" && micService.recording) {
            micService.stopAndTranscribe()
        }
    }

    function triggerCameraAction() {
        if (webcamService.active) {
            webcamService.setActive(false)
            return
        }
        if (!webcamService.permissionGranted || !webcamService.osPermissionAllowed) {
            deviceSettings.pendingAction = "cam"
            deviceSettings.open = true
            return
        }
        webcamService.setActive(true)
    }

    function visualHeatColor(score) {
        const s = Math.max(0, Math.min(100, score))
        if (s >= 50) {
            const t = (s - 50) / 50.0
            return Qt.rgba(
                0.96 + (0.06 - 0.96) * t,
                0.62 + (0.73 - 0.62) * t,
                0.04 + (0.51 - 0.04) * t,
                1.0
            )
        }
        const t = s / 50.0
        return Qt.rgba(
            0.94 + (0.96 - 0.94) * t,
            0.27 + (0.62 - 0.27) * t,
            0.27 + (0.04 - 0.27) * t,
            1.0
        )
    }

    function activeSoundShortName() {
        const list = chatVM.availableSoundEffects
        for (let i = 0; i < list.length; ++i) {
            if (list[i].id === chatVM.avatarSoundEffect)
                return list[i].label
        }
        return "Sound"
    }

    onChatOpenChanged: {
        Qt.callLater(syncAvatarViewport)
        if (chatOpen && !chatVM.awaitingNextAction) {
            Qt.callLater(() => {
                avatarHost.focusHostWindow()
                input.area.forceActiveFocus()
            })
        }
    }
    onAvatarAllowedChanged: Qt.callLater(syncAvatarViewport)
    onSidebarExpandedChanged: Qt.callLater(syncAvatarViewport)
    onWidthChanged: Qt.callLater(syncAvatarViewport)
    onHeightChanged: Qt.callLater(syncAvatarViewport)

    active: active_
    onActiveChanged: {
        if (!active && webcamService.active) {
            webcamService.setActive(false)
        }
        if (!active) {
            micService.cancelRecording()
            micService.stopMicTest()
        }
        if (active && chatOpen && !chatVM.awaitingNextAction) {
            input.area.forceActiveFocus()
        }
        if (active && !page.devicePromptShownThisRun &&
                (!micService.permissionGranted || !webcamService.permissionGranted ||
                 !micService.osPermissionAllowed || !webcamService.osPermissionAllowed)) {
            page.devicePromptShownThisRun = true
            Qt.callLater(() => {
                deviceSettings.pendingAction = ""
                deviceSettings.open = true
            })
        }
        Qt.callLater(syncAvatarViewport)
    }

    Connections {
        target: settingsVM
        function onMicTalkPressed() {
            if (page.active_ && !deviceSettings.open) {
                page.handleMicPress()
            }
        }
        function onMicTalkReleased() {
            if (settingsVM.micMode === "hold") {
                page.handleMicRelease()
            }
        }
        function onMicMuteTriggered() {
            if (page.active_) {
                micService.toggleMuted()
            }
        }
        function onCameraToggleTriggered() {
            if (page.active_ && !deviceSettings.open) {
                page.triggerCameraAction()
            }
        }
        function onAiMuteTriggered() {
            if (page.active_) {
                chatVM.toggleAvatarMute()
            }
        }
    }

    Connections {
        target: webcamService
        function onPermissionRequired() {
            deviceSettings.pendingAction = "cam"
            deviceSettings.open = true
        }
    }

    Connections {
        target: micService
        function onPermissionRequired() {
            deviceSettings.pendingAction = "mic"
            deviceSettings.open = true
        }
        function onTranscriptionReady(text) {
            chatVM.stopAvatarSpeech()
            page.chatOpen = true
            if (!chatVM.isBusy && !chatVM.isCompleting) {
                if (!chatVM.awaitingNextAction) {
                    chatVM.sendAnswer(text)
                } else {
                    const low = text.toLowerCase()
                    if (low.indexOf("наступн") >= 0 || low.indexOf("next") >= 0 ||
                            low.indexOf("далі") >= 0 || low.indexOf("продовж") >= 0) {
                        chatVM.triggerFollowUpAction("next_question")
                    } else if (low.indexOf("еталон") >= 0 || low.indexOf("reference") >= 0) {
                        chatVM.triggerFollowUpAction("explain_reference")
                    } else if (low.indexOf("бал") >= 0 || low.indexOf("відповід") >= 0 || low.indexOf("explain") >= 0) {
                        chatVM.triggerFollowUpAction("explain_answer")
                    } else {
                        // Also move to next question or let user inspect what was recognized
                        chatVM.triggerFollowUpAction("next_question")
                    }
                }
            } else {
                input.text = text
            }
        }
        function onTranscriptionError(err) {
            console.warn("STT error:", err)
        }
    }

    title: qsTr("Interview")
    subtitle: qsTr("Answers: %1  ·  Time: %2").arg(chatVM.candidateAnswersCount).arg(chatVM.elapsedText)
    badgeText: chatVM.isLocalSession ? qsTr("Offline demo") : ""
    bodyPadding: 20
    bodySpacing: 12

    headerExtra: [
        Badge {
            tone: "blue"
            mono: true
            caps: true
            text: chatVM.currentDirection
            fontSize: Typography.tiny
            fontWeight: Typography.semibold
        },
        Badge {
            tone: "neutral"
            mono: true
            text: chatVM.currentLevel.charAt(0).toUpperCase() + chatVM.currentLevel.slice(1)
            fontSize: Typography.tiny
        }
    ]

    actions: [
        TactileButton {
            variant: "secondary"
            text: "Debug"
            iconLeft: "code-xml"
            iconSize: 14
            fontSize: Typography.xs
            leftPadding: 14
            rightPadding: 14
            implicitHeight: 38
            onClicked: page.debugRequested()
        },
        Item {
            implicitWidth: finishButton.implicitWidth
            implicitHeight: finishButton.implicitHeight
            HoverHandler { id: finishHover }
            TactileButton {
                id: finishButton
                anchors.fill: parent
                variant: "success"
                text: chatVM.isCompleting ? qsTr("Generating...") : qsTr("Finish & report")
                iconLeft: chatVM.isCompleting ? "" : "circle-check-big"
                busy: chatVM.isCompleting
                fontSize: Typography.xs
                leftPadding: 14
                rightPadding: 14
                implicitHeight: 38
                enabled: !chatVM.isCompleting && !chatVM.isBusy && chatVM.candidateAnswersCount > 0
                opacity: enabled || chatVM.isCompleting ? 1.0 : 0.4
                onClicked: chatVM.completeInterview()
            }
            AppToolTip {
                delay: 300
                visible: finishHover.hovered && chatVM.candidateAnswersCount === 0
                text: qsTr("Answer at least one question first")
            }
        }
    ]

    function send() {
        const answer = input.text.trim()
        if (answer.length === 0 || chatVM.isBusy || chatVM.isCompleting)
            return
        chatVM.sendAnswer(answer)
        input.text = ""
    }

    AlertBox {
        visible: chatVM.errorMessage !== ""
        Layout.fillWidth: true
        text: chatVM.errorMessage
    }

    // --------------------------------------------------------- Main Split: Left Chat Drawer + Right Zoom Stage
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 14

        // LEFT (Replaces Sidebar when opened): Meeting Chat Drawer
        Rectangle {
            id: chatDrawer
            visible: page.chatOpen
            Layout.preferredWidth: Math.min(430, Math.max(330, Math.round(page.width * 0.34)))
            Layout.fillHeight: true
            radius: Theme.radiusLg
            color: "#0b0d12"
            border.width: 1
            border.color: Theme.borderGlass
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                // Drawer Header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    PulseDot {
                        color: chatVM.isBusy ? Theme.blue400 : Theme.emerald500
                        pingColor: Theme.blue400
                        pinging: chatVM.isBusy
                    }

                    MonoText {
                        Layout.fillWidth: true
                        caps: true
                        text: chatVM.sessionLanguage === "uk"
                            ? "Чат · Питання %1/5".arg(Math.min(5, chatVM.candidateAnswersCount + (chatVM.awaitingNextAction ? 0 : 1)))
                            : qsTr("Chat · Q %1/5").arg(Math.min(5, chatVM.candidateAnswersCount + (chatVM.awaitingNextAction ? 0 : 1)))
                        color: Theme.zinc200
                        font.pixelSize: Typography.xs
                        font.weight: Typography.semibold
                    }

                    TactileButton {
                        variant: "ghost"
                        text: "✕"
                        fontSize: Typography.xs
                        leftPadding: 10
                        rightPadding: 10
                        implicitHeight: 28
                        onClicked: page.chatOpen = false
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderFaint
                }

                // Current Question / Answer / Evaluation Thread (resets automatically on Next Question)
                ListView {
                    id: transcript
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 12
                    boundsBehavior: Flickable.StopAtBounds
                    model: chatVM.messagesModel
                    ScrollBar.vertical: ThemedScrollBar {}

                    function scrollToEnd() {
                        Qt.callLater(transcript.positionViewAtEnd)
                    }
                    onCountChanged: scrollToEnd()

                    header: Item { width: 1; height: 4 }

                    delegate: Item {
                        id: entry
                        required property string role
                        required property string content
                        required property string timeText

                        width: transcript.width - 8
                        height: bubble.implicitHeight

                        MessageBubble {
                            id: bubble
                            width: parent.width
                            senderRole: entry.role
                            content: entry.content
                            timestamp: entry.timeText
                        }
                    }

                    footer: Item {
                        width: transcript.width - 8
                        height: chatVM.isBusy ? thinking.implicitHeight + 12 : 4
                        onHeightChanged: transcript.scrollToEnd()

                        RowLayout {
                            id: thinking
                            visible: chatVM.isBusy
                            y: 8
                            spacing: 10

                            Rectangle {
                                Layout.alignment: Qt.AlignTop
                                implicitWidth: 28
                                implicitHeight: 28
                                radius: Theme.radiusMd
                                color: Theme.zinc900
                                border.width: 1
                                border.color: Theme.borderGlass
                                Icon { anchors.centerIn: parent; name: "bot"; size: 14; color: Theme.sky300 }
                            }
                            Rectangle {
                                implicitWidth: dots.implicitWidth + 28
                                implicitHeight: 34
                                radius: Theme.radiusLg
                                color: Theme.alpha(Theme.zinc900, 0.9)
                                border.width: 1
                                border.color: Theme.borderGlass

                                Row {
                                    id: dots
                                    anchors.centerIn: parent
                                    spacing: 5
                                    Repeater {
                                        model: 3
                                        Rectangle {
                                            id: dot
                                            required property int index
                                            width: 6
                                            height: 6
                                            radius: 3
                                            color: Theme.zinc400
                                            SequentialAnimation on opacity {
                                                running: chatVM.isBusy
                                                loops: Animation.Infinite
                                                PauseAnimation { duration: dot.index * 160 }
                                                NumberAnimation { from: 0.3; to: 1; duration: 320 }
                                                NumberAnimation { from: 1; to: 0.3; duration: 320 }
                                                PauseAnimation { duration: (2 - dot.index) * 160 }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Answer Input Box (Type Answer -> Send -> AI Reaction + Face + Voice/Sounds)
                Item {
                    visible: !chatVM.awaitingNextAction
                    Layout.fillWidth: true
                    implicitHeight: 96
                    Layout.preferredHeight: 96

                    Connections {
                        target: input.area
                        function onActiveFocusChanged() {
                            if (input.area.activeFocus) {
                                avatarHost.focusHostWindow()
                            }
                        }
                    }

                    DarkTextArea {
                        id: input
                        anchors.fill: parent
                        trailingSpace: 92
                        enabled: !chatVM.isCompleting && !chatVM.awaitingNextAction && !micService.transcribing
                        placeholderText: micService.muted
                            ? (chatVM.sessionLanguage === "uk"
                                ? "Мікрофон у муті (%1 — увімкнути)".arg(settingsVM.micMuteShortcut)
                                : qsTr("Microphone muted (%1 to unmute)").arg(settingsVM.micMuteShortcut))
                            : (micService.recording
                                ? (settingsVM.micMode === "toggle"
                                    ? (chatVM.sessionLanguage === "uk" ? "Говоріть… Натисніть ще раз (%1), щоб надіслати".arg(settingsVM.micTalkShortcut) : qsTr("Speaking… Click again (%1) to send").arg(settingsVM.micTalkShortcut))
                                    : (chatVM.sessionLanguage === "uk" ? "Говоріть у мікрофон… Відпустіть щоб надіслати" : qsTr("Speaking… Release to send")))
                                : (micService.transcribing
                                    ? "Parakeet TDT..."
                                    : (micService.statusText.length > 0
                                        ? micService.statusText
                                        : (settingsVM.micMode === "toggle"
                                            ? (chatVM.sessionLanguage === "uk" ? "Впишіть відповідь або натисніть мікрофон (%1)…".arg(settingsVM.micTalkShortcut) : qsTr("Type answer or click mic (%1)…").arg(settingsVM.micTalkShortcut))
                                            : (chatVM.sessionLanguage === "uk" ? "Впишіть відповідь або затисніть мікрофон (%1)…".arg(settingsVM.micTalkShortcut) : qsTr("Type answer or hold mic (%1)…").arg(settingsVM.micTalkShortcut))))))
                        onSubmitted: page.send()
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.rightMargin: 96
                        acceptedButtons: Qt.LeftButton
                        onPressed: (mouse) => {
                            avatarHost.focusHostWindow()
                            input.area.forceActiveFocus()
                            mouse.accepted = false
                        }
                    }

                    TactileButton {
                        id: inlineMicBtn
                        variant: micService.recording ? "danger" : (micService.transcribing ? "soft" : "secondary")
                        busy: micService.transcribing
                        iconLeft: (micService.muted || !micService.permissionGranted || !micService.osPermissionAllowed) ? "mic-off" : "mic"
                        iconSize: 16
                        implicitWidth: 38
                        implicitHeight: 38
                        leftPadding: 0
                        rightPadding: 0
                        down: inlineMicHold.pressed
                        opacity: micService.muted ? 0.55 : 1.0
                        anchors.right: sendButton.left
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: 6
                        anchors.bottomMargin: 10
                        enabled: !chatVM.isBusy && !chatVM.isCompleting && !micService.transcribing

                        MouseArea {
                            id: inlineMicHold
                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onPressed: page.handleMicPress()
                            onReleased: page.handleMicRelease()
                            onCanceled: page.handleMicRelease()
                        }

                        AppToolTip {
                            visible: inlineMicHold.containsMouse
                            text: micService.muted
                                ? (chatVM.sessionLanguage === "uk" ? "Мікрофон у муті (%1)".arg(settingsVM.micMuteShortcut) : qsTr("Muted (%1)").arg(settingsVM.micMuteShortcut))
                                : (micService.recording
                                    ? (settingsVM.micMode === "toggle"
                                        ? (chatVM.sessionLanguage === "uk" ? "Натисніть щоб зупинити і надіслати" : qsTr("Click to stop & send"))
                                        : (chatVM.sessionLanguage === "uk" ? "Відпустіть щоб розпізнати й надіслати (Parakeet)" : qsTr("Release to transcribe & send")))
                                    : (settingsVM.micMode === "toggle"
                                        ? (chatVM.sessionLanguage === "uk" ? "Натисніть щоб говорити (%1)".arg(settingsVM.micTalkShortcut) : qsTr("Click to speak (%1)").arg(settingsVM.micTalkShortcut))
                                        : (chatVM.sessionLanguage === "uk" ? "Натисніть і тримайте (%1), щоб сказати відповідь".arg(settingsVM.micTalkShortcut) : qsTr("Press & hold (%1) to speak").arg(settingsVM.micTalkShortcut))))
                        }
                    }

                    TactileButton {
                        id: sendButton
                        variant: "primary"
                        iconLeft: "send"
                        implicitWidth: 38
                        implicitHeight: 38
                        leftPadding: 0
                        rightPadding: 0
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: 10
                        anchors.bottomMargin: 10
                        enabled: input.text.trim().length > 0 && !chatVM.isBusy && !chatVM.isCompleting && !chatVM.awaitingNextAction
                        opacity: enabled ? 1.0 : 0.35
                        onClicked: page.send()
                        Accessible.name: qsTr("Send answer")
                        AppToolTip {
                            visible: sendButton.hovered
                            text: qsTr("Send (Enter)")
                        }
                    }
                }

                // Post-Answer AI Reactions & Next Question
                Rectangle {
                    id: actionCard
                    visible: chatVM.awaitingNextAction
                    Layout.fillWidth: true
                    implicitHeight: actionCol.implicitHeight + 16
                    Layout.preferredHeight: actionCol.implicitHeight + 16
                    radius: Theme.radiusMd
                    color: Theme.surfaceCard
                    border.width: 1
                    border.color: Theme.borderGlass

                    readonly property bool ukSession: chatVM.sessionLanguage === "uk"

                    Connections {
                        target: chatVM
                        function onActionStateChanged() {
                            if (!chatVM.awaitingNextAction && page.active && page.chatOpen) {
                                avatarHost.focusHostWindow()
                                input.area.forceActiveFocus()
                            }
                        }
                    }

                    ColumnLayout {
                        id: actionCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        TactileButton {
                            visible: chatVM.suggestedActions.indexOf("explain_reference") >= 0
                            Layout.fillWidth: true
                            variant: "secondary"
                            text: actionCard.ukSession
                                ? "Поясни еталонну відповідь"
                                : qsTr("Explain reference answer")
                            iconLeft: "book-open"
                            iconSize: 14
                            fontSize: Typography.xs
                            implicitHeight: 36
                            enabled: !chatVM.isBusy && !chatVM.isCompleting
                            onClicked: chatVM.triggerFollowUpAction("explain_reference")
                        }

                        TactileButton {
                            visible: chatVM.suggestedActions.indexOf("explain_answer") >= 0
                            Layout.fillWidth: true
                            variant: "secondary"
                            text: actionCard.ukSession
                                ? "Поясни мою відповідь і бал"
                                : qsTr("Explain my answer & score")
                            iconLeft: "info"
                            iconSize: 14
                            fontSize: Typography.xs
                            implicitHeight: 36
                            enabled: !chatVM.isBusy && !chatVM.isCompleting
                            onClicked: chatVM.triggerFollowUpAction("explain_answer")
                        }

                        TactileButton {
                            visible: chatVM.suggestedActions.indexOf("next_question") >= 0
                            Layout.fillWidth: true
                            variant: "primary"
                            text: actionCard.ukSession
                                ? "Наступне питання →"
                                : qsTr("Next question →")
                            iconRight: "arrow-right"
                            iconSize: 14
                            fontSize: Typography.xs
                            implicitHeight: 38
                            enabled: !chatVM.isBusy && !chatVM.isCompleting
                            onClicked: chatVM.triggerFollowUpAction("next_question")
                        }
                    }
                }
            }
        }

        // RIGHT: Side-by-side Zoom Video Grid (always keeps both Vladyslav & your Icon/Webcam visible!) + Controls Bar
        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 320
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                // Tile 1: 3D AI Interviewer (Vladyslav)
                Rectangle {
                    id: interviewerTile
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    radius: Theme.radiusLg
                    color: "#0c111b"
                    border.width: avatarHost.ready ? 1.5 : 1
                    border.color: avatarHost.ready ? Theme.alpha(Theme.blue500, 0.48) : Theme.borderGlass
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        // Top Participant Label
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            PulseDot {
                                color: avatarHost.ready ? Theme.emerald500 : Theme.amber500
                                pingColor: avatarHost.ready ? Theme.emerald400 : Theme.amber400
                                pinging: !avatarHost.ready
                            }
                            AppText {
                                Layout.fillWidth: true
                                text: chatVM.avatarGender === "Female"
                                    ? qsTr("AI Interviewer (Female)")
                                    : qsTr("Vladyslav · AI Interviewer")
                                font.pixelSize: Typography.xs
                                font.weight: Typography.semibold
                                color: Theme.zinc200
                                elide: Text.ElideRight
                            }
                            Badge {
                                tone: "blue"
                                mono: true
                                text: chatVM.avatarEmotion
                                fontSize: Typography.micro
                            }
                        }

                        // Native WebView2 3D Face Slot
                        Rectangle {
                            id: avatarSlot
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: Theme.radiusMd
                            color: "#0c111b"
                            border.width: 1
                            border.color: Theme.borderFaint
                            clip: true

                            onXChanged: Qt.callLater(page.syncAvatarViewport)
                            onYChanged: Qt.callLater(page.syncAvatarViewport)
                            onWidthChanged: Qt.callLater(page.syncAvatarViewport)
                            onHeightChanged: Qt.callLater(page.syncAvatarViewport)

                            ColumnLayout {
                                anchors.centerIn: parent
                                visible: !avatarHost.ready
                                spacing: 10

                                Spinner {
                                    Layout.alignment: Qt.AlignHCenter
                                    size: 26
                                }
                                AppText {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: backendVM.isOffline
                                        ? qsTr("Starting backend & 3D Studio...")
                                        : (avatarHost.available ? qsTr("Loading 3D Avatar...") : qsTr("Connecting 3D Engine..."))
                                    color: Theme.textMuted
                                    font.pixelSize: Typography.xs
                                }
                            }
                        }
                    }
                }

                // Tile 2: Candidate Self-View (Your Icon + Name, or Live Webcam when enabled)
                Rectangle {
                    id: candidateTile
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: page.chatOpen ? 0.72 : 1
                    radius: Theme.radiusLg
                    color: "#0b0e14"
                    border.width: 1
                    border.color: webcamService.active && webcamService.hasFrame
                        ? Theme.alpha(Theme.emerald500, 0.45)
                        : (!chatVM.isBusy && !chatVM.awaitingNextAction
                            ? Theme.alpha(Theme.blue500, 0.35)
                            : Theme.borderGlass)
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#121726" }
                            GradientStop { position: 1.0; color: "#090b10" }
                        }
                    }

                    // Live mirrored webcam feed when activated
                    Image {
                        id: webcamPreview
                        anchors.fill: parent
                        anchors.margins: 2
                        visible: webcamService.active && webcamService.hasFrame
                        source: webcamService.active ? ("image://webcam/frame?" + webcamService.frameRevision) : ""
                        cache: false
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        transform: Scale { origin.x: webcamPreview.width / 2; xScale: -1 }
                    }

                    // Overlay top label & status when camera is on or errored
                    RowLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 10
                        spacing: 6

                        Badge {
                            tone: webcamService.active ? "emerald" : "neutral"
                            mono: true
                            text: webcamService.active ? qsTr("CAM LIVE") : qsTr("CANDIDATE")
                            fontSize: Typography.micro
                        }

                        Item { Layout.fillWidth: true }

                        Badge {
                            visible: webcamService.statusText !== "" && !(webcamService.active && webcamService.hasFrame)
                            tone: webcamService.active ? "amber" : "red"
                            mono: true
                            text: webcamService.statusText
                            fontSize: Typography.micro
                        }

                        // Minimal green-to-red privacy & live visual score + emotion + scoring toggle pill
                        Rectangle {
                            id: camPrivacyPill
                            visible: webcamService.active && webcamService.hasFrame
                            readonly property color scoreColor: page.visualHeatColor(webcamService.visualScore)
                            implicitWidth: camPrivacyRow.implicitWidth + 16
                            implicitHeight: 24
                            radius: 12
                            color: Theme.alpha("#090b10", 0.82)
                            border.width: 1
                            border.color: Theme.alpha(scoreColor, 0.60)

                            MouseArea {
                                id: camPrivacyClick
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: webcamService.includeInScore = !webcamService.includeInScore
                            }

                            RowLayout {
                                id: camPrivacyRow
                                anchors.centerIn: parent
                                spacing: 5

                                Rectangle {
                                    implicitWidth: 8
                                    implicitHeight: 8
                                    radius: 4
                                    color: camPrivacyPill.scoreColor
                                    Behavior on color { ColorAnimation { duration: 220 } }
                                }

                                MonoText {
                                    text: webcamService.visualScore
                                    color: Theme.zinc100
                                    font.pixelSize: Typography.micro
                                    font.weight: Typography.bold
                                }

                                AppText {
                                    text: "· " + webcamService.detectedEmotionLabel
                                    color: Theme.zinc300
                                    font.pixelSize: Typography.micro
                                }

                                Icon {
                                    name: webcamService.includeInScore ? "shield-check" : "eye"
                                    size: 11
                                    color: webcamService.includeInScore ? camPrivacyPill.scoreColor : Theme.zinc400
                                }
                            }

                            AppToolTip {
                                visible: camPrivacyClick.containsMouse
                                text: chatVM.sessionLanguage === "uk"
                                    ? "Локальний аналіз (%1 FPS + %2): кадри не виходять за межі ПК\nЗагалом: %3/100 (%9) · Наявність: %4 · Руки: %5 · Світло: %6 · Вигляд: %7 · Постава: %8 · Емоції: %10\nВрахування камери в балі відповіді: %11 (натисніть щоб перемкнути)"
                                        .arg(webcamService.visionFps)
                                        .arg(webcamService.visionModelName)
                                        .arg(webcamService.visualScore)
                                        .arg(webcamService.presenceScore)
                                        .arg(webcamService.handsScore)
                                        .arg(webcamService.lightingScore)
                                        .arg(webcamService.appearanceScore)
                                        .arg(webcamService.postureScore)
                                        .arg(webcamService.detectedEmotionLabel)
                                        .arg(webcamService.emotionScore)
                                        .arg(webcamService.includeInScore ? "УВІМК. (15%)" : "ВИМК.")
                                    : qsTr("Local privacy-first analysis (%1 FPS + %2): frames stay on device\nOverall: %3/100 (%9) · Presence: %4 · Hands: %5 · Light: %6 · Look: %7 · Posture: %8 · Emotion: %10\nInclude camera in answer score: %11 (click to toggle)")
                                        .arg(webcamService.visionFps)
                                        .arg(webcamService.visionModelName)
                                        .arg(webcamService.visualScore)
                                        .arg(webcamService.presenceScore)
                                        .arg(webcamService.handsScore)
                                        .arg(webcamService.lightingScore)
                                        .arg(webcamService.appearanceScore)
                                        .arg(webcamService.postureScore)
                                        .arg(webcamService.detectedEmotionLabel)
                                        .arg(webcamService.emotionScore)
                                        .arg(webcamService.includeInScore ? "ON (15%)" : "OFF")
                            }
                        }
                    }

                    // User Icon + Name (always visible cleanly when webcam is off!)
                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: !(webcamService.active && webcamService.hasFrame)
                        spacing: 10

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: page.chatOpen ? 78 : 92
                            Layout.preferredHeight: page.chatOpen ? 78 : 92
                            radius: Theme.radiusLg
                            color: Theme.alpha(Theme.blue600, 0.14)
                            border.width: 1.5
                            border.color: Theme.alpha(Theme.blue500, 0.35)

                            UserAvatar {
                                anchors.centerIn: parent
                                size: page.chatOpen ? 62 : 76
                                glyphSize: page.chatOpen ? 30 : 36
                                avatarUrl: settingsVM.avatarUrl
                                highlighted: true
                            }
                        }

                        AppText {
                            Layout.alignment: Qt.AlignHCenter
                            text: settingsVM.displayName
                            font.pixelSize: page.chatOpen ? Typography.base : Typography.lg
                            font.weight: Typography.bold
                            color: Theme.zinc100
                        }

                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: "%1 · %2".arg(chatVM.currentDirection.toUpperCase())
                                          .arg(chatVM.currentLevel.charAt(0).toUpperCase() + chatVM.currentLevel.slice(1))
                            color: Theme.zinc400
                            font.pixelSize: Typography.xs
                        }
                    }

                    // Minimal floating name badge when live webcam is visible
                    Rectangle {
                        visible: webcamService.active && webcamService.hasFrame
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.margins: 10
                        implicitWidth: camNameLabel.implicitWidth + 16
                        implicitHeight: 26
                        radius: Theme.radiusSm
                        color: Theme.alpha("#090b10", 0.72)
                        border.width: 1
                        border.color: Theme.borderFaint

                        AppText {
                            id: camNameLabel
                            anchors.centerIn: parent
                            text: settingsVM.displayName
                            font.pixelSize: Typography.xs
                            font.weight: Typography.medium
                            color: Theme.zinc100
                        }
                    }
                }
            }

            // Zoom Bottom Toolbar (Mute / Stop / VOL / Sound Mode + Sound Cycler / Webcam / Open Chat)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 52
                Layout.preferredHeight: 52
                radius: Theme.radiusLg
                color: "#11151f"
                border.width: 1
                border.color: Theme.borderGlass

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    TactileButton {
                        variant: chatVM.avatarMuted ? "danger" : "secondary"
                        text: chatVM.avatarMuted ? qsTr("Muted") : qsTr("Mute")
                        fontSize: Typography.xs
                        leftPadding: 10
                        rightPadding: 10
                        implicitHeight: 34
                        onClicked: chatVM.toggleAvatarMute()
                    }

                    TactileButton {
                        variant: "secondary"
                        text: qsTr("Stop")
                        fontSize: Typography.xs
                        leftPadding: 10
                        rightPadding: 10
                        implicitHeight: 34
                        onClicked: chatVM.stopAvatarSpeech()
                    }

                    MonoText {
                        text: qsTr("VOL")
                        color: Theme.textMuted
                        font.pixelSize: Typography.micro
                    }

                    Slider {
                        id: volumeSlider
                        Layout.preferredWidth: 96
                        Layout.maximumWidth: 140
                        from: 0.0
                        to: 1.0
                        stepSize: 0.05
                        value: chatVM.avatarVolume
                        opacity: chatVM.avatarMuted ? 0.45 : 1.0
                        onMoved: chatVM.setAvatarVolume(value)

                        background: Rectangle {
                            x: volumeSlider.leftPadding
                            y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                            width: volumeSlider.availableWidth
                            height: 5
                            radius: 3
                            color: Theme.zinc800

                            Rectangle {
                                width: volumeSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 3
                                color: chatVM.avatarMuted ? Theme.zinc500 : Theme.blue500
                            }
                        }

                        handle: Rectangle {
                            x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                            y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                            width: 14
                            height: 14
                            radius: 7
                            color: volumeSlider.pressed ? Theme.blue400 : Theme.zinc100
                            border.width: 1
                            border.color: Theme.blue500
                        }
                    }

                    MonoText {
                        Layout.preferredWidth: 32
                        text: Math.round(chatVM.avatarVolume * 100) + "%"
                        color: Theme.zinc300
                        font.pixelSize: Typography.micro
                    }

                    TactileButton {
                        id: beepToggleBtn
                        variant: chatVM.avatarOutputMode === "sound" ? "primary" : "secondary"
                        text: chatVM.avatarOutputMode === "sound"
                            ? (chatVM.sessionLanguage === "uk" ? "Звук ✓" : qsTr("Sounds ✓"))
                            : (chatVM.sessionLanguage === "uk" ? "Звуки" : qsTr("Sounds"))
                        fontSize: Typography.xs
                        leftPadding: 10
                        rightPadding: 10
                        implicitHeight: 34
                        onClicked: chatVM.toggleAvatarOutputMode()
                        AppToolTip {
                            visible: beepToggleBtn.hovered
                            text: chatVM.sessionLanguage === "uk"
                                ? "Перемкнути: жива мова ↔ звуки-затички під губи"
                                : qsTr("Toggle Voice speech ↔ lip-synced sound effects")
                        }
                    }

                    TactileButton {
                        id: soundCycleBtn
                        visible: chatVM.avatarOutputMode === "sound"
                        variant: "soft"
                        text: page.activeSoundShortName()
                        fontSize: Typography.micro
                        leftPadding: 10
                        rightPadding: 10
                        implicitHeight: 32
                        onClicked: chatVM.cycleAvatarSoundEffect()
                        AppToolTip {
                            visible: soundCycleBtn.hovered
                            text: qsTr("Click to cycle sound effect")
                        }
                    }

                    Item { Layout.fillWidth: true }

                    TactileButton {
                        id: micMuteBtn
                        variant: micService.muted ? "danger" : "secondary"
                        iconLeft: micService.muted ? "mic-off" : "mic"
                        iconSize: 15
                        implicitWidth: 34
                        implicitHeight: 34
                        leftPadding: 0
                        rightPadding: 0
                        onClicked: micService.toggleMuted()
                        AppToolTip {
                            visible: micMuteBtn.hovered
                            text: micService.muted
                                ? (chatVM.sessionLanguage === "uk" ? "Розмутити мікрофон (%1)".arg(settingsVM.micMuteShortcut) : qsTr("Unmute mic (%1)").arg(settingsVM.micMuteShortcut))
                                : (chatVM.sessionLanguage === "uk" ? "Заглушити мікрофон (%1)".arg(settingsVM.micMuteShortcut) : qsTr("Mute mic (%1)").arg(settingsVM.micMuteShortcut))
                        }
                    }

                    TactileButton {
                        id: micToolbarBtn
                        variant: micService.recording ? "danger" : (micService.transcribing ? "soft" : "secondary")
                        busy: micService.transcribing
                        iconLeft: (micService.muted || !micService.permissionGranted || !micService.osPermissionAllowed) ? "mic-off" : "mic"
                        iconSize: 15
                        text: micService.muted
                            ? (chatVM.sessionLanguage === "uk" ? "Мікрофон (Mute)" : qsTr("Mic (Muted)"))
                            : (micService.recording
                                ? (settingsVM.micMode === "toggle"
                                    ? (chatVM.sessionLanguage === "uk" ? "Стоп & Надіслати" : qsTr("Stop & Send"))
                                    : (chatVM.sessionLanguage === "uk" ? "Говоріть… (відпустіть)" : qsTr("Speaking… (Release)")))
                                : (micService.transcribing
                                    ? "Parakeet..."
                                    : (settingsVM.micMode === "toggle"
                                        ? (chatVM.sessionLanguage === "uk" ? "Натисніть · Голос" : qsTr("Toggle · Mic"))
                                        : (chatVM.sessionLanguage === "uk" ? "Утримуйте · Голос" : qsTr("Hold · Mic")))))
                        fontSize: Typography.xs
                        leftPadding: 12
                        rightPadding: 12
                        implicitHeight: 34
                        down: toolbarMicHold.pressed
                        enabled: !chatVM.isBusy && !chatVM.isCompleting && !micService.transcribing

                        MouseArea {
                            id: toolbarMicHold
                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onPressed: {
                                if (micService.muted) {
                                    micService.setMuted(false)
                                } else {
                                    page.handleMicPress()
                                }
                            }
                            onReleased: page.handleMicRelease()
                            onCanceled: page.handleMicRelease()
                        }

                        AppToolTip {
                            visible: toolbarMicHold.containsMouse
                            text: micService.muted
                                ? (chatVM.sessionLanguage === "uk" ? "Натисніть щоб зняти мут мікрофона (%1)".arg(settingsVM.micMuteShortcut) : qsTr("Click to unmute mic (%1)").arg(settingsVM.micMuteShortcut))
                                : (micService.recording
                                    ? (settingsVM.micMode === "toggle"
                                        ? (chatVM.sessionLanguage === "uk" ? "Натисніть щоб завершити запис і надіслати (%1)".arg(settingsVM.micTalkShortcut) : qsTr("Click to stop & send (%1)").arg(settingsVM.micTalkShortcut))
                                        : (chatVM.sessionLanguage === "uk" ? "Відпустіть щоб надіслати відповідь моделі" : qsTr("Release to send answer to AI model")))
                                    : (settingsVM.micMode === "toggle"
                                        ? (chatVM.sessionLanguage === "uk" ? "Режим Toggle: натисніть (%1) щоб почати говорити".arg(settingsVM.micTalkShortcut) : qsTr("Toggle mode: click (%1) to start speaking").arg(settingsVM.micTalkShortcut))
                                        : (chatVM.sessionLanguage === "uk" ? "Утримуйте кнопку чи %1 поки говорите".arg(settingsVM.micTalkShortcut) : qsTr("Hold button or %1 while speaking").arg(settingsVM.micTalkShortcut))))
                        }
                    }

                    TactileButton {
                        id: webcamBtn
                        variant: webcamService.active ? "success" : "secondary"
                        iconLeft: webcamService.active ? "video" : "video-off"
                        iconSize: 15
                        text: chatVM.sessionLanguage === "uk" ? "Камера" : qsTr("Camera")
                        fontSize: Typography.xs
                        leftPadding: 12
                        rightPadding: 12
                        implicitHeight: 34
                        onClicked: page.triggerCameraAction()
                        AppToolTip {
                            visible: webcamBtn.hovered
                            text: (webcamService.selectedCameraName.length > 0 ? webcamService.selectedCameraName : (chatVM.sessionLanguage === "uk" ? "Веб-камера" : qsTr("Webcam"))) + " · " + settingsVM.cameraToggleShortcut
                        }
                    }

                    TactileButton {
                        id: devicesBtn
                        variant: (!micService.permissionGranted || !webcamService.permissionGranted ||
                                  !micService.osPermissionAllowed || !webcamService.osPermissionAllowed)
                            ? "soft"
                            : "secondary"
                        iconLeft: "sliders-horizontal"
                        iconSize: 14
                        text: chatVM.sessionLanguage === "uk" ? "Пристрої" : qsTr("Devices")
                        fontSize: Typography.xs
                        leftPadding: 10
                        rightPadding: 10
                        implicitHeight: 34
                        onClicked: {
                            deviceSettings.pendingAction = ""
                            deviceSettings.open = true
                        }
                        AppToolTip {
                            visible: devicesBtn.hovered
                            text: chatVM.sessionLanguage === "uk"
                                ? "Вибір мікрофона, камери та дозволи (Meet / Teams)"
                                : qsTr("Select microphone, webcam and permissions")
                        }
                    }

                    TactileButton {
                        id: chatToggleBtn
                        variant: page.chatOpen ? "secondary" : "primary"
                        glow: !page.chatOpen
                        text: page.chatOpen
                            ? (chatVM.sessionLanguage === "uk" ? "Сховати чат" : qsTr("Hide chat"))
                            : (chatVM.sessionLanguage === "uk" ? "Чат / Відповісти" : qsTr("Chat / Answer"))
                        iconLeft: "bot"
                        iconSize: 15
                        fontSize: Typography.xs
                        leftPadding: 14
                        rightPadding: 14
                        implicitHeight: 36
                        onClicked: page.chatOpen = !page.chatOpen
                    }
                }
            }
        }
    }

    DeviceSettingsOverlay {
        id: deviceSettings
        isUk: chatVM.sessionLanguage === "uk"
        onOpenChanged: {
            Qt.callLater(page.syncAvatarViewport)
            if (!open) {
                pendingAction = ""
                micService.stopMicTest()
            }
        }
        onApprovedAction: (action) => {
            if (action === "cam") {
                page.triggerCameraAction()
            }
        }
    }
}
