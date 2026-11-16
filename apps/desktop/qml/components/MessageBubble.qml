import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// One chat message. The interviewer sits on the left in a dark bubble with
// a bot avatar, the candidate on the right in the blue accent
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

    // Plain text as HTML, so the selectable text keeps the relaxed line height.
    function asHtml(text) {
        const escaped = text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                            .replace(/\n/g, "<br>")
        return "<div style=\"line-height:" + Math.round(Typography.leadingRelaxed * 100) + "%\">" + escaped + "</div>"
    }

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
                color: root.isAssistant ? Theme.sky300 : Theme.blue300
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
                    GradientStop { position: 0.0; color: Theme.blue600 }
                    GradientStop { position: 0.5; color: Theme.blue500 }
                    GradientStop { position: 1.0; color: Theme.indigo500 }
                }
            }

            ColumnLayout {
                id: bubbleColumn
                x: 16
                y: 12
                width: parent.width - 32
                spacing: 6

                TextEdit {
                    id: body
                    Layout.fillWidth: true
                    text: root.asHtml(root.content)
                    textFormat: TextEdit.RichText
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    color: root.isAssistant ? Theme.zinc200 : Theme.textOnAccent
                    selectionColor: root.isAssistant ? Theme.alpha(Theme.blue500, 0.45) : Theme.alpha(Theme.zinc950, 0.45)
                    selectedTextColor: Theme.textOnAccent
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.sm
                    Accessible.name: root.content
                    HoverHandler { cursorShape: Qt.IBeamCursor }
                }
                MonoText {
                    id: time
                    text: root.timestamp
                    color: root.isAssistant ? Theme.textMuted : Theme.alpha(Theme.blue100, 0.75)
                    font.pixelSize: Typography.micro
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
