pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Interview room: the transcript fills the window, the answer box sits at
// the bottom, session facts and "Finish" live in the header.
AppPage {
    id: page

    active: StackLayout.isCurrentItem
    onActiveChanged: if (active) input.area.forceActiveFocus()

    title: qsTr("Interview")
    subtitle: qsTr("Answers: %1  ·  Time: %2").arg(chatVM.candidateAnswersCount).arg(chatVM.elapsedText)
    badgeText: chatVM.isLocalSession ? qsTr("Offline demo") : ""
    bodyPadding: 24

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
            variant: "success"
            text: chatVM.isCompleting ? qsTr("Generating report...") : qsTr("Finish & get report")
            iconLeft: chatVM.isCompleting ? "" : "circle-check-big"
            busy: chatVM.isCompleting
            implicitHeight: 40
            enabled: !chatVM.isCompleting && !chatVM.isBusy && chatVM.candidateAnswersCount > 0
            opacity: enabled || chatVM.isCompleting ? 1.0 : 0.4
            glow: enabled
            onClicked: chatVM.completeInterview()
            ToolTip.visible: hovered && chatVM.candidateAnswersCount === 0
            ToolTip.delay: 300
            ToolTip.text: qsTr("Answer at least one question first")
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

    // --------------------------------------------------------- Transcript
    ListView {
        id: transcript
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 18
        boundsBehavior: Flickable.StopAtBounds
        model: chatVM.messagesModel
        ScrollBar.vertical: ThemedScrollBar {}

        // Messages keep a readable line length on wide windows.
        readonly property real columnWidth: Math.min(width - 16, 920)
        readonly property real columnX: (width - columnWidth) / 2

        function scrollToEnd() {
            Qt.callLater(transcript.positionViewAtEnd)
        }
        onCountChanged: scrollToEnd()

        header: Item { width: 1; height: 8 }

        delegate: Item {
            id: entry
            required property string role
            required property string content
            required property string timeText

            width: transcript.width
            height: bubble.implicitHeight

            MessageBubble {
                id: bubble
                x: transcript.columnX
                width: transcript.columnWidth
                senderRole: entry.role
                content: entry.content
                timestamp: entry.timeText
            }
        }

        // Shown while the interviewer prepares the next question.
        footer: Item {
            width: transcript.width
            height: chatVM.isBusy ? thinking.implicitHeight + 18 : 8
            onHeightChanged: transcript.scrollToEnd()

            RowLayout {
                id: thinking
                visible: chatVM.isBusy
                x: transcript.columnX
                y: 18
                spacing: 12

                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    implicitWidth: 32
                    implicitHeight: 32
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderGlass
                    Icon { anchors.centerIn: parent; name: "bot"; size: 16; color: Theme.sky300 }
                }
                Rectangle {
                    implicitWidth: dots.implicitWidth + 32
                    implicitHeight: 40
                    radius: Theme.radiusLg
                    color: Theme.alpha(Theme.zinc900, 0.9)
                    border.width: 1
                    border.color: Theme.borderGlass

                    // Typing indicator
                    Row {
                        id: dots
                        anchors.centerIn: parent
                        spacing: 6
                        Repeater {
                            model: 3
                            Rectangle {
                                id: dot
                                required property int index
                                width: 7
                                height: 7
                                radius: 4
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

    // --------------------------------------------------------- Answer box
    Item {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        Layout.maximumWidth: 920
        implicitHeight: 92

        DarkTextArea {
            id: input
            anchors.fill: parent
            trailingSpace: 48
            enabled: !chatVM.isCompleting
            placeholderText: qsTr("Type your answer…   Enter to send, Shift+Enter for a new line")
            onSubmitted: page.send()
        }

        TactileButton {
            iconLeft: "send"
            implicitWidth: 40
            implicitHeight: 40
            leftPadding: 0
            rightPadding: 0
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 14
            anchors.bottomMargin: 14
            enabled: input.text.trim().length > 0 && !chatVM.isBusy && !chatVM.isCompleting
            opacity: enabled ? 1.0 : 0.3
            glow: enabled
            onClicked: page.send()
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: qsTr("Send (Enter)")
        }
    }
}
