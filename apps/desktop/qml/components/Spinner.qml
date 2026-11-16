import QtQuick
import QtQuick.Shapes
import DevDialogue.App

// Indeterminate ring spinner (web: animate-spin border-2 border-t-transparent).
Item {
    id: root

    property color color: Theme.textOnAccent
    property real size: 16
    property real thickness: 2
    property bool running: visible

    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            strokeColor: root.color
            strokeWidth: root.thickness
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - root.thickness) / 2
                radiusY: (root.height - root.thickness) / 2
                startAngle: 0
                sweepAngle: 270
            }
        }

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: 800
            loops: Animation.Infinite
            running: root.running
        }
    }
}
