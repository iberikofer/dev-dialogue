pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Live developer debug inspector (Delete / F12 / Ctrl+D / Debug button):
// shows active database state, AI engine, 3D Avatar voice/sound/wardrobe studio,
// live trace and full question pool without overflowing or overlapping.
ModalOverlay {
    id: root

    dialogWidth: dockRight ? Math.min(580, Math.max(460, Math.round(root.width * 0.44))) : 860
    dialogPadding: 20
    backdropOpacity: dockRight ? 0.25 : 0.8

    onOpenChanged: {
        if (open) {
            backendVM.refreshDebugInfo()
        }
    }

    readonly property var db: backendVM.debugDatabase || ({})
    readonly property var ai: backendVM.debugAi || ({})

    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(root.maxBodyHeight, headerBar.implicitHeight + 12 + debugContentCol.implicitHeight)
        spacing: 12

        // ------------------------------------------------------------- Fixed Header
        RowLayout {
            id: headerBar
            Layout.fillWidth: true
            spacing: 8

            Icon { name: "code-xml"; size: 17; color: Theme.blue400 }

            AppText {
                Layout.fillWidth: true
                Layout.minimumWidth: 80
                text: root.dockRight ? qsTr("Debug Inspector") : qsTr("Debug Inspector — DB & Avatar Studio")
                font.pixelSize: Typography.base
                font.weight: Typography.bold
                elide: Text.ElideRight
            }

            Badge {
                tone: root.db.encrypted ? "emerald" : "amber"
                mono: true
                text: root.db.encrypted ? "SQLCipher AES-256" : (root.db.mode || "Offline")
                fontSize: Typography.micro
            }

            TactileButton {
                variant: "secondary"
                text: backendVM.isDebugLoading ? qsTr("...") : qsTr("Refresh")
                iconLeft: "refresh-cw"
                iconSize: 12
                fontSize: Typography.xs
                leftPadding: 10
                rightPadding: 10
                implicitHeight: 30
                enabled: !backendVM.isDebugLoading
                onClicked: backendVM.refreshDebugInfo()
            }

            TactileButton {
                variant: "ghost"
                text: qsTr("Close")
                fontSize: Typography.xs
                leftPadding: 10
                rightPadding: 10
                implicitHeight: 30
                onClicked: root.closeRequested()
            }
        }

        // ------------------------------------------------------------- Scrollable Body
        Flickable {
            id: debugScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: debugContentCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ThemedScrollBar {}

            ColumnLayout {
                id: debugContentCol
                width: debugScroll.width - 12
                spacing: 14

                // ------------------------------------------- DB & AI Summary
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        implicitHeight: dbCol.implicitHeight + 20
                        radius: Theme.radiusMd
                        color: Theme.zinc900
                        border.width: 1
                        border.color: Theme.borderGlass

                        ColumnLayout {
                            id: dbCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            MonoText { caps: true; text: "DATABASE"; color: Theme.textMuted; font.pixelSize: Typography.micro }
                            AppText {
                                Layout.fillWidth: true
                                text: backendVM.isOffline
                                    ? qsTr("Server offline (demo fallback)")
                                    : ("%1 · CanConnect: %2".arg(root.db.provider || "-").arg(root.db.canConnect ? "true" : "false"))
                                font.pixelSize: Typography.xs
                                font.weight: Typography.medium
                                elide: Text.ElideRight
                            }
                            MonoText {
                                Layout.fillWidth: true
                                text: "Pool: %1 | Interviews: %2 | Users: %3"
                                    .arg(root.db.questionsCount !== undefined ? root.db.questionsCount : 0)
                                    .arg(root.db.interviewsCount !== undefined ? root.db.interviewsCount : 0)
                                    .arg(root.db.usersCount !== undefined ? root.db.usersCount : 0)
                                color: Theme.emerald400
                                font.pixelSize: Typography.tiny
                                elide: Text.ElideRight
                            }
                            MonoText {
                                Layout.fillWidth: true
                                text: root.db.sqliteFile || ""
                                visible: text !== ""
                                color: Theme.zinc400
                                font.pixelSize: Typography.micro
                                elide: Text.ElideMiddle
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        implicitHeight: aiCol.implicitHeight + 20
                        radius: Theme.radiusMd
                        color: Theme.zinc900
                        border.width: 1
                        border.color: Theme.borderGlass

                        ColumnLayout {
                            id: aiCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            MonoText { caps: true; text: "AI & SESSION"; color: Theme.textMuted; font.pixelSize: Typography.micro }
                            AppText {
                                Layout.fillWidth: true
                                text: "%1 (%2)".arg(root.ai.provider || "Mock").arg(root.ai.model || "-")
                                font.pixelSize: Typography.xs
                                font.weight: Typography.medium
                                elide: Text.ElideRight
                            }
                            MonoText {
                                Layout.fillWidth: true
                                text: "%1/%2 (%3) | Awaiting: %4"
                                    .arg(chatVM.currentDirection || "-")
                                    .arg(chatVM.currentLevel || "-")
                                    .arg(chatVM.sessionLanguage || "-")
                                    .arg(chatVM.awaitingNextAction ? "true" : "false")
                                color: Theme.blue400
                                font.pixelSize: Typography.tiny
                                elide: Text.ElideRight
                            }
                            MonoText {
                                Layout.fillWidth: true
                                text: "Shortcut: Delete / F12 / Ctrl+D"
                                color: Theme.zinc400
                                font.pixelSize: Typography.micro
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                // ------------------------------------------- 3D Avatar Voice, Sound & Wardrobe Studio
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: avatarStudioCol.implicitHeight + 22
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderGlass

                    ColumnLayout {
                        id: avatarStudioCol
                        anchors.fill: parent
                        anchors.margins: 11
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            MonoText {
                                Layout.fillWidth: true
                                caps: true
                                text: qsTr("3D Avatar Studio — Voice, Beeps & Wardrobe")
                                color: Theme.textMuted
                                font.pixelSize: Typography.micro
                                elide: Text.ElideRight
                            }
                            Badge {
                                tone: avatarHost.ready ? "emerald" : "amber"
                                mono: true
                                text: avatarHost.ready ? "WebView2 Ready" : "Standby"
                                fontSize: Typography.micro
                            }
                        }

                        // Row 1: Output Mode (Voice vs Blip Sound Stub) + Sound Effect Selector
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.preferredWidth: 185
                                spacing: 4
                                MonoText { text: qsTr("Audio Mode"); color: Theme.zinc400; font.pixelSize: Typography.micro }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    TactileButton {
                                        variant: chatVM.avatarOutputMode === "voice" ? "primary" : "secondary"
                                        text: qsTr("Voice (TTS)")
                                        fontSize: Typography.xs
                                        leftPadding: 8
                                        rightPadding: 8
                                        implicitHeight: 36
                                        Layout.fillWidth: true
                                        onClicked: chatVM.setAvatarOutputMode("voice")
                                    }
                                    TactileButton {
                                        variant: chatVM.avatarOutputMode === "sound" ? "primary" : "secondary"
                                        text: qsTr("Beeps")
                                        fontSize: Typography.xs
                                        leftPadding: 8
                                        rightPadding: 8
                                        implicitHeight: 36
                                        Layout.fillWidth: true
                                        onClicked: chatVM.setAvatarOutputMode("sound")
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                MonoText { text: qsTr("Blip / Sound Effect"); color: Theme.zinc400; font.pixelSize: Typography.micro }
                                SelectBox {
                                    Layout.fillWidth: true
                                    textRole: "label"
                                    valueRole: "id"
                                    model: chatVM.availableSoundEffects
                                    currentIndex: {
                                        const list = chatVM.availableSoundEffects
                                        for (let i = 0; i < list.length; ++i) {
                                            if (list[i].id === chatVM.avatarSoundEffect) return i
                                        }
                                        return 0
                                    }
                                    onActivated: (idx) => {
                                        const list = chatVM.availableSoundEffects
                                        if (idx >= 0 && idx < list.length) {
                                            chatVM.setAvatarSoundEffect(list[idx].id)
                                            chatVM.setAvatarOutputMode("sound")
                                        }
                                    }
                                }
                            }
                        }

                        // Row 2: Neural Voice + Appearance + Model Gender
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                MonoText { text: qsTr("Neural Voice"); color: Theme.zinc400; font.pixelSize: Typography.micro }
                                SelectBox {
                                    Layout.fillWidth: true
                                    textRole: "label"
                                    valueRole: "id"
                                    model: chatVM.availableVoices
                                    currentIndex: {
                                        const list = chatVM.availableVoices
                                        for (let i = 0; i < list.length; ++i) {
                                            if (list[i].id === chatVM.avatarVoice) return i
                                        }
                                        return 0
                                    }
                                    onActivated: (idx) => {
                                        const list = chatVM.availableVoices
                                        if (idx >= 0 && idx < list.length) {
                                            chatVM.setAvatarVoice(list[idx].id)
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 136
                                spacing: 4
                                MonoText { text: qsTr("Appearance"); color: Theme.zinc400; font.pixelSize: Typography.micro }
                                SelectBox {
                                    Layout.fillWidth: true
                                    textRole: "label"
                                    valueRole: "id"
                                    model: chatVM.availablePresets
                                    currentIndex: {
                                        const list = chatVM.availablePresets
                                        for (let i = 0; i < list.length; ++i) {
                                            if (list[i].id === chatVM.avatarPreset) return i
                                        }
                                        return 0
                                    }
                                    onActivated: (idx) => {
                                        const list = chatVM.availablePresets
                                        if (idx >= 0 && idx < list.length) {
                                            chatVM.setAvatarPreset(list[idx].id)
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 128
                                spacing: 4
                                MonoText { text: qsTr("Model"); color: Theme.zinc400; font.pixelSize: Typography.micro }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    TactileButton {
                                        variant: chatVM.avatarGender === "Male" ? "primary" : "secondary"
                                        text: qsTr("Male")
                                        fontSize: Typography.xs
                                        leftPadding: 8
                                        rightPadding: 8
                                        implicitHeight: 36
                                        Layout.fillWidth: true
                                        onClicked: chatVM.setAvatarGender("Male")
                                    }
                                    TactileButton {
                                        variant: chatVM.avatarGender === "Female" ? "primary" : "secondary"
                                        text: qsTr("Female")
                                        fontSize: Typography.xs
                                        leftPadding: 8
                                        rightPadding: 8
                                        implicitHeight: 36
                                        Layout.fillWidth: true
                                        onClicked: chatVM.setAvatarGender("Female")
                                    }
                                }
                            }
                        }

                        // Clothing & Accessories
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            MonoText { text: qsTr("Clothing & Accessories"); color: Theme.zinc400; font.pixelSize: Typography.micro }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 6

                                TactileButton {
                                    variant: chatVM.glassesVisible ? "primary" : "secondary"
                                    text: qsTr("Glasses")
                                    fontSize: Typography.xs
                                    leftPadding: 12; rightPadding: 12
                                    implicitHeight: 30
                                    onClicked: chatVM.toggleAccessory("Glasses")
                                }
                                TactileButton {
                                    variant: chatVM.tieVisible ? "primary" : "secondary"
                                    text: qsTr("Tie")
                                    fontSize: Typography.xs
                                    leftPadding: 12; rightPadding: 12
                                    implicitHeight: 30
                                    onClicked: chatVM.toggleAccessory("Tie")
                                }
                                TactileButton {
                                    variant: chatVM.hatVisible ? "primary" : "secondary"
                                    text: qsTr("Hat")
                                    fontSize: Typography.xs
                                    leftPadding: 12; rightPadding: 12
                                    implicitHeight: 30
                                    onClicked: chatVM.toggleAccessory("Hat")
                                }
                                TactileButton {
                                    variant: chatVM.hornsVisible ? "primary" : "secondary"
                                    text: qsTr("Horns")
                                    fontSize: Typography.xs
                                    leftPadding: 12; rightPadding: 12
                                    implicitHeight: 30
                                    onClicked: chatVM.toggleAccessory("Horns")
                                }
                                TactileButton {
                                    variant: chatVM.haloVisible ? "primary" : "secondary"
                                    text: qsTr("Halo")
                                    fontSize: Typography.xs
                                    leftPadding: 12; rightPadding: 12
                                    implicitHeight: 30
                                    onClicked: chatVM.toggleAccessory("Halo")
                                }
                            }
                        }

                        // Test Expressions
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            MonoText { text: qsTr("Test Expressions"); color: Theme.zinc400; font.pixelSize: Typography.micro }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 6

                                Repeater {
                                    model: ["smile", "laugh", "thinking", "skeptical", "surprise", "anger", "wink"]
                                    TactileButton {
                                        required property string modelData
                                        variant: chatVM.avatarEmotion === modelData ? "primary" : "secondary"
                                        text: modelData.charAt(0).toUpperCase() + modelData.slice(1)
                                        fontSize: Typography.micro
                                        leftPadding: 10; rightPadding: 10
                                        implicitHeight: 28
                                        onClicked: chatVM.triggerTestEmotion(modelData)
                                    }
                                }
                            }
                        }
                    }
                }

                // ------------------------------------------- Visual Camera Evaluation
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: visionCol.implicitHeight + 24
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: webcamService.active && webcamService.hasFrame
                        ? Theme.alpha(Theme.emerald500, 0.35)
                        : Theme.borderFaint

                    ColumnLayout {
                        id: visionCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Icon { name: "video"; size: 15; color: Theme.blue400 }
                            AppText {
                                Layout.fillWidth: true
                                text: qsTr("Оцінка візуалу (Камера · 30 FPS CV + %1)").arg(webcamService.visionModelName)
                                font.pixelSize: Typography.xs
                                font.weight: Typography.bold
                            }
                            Badge {
                                tone: webcamService.personPresent ? "blue" : "rose"
                                mono: true
                                text: webcamService.detectedEmotionLabel
                                fontSize: Typography.micro
                            }
                            Badge {
                                tone: webcamService.visualScore >= 75 ? "emerald" : (webcamService.visualScore >= 45 ? "amber" : "rose")
                                mono: true
                                text: webcamService.visualScore + "/100"
                                fontSize: Typography.micro
                            }
                            MonoText {
                                text: "%1 FPS · %2 VLM".arg(webcamService.visionFps).arg(webcamService.visionSamplesCount)
                                color: Theme.zinc400
                                font.pixelSize: Typography.micro
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: 10
                            rowSpacing: 6

                            Repeater {
                                model: [
                                    { label: qsTr("Наявність"), score: webcamService.presenceScore },
                                    { label: qsTr("Руки"), score: webcamService.handsScore },
                                    { label: qsTr("Освітлення"), score: webcamService.lightingScore },
                                    { label: qsTr("Вигляд"), score: webcamService.appearanceScore },
                                    { label: qsTr("Постава"), score: webcamService.postureScore },
                                    { label: qsTr("Емоції"), score: webcamService.emotionScore }
                                ]
                                ColumnLayout {
                                    id: vCell
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 3

                                    RowLayout {
                                        Layout.fillWidth: true
                                        MonoText {
                                            Layout.fillWidth: true
                                            text: vCell.modelData.label
                                            color: Theme.zinc400
                                            font.pixelSize: Typography.micro
                                            elide: Text.ElideRight
                                        }
                                        MonoText {
                                            text: vCell.modelData.score
                                            color: Theme.scoreColor(vCell.modelData.score)
                                            font.pixelSize: Typography.micro
                                            font.weight: Typography.bold
                                        }
                                    }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 4
                                        radius: 2
                                        color: Theme.zinc800
                                        Rectangle {
                                            width: parent.width * Math.max(0, Math.min(100, vCell.modelData.score)) / 100
                                            height: parent.height
                                            radius: 2
                                            color: Theme.scoreColor(vCell.modelData.score)
                                        }
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            AppText {
                                Layout.fillWidth: true
                                text: qsTr("Враховувати камеру при оцінці відповіді агентом (15% ваги)")
                                color: Theme.zinc300
                                font.pixelSize: Typography.micro
                            }
                            ToggleSwitch {
                                checked: webcamService.includeInScore
                                onToggled: webcamService.includeInScore = !webcamService.includeInScore
                            }
                        }

                        AppText {
                            visible: webcamService.visualSummary.length > 0
                            Layout.fillWidth: true
                            text: webcamService.visualSummary
                            color: Theme.zinc300
                            font.pixelSize: Typography.micro
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // ------------------------------------------- Selection Trace Log
                MonoText {
                    caps: true
                    text: qsTr("Live Selection & Evaluation Trace (%1)").arg(backendVM.debugTrace.length)
                    color: Theme.textMuted
                    font.pixelSize: Typography.micro
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 115
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderFaint
                    clip: true

                    ListView {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6
                        model: backendVM.debugTrace
                        ScrollBar.vertical: ThemedScrollBar {}

                        delegate: RowLayout {
                            id: traceRow
                            required property var modelData
                            width: ListView.view.width - 8
                            spacing: 8

                            Badge {
                                tone: traceRow.modelData.stage.indexOf("Error") >= 0 ? "red"
                                    : (traceRow.modelData.stage.indexOf("DB") >= 0 ? "emerald" : "blue")
                                mono: true
                                text: traceRow.modelData.stage || ""
                                fontSize: Typography.micro
                            }
                            MonoText {
                                Layout.fillWidth: true
                                text: traceRow.modelData.detail || ""
                                color: Theme.zinc300
                                font.pixelSize: Typography.tiny
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                // ------------------------------------------- Full Question Pool
                MonoText {
                    caps: true
                    text: qsTr("Questions Pool in Active DB (%1)").arg(backendVM.debugQuestionsPool.length)
                    color: Theme.textMuted
                    font.pixelSize: Typography.micro
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 190
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderFaint
                    clip: true

                    ListView {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        model: backendVM.debugQuestionsPool
                        ScrollBar.vertical: ThemedScrollBar {}

                        delegate: Rectangle {
                            id: qRow
                            required property var modelData
                            width: ListView.view.width - 8
                            implicitHeight: qCol.implicitHeight + 14
                            radius: Theme.radiusSm
                            color: Theme.alpha(Theme.zinc800, 0.45)
                            border.width: 1
                            border.color: Theme.borderFaint

                            ColumnLayout {
                                id: qCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 8
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Badge { tone: "blue"; mono: true; caps: true; text: qRow.modelData.direction || ""; fontSize: Typography.micro }
                                    Badge { tone: "neutral"; mono: true; text: qRow.modelData.level || ""; fontSize: Typography.micro }
                                    AppText {
                                        Layout.fillWidth: true
                                        text: qRow.modelData.topic || ""
                                        color: Theme.zinc300
                                        font.pixelSize: Typography.xs
                                        font.weight: Typography.medium
                                        elide: Text.ElideRight
                                    }
                                    Badge {
                                        tone: qRow.modelData.hasExpectedAnswer ? "emerald" : "amber"
                                        mono: true
                                        text: qRow.modelData.hasExpectedAnswer ? "Ref ✓" : "No Ref"
                                        fontSize: Typography.micro
                                    }
                                }

                                AppText {
                                    Layout.fillWidth: true
                                    text: qRow.modelData.questionText || ""
                                    color: Theme.zinc400
                                    font.pixelSize: Typography.xs
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
