import QtQuick
import DevDialogue.App

Rectangle {
    id: badge

    property string text: ""
    property color toneColor: Theme.accentIndigo

    implicitWidth: label.implicitWidth + 16
    implicitHeight: 22
    radius: 6
    color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.14)
    border.width: 1
    border.color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.34)

    Text {
        id: label
        anchors.centerIn: parent
        text: badge.text
        font.family: Theme.fontMono
        font.pixelSize: Typography.tiny
        font.weight: Typography.weightMedium
        color: badge.toneColor
    }

    Accessible.role: Accessible.StaticText
    Accessible.name: badge.text
}
