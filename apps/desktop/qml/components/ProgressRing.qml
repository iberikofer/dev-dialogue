import QtQuick
import QtQuick.Shapes
import DevDialogue.App

// Circular progress indicator. `value` is 0..1; the arc starts at 12 o'clock
// and animates whenever the value changes.
Item {
    id: root

    property real value: 0
    property real size: 40
    property real thickness: 4
    property color trackColor: Theme.zinc800
    property color progressColor: Theme.blue400
    property int animationDuration: 900

    readonly property real clamped: Math.max(0, Math.min(1, value))
    property real shown: clamped
    Behavior on shown {
        NumberAnimation { duration: root.animationDuration; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
    }

    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            strokeColor: root.trackColor
            strokeWidth: root.thickness
            fillColor: "transparent"
            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - root.thickness) / 2
                radiusY: (root.height - root.thickness) / 2
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath {
            strokeColor: root.shown > 0.001 ? root.progressColor : "transparent"
            strokeWidth: root.thickness
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - root.thickness) / 2
                radiusY: (root.height - root.thickness) / 2
                startAngle: -90
                sweepAngle: 360 * root.shown
            }
        }
    }
}
