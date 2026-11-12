import QtQuick
import QtQuick.Shapes
import DevDialogue.App

// Soft ambient light: an elliptical radial gradient fading to transparent.
// Stands in for the heavily blurred gradient blobs behind the web hero and
// login card.
Item {
    id: root

    property color color: Theme.blue600
    property real strength: 0.2

    Shape {
        // Drawn as a circle and stretched to the item's aspect ratio.
        width: root.height
        height: root.height
        transform: Scale { xScale: root.width / Math.max(1, root.height) }

        ShapePath {
            strokeColor: "transparent"
            fillGradient: RadialGradient {
                centerX: root.height / 2
                centerY: root.height / 2
                centerRadius: root.height / 2
                focalX: centerX
                focalY: centerY
                GradientStop { position: 0.0; color: Theme.alpha(root.color, root.strength) }
                GradientStop { position: 0.25; color: Theme.alpha(root.color, root.strength * 0.72) }
                GradientStop { position: 0.5; color: Theme.alpha(root.color, root.strength * 0.38) }
                GradientStop { position: 0.75; color: Theme.alpha(root.color, root.strength * 0.12) }
                GradientStop { position: 1.0; color: Theme.alpha(root.color, 0) }
            }
            PathRectangle {
                x: 0
                y: 0
                width: root.height
                height: root.height
            }
        }
    }
}
