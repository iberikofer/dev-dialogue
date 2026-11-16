import QtQuick
import QtQuick.Effects
import DevDialogue.App

// Blurred rounded rectangle used as a coloured drop shadow / glow behind
// buttons and cards (Tailwind shadow-lg shadow-blue-600/25 and friends).
// Place it as a sibling below the item it belongs to.
Item {
    id: root

    property color color: Theme.primary
    property real radius: Theme.radiusMd
    property real blur: 24
    property real offsetY: 8
    property real strength: 0.25

    Rectangle {
        id: shape
        anchors.fill: parent
        radius: root.radius
        color: root.color
        visible: false
        layer.enabled: true
    }

    MultiEffect {
        source: shape
        x: 0
        y: root.offsetY
        width: shape.width
        height: shape.height
        opacity: root.strength
        blurEnabled: true
        blur: 1.0
        blurMax: Math.max(2, Math.round(root.blur * 2))
        autoPaddingEnabled: true
    }
}
