import QtQuick
import DevDialogue.App

// Status dot with a radar ping (web: animate-ping behind a solid dot).
Item {
    id: root

    property color color: Theme.amber500
    property color pingColor: Theme.amber400
    property real size: 8
    property bool pinging: true

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        id: ping
        anchors.centerIn: parent
        width: root.size
        height: root.size
        radius: width / 2
        color: root.pingColor
        visible: root.pinging

        ParallelAnimation {
            running: root.pinging && root.visible
            loops: Animation.Infinite
            NumberAnimation { target: ping; property: "scale"; from: 1.0; to: 2.0; duration: 1000; easing.type: Easing.OutCubic }
            NumberAnimation { target: ping; property: "opacity"; from: 0.75; to: 0.0; duration: 1000; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.color
    }
}
