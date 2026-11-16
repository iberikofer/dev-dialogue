import QtQuick
import DevDialogue.App

// Squircle user picture. Without an avatar URL it shows the signature
// "square head" glyph of the web client.
Rectangle {
    id: root

    property url avatarUrl
    property int size: 32
    property int glyphSize: 18
    property bool highlighted: false

    implicitWidth: size
    implicitHeight: size
    radius: Theme.radiusSm
    color: Theme.zinc800
    border.width: 1
    border.color: highlighted ? Theme.alpha(Theme.blue500, 0.4) : Theme.borderStrong
    clip: true
    Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }

    Image {
        id: picture
        anchors.fill: parent
        anchors.margins: 1
        source: root.avatarUrl
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        visible: status === Image.Ready
    }

    Icon {
        anchors.centerIn: parent
        visible: !picture.visible
        name: "user-square-head"
        size: root.glyphSize
        color: root.highlighted ? Theme.blue300 : Theme.blue400
    }
}
