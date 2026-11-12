pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Interview room (web ChatRoom.tsx): session bar, scrolling transcript,
// "AI is thinking" indicator and the answer box.
Item {
    id: view

    readonly property bool active: StackLayout.isCurrentItem
    onActiveChanged: if (active) input.area.forceActiveFocus()

    function send() {
        const answer = input.text.trim()
        if (answer.length === 0 || chatVM.isBusy || chatVM.isCompleting)
            return
        chatVM.sendAnswer(answer)
        input.text = ""
    }

    ColumnLayout {
        width: Math.min(parent.width - 32, Theme.contentWidthMedium)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        spacing: 12

        // -------------------------------------------------- Session bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                implicitWidth: 40
                implicitHeight: 40
                radius: Theme.radiusMd
                border.width: 1
                border.color: Theme.alpha(Theme.rose500, 0.3)
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.rose500, 0.2)) }
                    GradientStop { position: 0.5; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.indigo500, 0.1)) }
                    GradientStop { position: 1.0; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.blue500, 0.2)) }
                }
                Icon { anchors.centerIn: parent; name: "bot"; size: 20; color: Theme.rose300 }
            }

            ColumnLayout {
                spacing: 2

                RowLayout {
                    spacing: 8
                    AppText {
                        text: qsTr("Technical Interview")
                        font.pixelSize: Typography.base
                        font.weight: Typography.semibold
                    }
                    Badge {
                        tone: "rose"
                        mono: true
                        caps: true
                        text: chatVM.currentDirection
                        fontSize: Typography.tiny
                        fillOpacity: 0.15
                        strokeOpacity: 0.3
                    }
                    Badge {
                        tone: "neutral"
                        mono: true
                        text: chatVM.currentLevel
                        fontSize: Typography.tiny
                        fontWeight: Typography.regular
                    }
                }
                RowLayout {
                    spacing: 8
                    Icon { name: "clock"; size: 14; color: Theme.textMuted }
                    MonoText {
                        text: qsTr("Candidate responses: %1").arg(chatVM.candidateAnswersCount)
                    }
                    MonoText {
                        visible: chatVM.elapsedText !== ""
                        text: "·  " + chatVM.elapsedText
                        color: Theme.textMuted
                    }
                }
            }

            Item { Layout.fillWidth: true }

            TactileButton {
                variant: "success"
                text: chatVM.isCompleting ? qsTr("Generating Report...") : qsTr("Finish Interview")
                iconLeft: chatVM.isCompleting ? "" : "circle-check-big"
                fontSize: Typography.xs
                implicitHeight: 34
                leftPadding: 16
                rightPadding: 16
                enabled: !chatVM.isCompleting && chatVM.candidateAnswersCount > 0
                opacity: enabled ? 1.0 : 0.4
                glow: enabled
                onClicked: chatVM.completeInterview()
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.borderGlass }

        AlertBox {
            visible: chatVM.errorMessage !== ""
            Layout.fillWidth: true
            text: chatVM.errorMessage
        }

        // --------------------------------------------------- Transcript
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radiusLg
            color: Theme.alpha(Theme.zinc950, 0.7)
            border.width: 1
            border.color: Theme.borderGlass

            ListView {
                id: transcript
                anchors.fill: parent
                anchors.margins: 20
                anchors.rightMargin: 8
                clip: true
                spacing: 16
                boundsBehavior: Flickable.StopAtBounds
                model: chatVM.messagesModel
                ScrollBar.vertical: ThemedScrollBar {}

                function scrollToEnd() {
                    Qt.callLater(transcript.positionViewAtEnd)
                }
                onCountChanged: scrollToEnd()

                delegate: Item {
                    id: entry
                    required property string role
                    required property string content
                    required property string timeText

                    width: transcript.width - 12
                    height: bubble.implicitHeight

                    MessageBubble {
                        id: bubble
                        width: parent.width
                        senderRole: entry.role
                        content: entry.content
                        timestamp: entry.timeText
                    }
                }

                // Thinking indicator while the interviewer prepares a reply.
                footer: Item {
                    width: transcript.width - 12
                    height: chatVM.isBusy ? thinking.implicitHeight + 16 : 0
                    visible: chatVM.isBusy
                    onVisibleChanged: transcript.scrollToEnd()

                    RowLayout {
                        id: thinking
                        y: 16
                        spacing: 12

                        Rectangle {
                            id: thinkingAvatar
                            Layout.alignment: Qt.AlignTop
                            implicitWidth: 32
                            implicitHeight: 32
                            radius: Theme.radiusMd
                            color: Theme.zinc900
                            border.width: 1
                            border.color: Theme.borderGlass
                            Icon { anchors.centerIn: parent; name: "bot"; size: 16; color: Theme.rose300 }

                            SequentialAnimation on opacity {
                                running: chatVM.isBusy
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 0.5; duration: 1000; easing.type: Easing.InOutCubic }
                                NumberAnimation { from: 0.5; to: 1.0; duration: 1000; easing.type: Easing.InOutCubic }
                            }
                        }
                        Rectangle {
                            implicitWidth: thinkingRow.implicitWidth + 32
                            implicitHeight: thinkingRow.implicitHeight + 24
                            radius: Theme.radiusLg
                            color: Theme.alpha(Theme.zinc900, 0.9)
                            border.width: 1
                            border.color: Theme.borderGlass

                            RowLayout {
                                id: thinkingRow
                                anchors.centerIn: parent
                                spacing: 8

                                Icon {
                                    name: "sparkles"
                                    size: 16
                                    color: Theme.rose400
                                    RotationAnimator on rotation {
                                        from: 0; to: 360; duration: 1000
                                        loops: Animation.Infinite
                                        running: chatVM.isBusy
                                    }
                                }
                                AppText {
                                    text: qsTr("AI Interviewer is analyzing your response and preparing the next question...")
                                    color: Theme.zinc300
                                }
                            }
                        }
                    }
                }
            }
        }

        // --------------------------------------------------- Answer box
        Item {
            Layout.fillWidth: true
            implicitHeight: 76

            DarkTextArea {
                id: input
                anchors.fill: parent
                trailingSpace: 40
                placeholderText: qsTr("Type your answer... (Press Enter to send, Shift+Enter for a new line)")
                onSubmitted: view.send()
            }

            TactileButton {
                variant: "rose"
                iconLeft: "send"
                implicitWidth: 36
                implicitHeight: 36
                leftPadding: 0
                rightPadding: 0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 14
                anchors.bottomMargin: 14
                enabled: input.text.trim().length > 0 && !chatVM.isBusy && !chatVM.isCompleting
                opacity: enabled ? 1.0 : 0.3
                glow: enabled
                onClicked: view.send()

                ToolTip.visible: hovered
                ToolTip.delay: 600
                ToolTip.text: qsTr("Send")
            }
        }
    }
}
