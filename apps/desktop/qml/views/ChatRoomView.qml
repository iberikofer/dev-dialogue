import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: chatView

    function submitAnswer() {
        const value = inputArea.text.trim()
        if (value.length > 0 && !chatVM.isBusy) {
            chatVM.sendAnswer(value)
            inputArea.text = ""
        }
    }

    onVisibleChanged: {
        if (visible && navVM.activeInterviewId !== ""
                && chatVM.interviewId !== navVM.activeInterviewId)
            chatVM.loadInterview(navVM.activeInterviewId)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // ---------------------------------------------------------------
        // Session bar
        // ---------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            radius: Theme.radiusMd
            color: Theme.backgroundSecondary
            border.width: 1
            border.color: Theme.borderGlass

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 10

                Text {
                    text: qsTr("Interview session")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.body
                    font.weight: Typography.weightSemibold
                    color: Theme.textPrimary
                }

                Badge {
                    text: Labels.directionLabel(chatVM.currentDirection).toUpperCase()
                    toneColor: Theme.accentRose
                }

                Badge {
                    text: Labels.levelLabel(chatVM.currentLevel)
                    toneColor: Theme.accentIndigo
                }

                Badge {
                    text: Labels.languageLabel(chatVM.currentLanguage)
                    toneColor: Theme.accentTeal
                }

                Item { Layout.fillWidth: true }

                Spinner {
                    visible: chatVM.isBusy
                    diameter: 18
                }

                Text {
                    text: qsTr("Answers: %1").arg(chatVM.candidateAnswersCount)
                    font.family: Theme.fontMono
                    font.pixelSize: Typography.small
                    color: Theme.textSecondary
                }

                TactileButton {
                    implicitWidth: 130
                    implicitHeight: 34
                    buttonRadius: Theme.radiusSm
                    outlined: true
                    text: qsTr("Leave")
                    onClicked: chatVM.leaveSession()
                }

                TactileButton {
                    implicitWidth: 178
                    implicitHeight: 34
                    buttonRadius: Theme.radiusSm
                    text: qsTr("Complete interview")
                    bgGradientStart: Theme.accentEmerald
                    bgGradientEnd: Theme.accentTeal
                    enabled: chatVM.candidateAnswersCount > 0 && !chatVM.isBusy
                    onClicked: chatVM.completeInterview()
                }
            }
        }

        ErrorBanner {
            Layout.fillWidth: true
            errorMessage: chatVM.errorMessage
            onDismissed: chatVM.clearError()
        }

        // ---------------------------------------------------------------
        // Transcript
        // ---------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messageList
                anchors.fill: parent
                clip: true
                spacing: 14
                model: chatVM.messagesModel
                boundsBehavior: Flickable.StopAtBounds

                delegate: MessageBubble {
                    width: ListView.view ? ListView.view.width : 0
                    senderRole: model.role
                    content: model.content
                    timestamp: model.timestamp
                }

                onCountChanged: Qt.callLater(() => messageList.positionViewAtEnd())

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 12
                visible: chatVM.messagesModel.count === 0 && chatVM.isBusy

                Spinner {
                    anchors.horizontalCenter: parent.horizontalCenter
                    diameter: 28
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Preparing your first question…")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.small
                    color: Theme.textMuted
                }
            }

            EmptyState {
                anchors.fill: parent
                visible: chatVM.messagesModel.count === 0 && !chatVM.isBusy
                iconSource: Icons.bot
                title: qsTr("No active session")
                description: qsTr("Start a new interview to open the chat room and meet your AI interviewer.")
                actionText: qsTr("Set up an interview")
                onActionClicked: navVM.navigateToSetup()
            }
        }

        // ---------------------------------------------------------------
        // Answer composer
        // ---------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            radius: Theme.radiusLg
            color: Theme.backgroundSecondary
            border.width: 1
            border.color: inputArea.activeFocus ? Theme.accentIndigo
                                                : Theme.borderSubtle

            Behavior on border.color {
                ColorAnimation { duration: Theme.durationFast }
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                TextArea {
                    id: inputArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: qsTr("Type your answer… (Enter to send, Shift+Enter for a new line)")
                    placeholderTextColor: Theme.textMuted
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.body
                    color: Theme.textPrimary
                    selectionColor: Theme.accentIndigo
                    selectedTextColor: "#ffffff"
                    wrapMode: Text.Wrap
                    background: null
                    leftPadding: 4
                    topPadding: 4

                    Keys.onReturnPressed: (event) => {
                        if (!(event.modifiers & Qt.ShiftModifier)) {
                            event.accepted = true
                            chatView.submitAnswer()
                        }
                    }
                }

                TactileButton {
                    implicitWidth: 92
                    implicitHeight: 38
                    text: qsTr("Send")
                    enabled: inputArea.text.trim().length > 0 && !chatVM.isBusy
                    onClicked: chatView.submitAnswer()
                }
            }
        }
    }
}
