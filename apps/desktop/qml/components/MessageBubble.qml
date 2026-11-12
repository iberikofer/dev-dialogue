import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// One chat message. The interviewer sits on the left in a dark bubble with
// a bot avatar, the candidate on the right in the rose > pink > indigo
// gradient bubble with a user avatar.
Item {
    id: root

    property string senderRole: "assistant"
    property string content
    property string timestamp
    readonly property bool isAssistant: senderRole !== "user"
    // Bubbles never exceed 82% of the row, like the web client.
    readonly property real maxBubbleWidth: Math.max(160, (width - 44) * 0.82)

    implicitHeight: row.implicitHeight

    // Entrance: fade-in-up for each new message.
    opacity: 0
    transform: Translate { id: shift; y: 8 }
    Component.onCompleted: entrance.start()
    ParallelAnimation {
        id: entrance
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: Theme.durationEnter }
        NumberAnimation {
            target: shift; property: "y"; to: 0
            duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
        }
    }

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12
        layoutDirection: root.isAssistant ? Qt.LeftToRight : Qt.RightToLeft

        Rectangle {
            Layout.alignment: Qt.AlignTop
            implicitWidth: 32
            implicitHeight: 32
            radius: Theme.radiusMd
            color: Theme.zinc900
            border.width: 1
            border.color: Theme.borderGlass

            Icon {
                anchors.centerIn: parent
                name: root.isAssistant ? "bot" : "user"
                size: 16
                color: root.isAssistant ? Theme.rose300 : Theme.blue400
            }
        }

        Rectangle {
            id: bubble
            Layout.alignment: Qt.AlignTop
            implicitWidth: Math.min(root.maxBubbleWidth, Math.max(body.implicitWidth, time.implicitWidth) + 32)
            implicitHeight: bubbleColumn.implicitHeight + 24
            radius: Theme.radiusLg
            color: root.isAssistant ? Theme.alpha(Theme.zinc900, 0.9) : "transparent"
            border.width: root.isAssistant ? 1 : 0
            border.color: Theme.borderGlass

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                visible: !root.isAssistant
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.rose500 }
                    GradientStop { position: 0.5; color: Theme.pink500 }
                    GradientStop { position: 1.0; color: Theme.indigo600 }
                }
            }

            ColumnLayout {
                id: bubbleColumn
                x: 16
                y: 12
                width: parent.width - 32
                spacing: 6

                AppText {
                    id: body
                    Layout.fillWidth: true
                    text: root.content
                    color: root.isAssistant ? Theme.zinc200 : Theme.textOnAccent
                    lineHeight: Typography.leadingRelaxed
                    wrapMode: Text.Wrap
                }
                MonoText {
                    id: time
                    text: root.timestamp
                    color: root.isAssistant ? Theme.textMuted : Theme.alpha(Theme.rose100, 0.8)
                    font.pixelSize: Typography.micro
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
