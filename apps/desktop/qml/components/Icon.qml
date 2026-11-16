import QtQuick
import QtQuick.Effects
import DevDialogue.App

// Vector icon tinted at runtime. Icon files are drawn in white, so a full
// colorization pass turns them into any theme colour and lets the colour
// animate (hover states) without reloading the image.
Item {
    id: root

    property string name
    property color color: Theme.textPrimary
    property real size: 16
    // Multi-colour marks (brand logos) keep their own colours.
    property bool colorize: true

    implicitWidth: size
    implicitHeight: size

    Image {
        id: glyph
        anchors.fill: parent
        source: Icons.source(root.name)
        sourceSize.width: Math.round(root.size * Screen.devicePixelRatio)
        sourceSize.height: Math.round(root.size * Screen.devicePixelRatio)
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: !root.colorize
    }

    MultiEffect {
        anchors.fill: glyph
        source: glyph
        visible: root.colorize
        colorization: 1.0
        colorizationColor: root.color
    }
}
