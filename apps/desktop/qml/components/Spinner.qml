import QtQuick
import QtQuick.Shapes
import DevDialogue.App

Item {
    id: spinner

    property int diameter: 20
    property int strokeWidth: 2
    property color tint: Theme.accentIndigo

    implicitWidth: diameter
    implicitHeight: diameter

    Item {
        id: rotating
        anchors.fill: parent

        Shape {
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 4
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: spinner.tint
                strokeWidth: spinner.strokeWidth
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap

                PathAngleArc {
                    centerX: rotating.width / 2
                    centerY: rotating.height / 2
                    radiusX: rotating.width / 2 - spinner.strokeWidth
                    radiusY: rotating.height / 2 - spinner.strokeWidth
                    startAngle: 0
                    sweepAngle: 270
                }
            }
        }

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: 850
            loops: Animation.Infinite
            running: spinner.visible
        }
    }

    Accessible.role: Accessible.ProgressBar
    Accessible.name: qsTr("Loading")
}
