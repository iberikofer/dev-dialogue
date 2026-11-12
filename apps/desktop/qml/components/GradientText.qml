import QtQuick
import QtQuick.Effects
import DevDialogue.App

// Text filled with a horizontal gradient (web: bg-clip-text
// bg-gradient-to-r from-blue-400 via-sky-300 to-indigo-400).
Item {
    id: root

    property alias text: label.text
    property alias font: label.font
    property color fromColor: Theme.blue400
    property color viaColor: Theme.sky300
    property color toColor: Theme.indigo400

    implicitWidth: label.implicitWidth
    implicitHeight: label.implicitHeight

    Text {
        id: label
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        color: "#ffffff"
        font.family: Theme.fontSans
        visible: false
        layer.enabled: true
    }

    Rectangle {
        id: fill
        anchors.fill: parent
        visible: false
        layer.enabled: true
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: root.fromColor }
            GradientStop { position: 0.5; color: root.viaColor }
            GradientStop { position: 1.0; color: root.toColor }
        }
    }

    MultiEffect {
        anchors.fill: parent
        source: fill
        maskEnabled: true
        maskSource: label
    }
}
