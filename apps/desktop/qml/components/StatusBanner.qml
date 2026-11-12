import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Rectangle {
    id: banner

    property string message: qsTr("Backend is unreachable. The app is running in offline demo mode.")

    signal retryRequested()

    implicitHeight: layout.implicitHeight + 20
    color: Qt.rgba(0.96, 0.62, 0.04, 0.12)
    border.width: 1
    border.color: Qt.rgba(0.96, 0.62, 0.04, 0.35)

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        Image {
            source: Icons.warning
            sourceSize.width: 18
            sourceSize.height: 18
            fillMode: Image.PreserveAspectFit
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            Layout.fillWidth: true
            text: banner.message
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            color: Theme.accentAmber
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }

        TactileButton {
            text: qsTr("Retry")
            outlined: true
            implicitHeight: 30
            implicitWidth: 84
            buttonRadius: Theme.radiusSm
            textColor: Theme.accentAmber
            onClicked: banner.retryRequested()
        }
    }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: banner.message
}
