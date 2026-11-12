import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// UA / EN interface language switch. The highlighted code is the language
// actually on screen (see I18n), so it only moves once a catalog for the
// requested language is installed.
AbstractButton {
    id: toggle

    // "navbar" = bordered chip with a globe, "inline" = settings dialog chip
    property string appearance: "navbar"
    readonly property bool inline: appearance === "inline"

    implicitHeight: inline ? 26 : 30
    implicitWidth: row.implicitWidth + (inline ? 24 : 20)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    // Set when a switch was requested but no catalog for that language is
    // installed, so the interface stayed as it was.
    property bool unavailable: false
    onClicked: {
        const before = I18n.language
        I18n.toggle()
        Qt.callLater(() => {
            toggle.unavailable = I18n.language === before
            if (toggle.unavailable)
                unavailableTimer.restart()
        })
    }
    onHoveredChanged: if (!hovered) unavailable = false

    Timer { id: unavailableTimer; interval: 3500; onTriggered: toggle.unavailable = false }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    ToolTip.visible: hovered || unavailable
    ToolTip.delay: unavailable ? 0 : 600
    ToolTip.text: unavailable
        ? qsTr("This language is not installed yet")
        : (I18n.isUkrainian ? qsTr("Switch to English") : qsTr("Switch to Ukrainian"))

    scale: down ? 0.97 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.durationFast } }

    background: Rectangle {
        radius: toggle.inline ? Theme.radiusSm : Theme.radiusMd
        color: toggle.inline
            ? (toggle.hovered ? Theme.zinc700 : Theme.zinc800)
            : (toggle.hovered ? Theme.zinc800 : Theme.surfaceControl)
        border.width: 1
        border.color: toggle.visualFocus ? Theme.focusRing : (toggle.hovered && !toggle.inline ? Theme.zinc700 : Theme.borderGlass)
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    contentItem: Item {
        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: 6

            Icon {
                visible: !toggle.inline
                name: "globe"
                size: 14
                color: Theme.blue400
            }
            MonoText {
                text: "UA"
                color: I18n.isUkrainian ? Theme.blue400 : Theme.textMuted
                font.weight: I18n.isUkrainian ? Typography.bold : Typography.regular
            }
            MonoText {
                text: "/"
                color: Theme.zinc600
            }
            MonoText {
                text: "EN"
                color: !I18n.isUkrainian ? Theme.blue400 : Theme.textMuted
                font.weight: !I18n.isUkrainian ? Typography.bold : Typography.regular
            }
        }
    }
}
