import QtQuick
import DevDialogue.App

Item {
    id: control

    property bool checked: false

    signal toggled()

    implicitWidth: 42
    implicitHeight: 24

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: control.checked
               ? Qt.rgba(Theme.accentEmerald.r, Theme.accentEmerald.g,
                         Theme.accentEmerald.b, 0.28)
               : Theme.backgroundElevated
        border.width: 1
        border.color: control.checked ? Theme.accentEmerald
                                      : Theme.borderHighlight

        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }

    Rectangle {
        id: thumb
        width: 16
        height: 16
        radius: 8
        y: 4
        x: control.checked ? track.width - width - 4 : 4
        color: control.checked ? Theme.accentEmerald : Theme.textMuted

        Behavior on x {
            NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic }
        }
        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: control.toggled()
    }

    Accessible.role: Accessible.CheckBox
    Accessible.name: qsTr("Active")
    Accessible.checked: control.checked
}
