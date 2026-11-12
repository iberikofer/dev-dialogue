import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

TextArea {
    id: area

    implicitHeight: 96
    leftPadding: 12
    rightPadding: 12
    topPadding: 10
    bottomPadding: 10

    font.family: Theme.fontSans
    font.pixelSize: Typography.body
    color: Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accentIndigo
    selectedTextColor: "#ffffff"
    wrapMode: Text.Wrap
    selectByMouse: true

    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.backgroundPrimary
        border.width: 1
        border.color: area.activeFocus ? Theme.accentIndigo : Theme.borderSubtle

        Behavior on border.color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }
}
