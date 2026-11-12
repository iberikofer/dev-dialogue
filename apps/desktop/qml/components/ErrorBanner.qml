import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Rectangle {
    id: banner

    property string errorMessage: ""

    signal dismissed()

    visible: errorMessage.length > 0
    implicitHeight: layout.implicitHeight + 20
    radius: Theme.radiusMd
    color: Qt.rgba(0.96, 0.25, 0.37, 0.1)
    border.width: 1
    border.color: Qt.rgba(0.96, 0.25, 0.37, 0.32)

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        spacing: 10

        Image {
            source: Icons.warning
            sourceSize.width: 18
            sourceSize.height: 18
            fillMode: Image.PreserveAspectFit
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            Layout.alignment: Qt.AlignTop
        }

        Text {
            Layout.fillWidth: true
            text: banner.errorMessage
            wrapMode: Text.Wrap
            font.family: Theme.fontSans
            font.pixelSize: Typography.bodySmall
            lineHeight: Typography.lineNormal
            color: Theme.accentRose
        }

        TactileButton {
            text: "✕"
            outlined: true
            implicitWidth: 30
            implicitHeight: 30
            buttonRadius: Theme.radiusSm
            textColor: Theme.accentRose
            onClicked: banner.dismissed()
        }
    }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: banner.errorMessage
}
