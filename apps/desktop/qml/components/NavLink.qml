import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Primary navigation link in the top bar. `accent` renders the blue admin
// variant.
AbstractButton {
    id: link

    property string iconName
    property bool active: false
    property bool accent: false

    implicitHeight: 30
    implicitWidth: row.implicitWidth + 24
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    readonly property color foreground: accent
        ? (active || hovered ? Theme.blue200 : Theme.alpha(Theme.blue400, 0.9))
        : (active ? Theme.zinc100 : (hovered ? Theme.zinc200 : Theme.zinc400))

    background: Rectangle {
        radius: Theme.radiusSm
        color: link.accent
            ? (link.active ? Theme.alpha(Theme.blue950, 0.6) : (link.hovered ? Theme.alpha(Theme.blue950, 0.3) : "transparent"))
            : (link.active ? Theme.alpha(Theme.zinc800, 0.8) : (link.hovered ? Theme.alpha(Theme.zinc900, 0.6) : "transparent"))
        border.width: 1
        border.color: link.active
            ? (link.accent ? Theme.alpha(Theme.blue500, 0.3) : Theme.borderGlass)
            : (link.visualFocus ? Theme.focusRing : "transparent")
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    contentItem: Item {
        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: 8

            Icon {
                name: link.iconName
                size: 14
                color: Theme.blue400
            }
            AppText {
                text: link.text
                color: link.foreground
                font.pixelSize: Typography.xs
                font.weight: Typography.medium
                Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            }
        }
    }
}
