import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// 40x20 pill switch from the settings dialog (blue when on, zinc when off).
Switch {
    id: control

    implicitWidth: 40
    implicitHeight: 20
    padding: 0
    hoverEnabled: true

    HoverHandler { cursorShape: Qt.PointingHandCursor }

    indicator: Rectangle {
        implicitWidth: 40
        implicitHeight: 20
        radius: 10
        color: control.checked ? Theme.primary : Theme.zinc800
        Behavior on color { ColorAnimation { duration: 200 } }

        Rectangle {
            width: 16
            height: 16
            radius: 8
            y: 2
            x: control.checked ? parent.width - width - 2 : 2
            color: "#ffffff"
            Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: height / 2
            color: "transparent"
            border.width: 2
            border.color: Theme.focusRing
            visible: control.visualFocus
        }
    }

    contentItem: Item {}
}
