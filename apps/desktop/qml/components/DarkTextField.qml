import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Single-line input of the web forms: translucent zinc fill, hairline
// border, blue border + ring on focus, optional leading icon and a
// show/hide toggle for passwords.
TextField {
    id: field

    property string leadingIcon
    property bool password: false
    property bool revealed: false
    property bool invalid: false
    // "form" = login / settings inputs, "solid" = admin dialog inputs
    property string appearance: "form"

    readonly property bool solid: appearance === "solid"

    implicitHeight: solid ? 36 : 42
    leftPadding: leadingIcon !== "" ? 40 : (solid ? 10 : 14)
    rightPadding: password ? 44 : (solid ? 10 : 14)
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    color: solid ? Theme.textOnAccent : Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.alpha(Theme.blue500, 0.35)
    selectedTextColor: Theme.textOnAccent
    font.family: Theme.fontSans
    font.pixelSize: solid ? Typography.xs : Typography.sm
    echoMode: password && !revealed ? TextInput.Password : TextInput.Normal
    passwordCharacter: "•"
    selectByMouse: true

    background: Rectangle {
        radius: Theme.radiusMd
        color: field.solid ? Theme.zinc800 : (field.activeFocus ? Theme.zinc900 : Theme.surfaceInput)
        border.width: 1
        border.color: field.invalid
            ? Theme.alpha(Theme.red500, 0.6)
            : field.activeFocus
                ? Theme.alpha(Theme.blue500, 0.6)
                : (field.solid ? Theme.zinc700 : Theme.borderGlass)
        Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }

        // focus:ring-1 focus:ring-blue-500/40
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.width: 1
            border.color: Theme.alpha(Theme.blue500, 0.4)
            visible: field.activeFocus && !field.invalid
        }
    }

    Icon {
        visible: field.leadingIcon !== ""
        name: field.leadingIcon
        size: 16
        color: Theme.textMuted
        x: 14
        anchors.verticalCenter: parent.verticalCenter
    }

    Rectangle {
        id: revealButton
        visible: field.password
        width: 28
        height: 28
        radius: Theme.radiusSm
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        color: revealHover.hovered ? Qt.rgba(1, 1, 1, 0.06) : "transparent"

        Icon {
            anchors.centerIn: parent
            name: field.revealed ? "eye-off" : "eye"
            size: 16
            color: revealHover.hovered ? Theme.zinc200 : Theme.zinc400
        }
        HoverHandler { id: revealHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: field.revealed = !field.revealed }

        ToolTip.visible: revealHover.hovered
        ToolTip.delay: 600
        ToolTip.text: field.revealed ? qsTr("Hide password") : qsTr("Show password")
    }
}
