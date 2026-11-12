import QtQuick
import DevDialogue.App

Item {
    id: bubble

    property string senderRole: "assistant"
    property string content: ""
    property string timestamp: ""

    readonly property bool isUser: senderRole === "user"
    readonly property real maxBubbleWidth: Math.min(width * 0.78, 680)

    implicitHeight: messageRow.implicitHeight
    implicitWidth: parent ? parent.width : 0

    TextMetrics {
        id: metrics
        font.family: Theme.fontSans
        font.pixelSize: Typography.body
        text: bubble.content
    }

    Row {
        id: messageRow
        width: parent.width
        spacing: 10
        layoutDirection: bubble.isUser ? Qt.RightToLeft : Qt.LeftToRight

        Rectangle {
            width: 32
            height: 32
            radius: 16
            color: bubble.isUser ? Qt.rgba(0.39, 0.4, 0.95, 0.18)
                                 : Qt.rgba(0.08, 0.72, 0.65, 0.16)
            border.width: 1
            border.color: bubble.isUser ? Qt.rgba(0.39, 0.4, 0.95, 0.4)
                                        : Qt.rgba(0.08, 0.72, 0.65, 0.35)

            Image {
                anchors.centerIn: parent
                source: bubble.isUser ? Icons.user : Icons.bot
                sourceSize.width: 18
                sourceSize.height: 18
                width: 18
                height: 18
                fillMode: Image.PreserveAspectFit
            }
        }

        Column {
            id: bubbleColumn
            width: Math.min(bubble.maxBubbleWidth,
                            Math.max(180, metrics.advanceWidth + 28))
            spacing: 4

            Rectangle {
                width: parent.width
                height: bodyText.implicitHeight + 24
                radius: 14
                color: bubble.isUser ? Qt.rgba(0.39, 0.4, 0.95, 0.18)
                                     : Theme.backgroundSecondary
                border.width: 1
                border.color: bubble.isUser ? Qt.rgba(0.39, 0.4, 0.95, 0.36)
                                            : Theme.borderSubtle

                Text {
                    id: bodyText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    text: bubble.content
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.body
                    lineHeight: Typography.lineNormal
                    color: Theme.textPrimary
                }
            }

            Text {
                width: parent.width
                text: Labels.formatTime(bubble.timestamp)
                horizontalAlignment: bubble.isUser ? Text.AlignRight : Text.AlignLeft
                font.family: Theme.fontMono
                font.pixelSize: Typography.tiny
                color: Theme.textMuted
                visible: text.length > 0
            }
        }
    }

    Accessible.role: Accessible.StaticText
    Accessible.name: (bubble.isUser ? qsTr("Your answer: ") : qsTr("Interviewer: ")) + bubble.content
}
