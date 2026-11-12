import QtQuick
import DevDialogue.App

Item {
    id: empty

    property string iconSource: Icons.sparkles
    property string title: qsTr("Nothing here yet")
    property string description: ""
    property string actionText: ""

    signal actionClicked()

    Column {
        width: Math.min(400, empty.width - 48)
        anchors.centerIn: parent
        spacing: 14

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 56
            height: 56
            radius: 28
            color: Qt.rgba(0.39, 0.4, 0.95, 0.12)
            border.width: 1
            border.color: Qt.rgba(0.39, 0.4, 0.95, 0.3)

            Image {
                anchors.centerIn: parent
                source: empty.iconSource
                sourceSize.width: 24
                sourceSize.height: 24
                width: 24
                height: 24
                fillMode: Image.PreserveAspectFit
            }
        }

        Text {
            width: parent.width
            text: empty.title
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.family: Theme.fontSans
            font.pixelSize: Typography.h3
            font.weight: Typography.weightSemibold
            color: Theme.textPrimary
        }

        Text {
            width: parent.width
            visible: empty.description.length > 0
            text: empty.description
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.family: Theme.fontSans
            font.pixelSize: Typography.bodySmall
            lineHeight: Typography.lineNormal
            color: Theme.textSecondary
        }

        TactileButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: empty.actionText.length > 0
            text: empty.actionText
            onClicked: empty.actionClicked()
        }
    }
}
