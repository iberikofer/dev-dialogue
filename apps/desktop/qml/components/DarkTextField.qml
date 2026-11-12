import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

TextField {
    id: field

    implicitHeight: 42
    leftPadding: 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    font.family: Theme.fontSans
    font.pixelSize: Typography.body
    color: Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accentIndigo
    selectedTextColor: "#ffffff"
    selectByMouse: true

    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.backgroundPrimary
        border.width: 1
        border.color: field.activeFocus ? Theme.accentIndigo : Theme.borderSubtle

        Behavior on border.color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }
}
