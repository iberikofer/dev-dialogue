import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Multi-line input. "chat" matches the interview answer box (rose focus
// ring), "solid" matches the admin dialog textarea.
ScrollView {
    id: root

    property alias text: area.text
    property alias placeholderText: area.placeholderText
    property alias area: area
    property string appearance: "chat"
    property bool invalid: false
    property int trailingSpace: 0
    signal submitted()

    readonly property bool solid: appearance === "solid"
    readonly property color focusColor: solid ? Theme.blue500 : Theme.rose500

    implicitHeight: solid ? 76 : 76
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    background: Rectangle {
        radius: root.solid ? Theme.radiusMd : Theme.radiusLg
        color: root.solid ? Theme.zinc800 : Theme.alpha(Theme.zinc900, 0.7)
        border.width: 1
        border.color: root.invalid
            ? Theme.alpha(Theme.red500, 0.6)
            : area.activeFocus
                ? Theme.alpha(root.focusColor, 0.5)
                : (root.solid ? Theme.zinc700 : Theme.borderGlass)
        Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.width: 1
            border.color: Theme.alpha(root.focusColor, 0.4)
            visible: area.activeFocus && !root.invalid
        }
    }

    TextArea {
        id: area
        leftPadding: root.solid ? 10 : 16
        rightPadding: (root.solid ? 10 : 16) + root.trailingSpace
        topPadding: root.solid ? 10 : 16
        bottomPadding: root.solid ? 10 : 16
        wrapMode: TextEdit.Wrap
        color: root.solid ? Theme.textOnAccent : Theme.textPrimary
        placeholderTextColor: Theme.textMuted
        selectionColor: Theme.alpha(Theme.blue500, 0.35)
        selectedTextColor: Theme.textOnAccent
        font.family: Theme.fontSans
        font.pixelSize: root.solid ? Typography.xs : Typography.sm
        selectByMouse: true
        background: null

        // Enter submits, Shift+Enter inserts a new line (web ChatRoom).
        Keys.onPressed: (event) => {
            if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                    && !(event.modifiers & Qt.ShiftModifier)
                    && root.appearance === "chat") {
                event.accepted = true
                root.submitted()
            }
        }
    }
}
