import QtQuick
import DevDialogue.App

// Page footer (web Footer.tsx).
Rectangle {
    implicitHeight: 65
    color: Theme.alpha(Theme.zinc950, 0.8)

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.borderGlass
    }

    MonoText {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, implicitWidth)
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        color: Theme.textMuted
        text: qsTr("Dev Dialogue © 2026 · Platform engineered for VNTU Software Engineering Olympiad")
    }
}
