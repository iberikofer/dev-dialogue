import QtQuick
import QtQuick.Shapes
import DevDialogue.App

Item {
    id: ring

    property real progress: 0
    property int diameter: 56
    property int strokeWidth: 5
    property string centerText: ""
    property color toneColor: progress >= 0.7 ? Theme.accentEmerald : Theme.accentAmber

    property real animatedProgress: 0

    implicitWidth: diameter
    implicitHeight: diameter

    Behavior on animatedProgress {
        NumberAnimation { duration: 700; easing.type: Easing.OutCubic }
    }

    onProgressChanged: animatedProgress = Math.max(0, Math.min(1, progress))
    Component.onCompleted: animatedProgress = Math.max(0, Math.min(1, progress))

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.backgroundElevated
            strokeWidth: ring.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.width / 2 - ring.strokeWidth / 2 - 2
                radiusY: ring.height / 2 - ring.strokeWidth / 2 - 2
                startAngle: -90
                sweepAngle: 359.99
            }
        }

        ShapePath {
            strokeColor: ring.toneColor
            strokeWidth: ring.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.width / 2 - ring.strokeWidth / 2 - 2
                radiusY: ring.height / 2 - ring.strokeWidth / 2 - 2
                startAngle: -90
                sweepAngle: Math.max(0, Math.min(359.99, ring.animatedProgress * 360))
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: ring.centerText
        font.family: Theme.fontMono
        font.pixelSize: Typography.tiny
        font.weight: Typography.weightSemibold
        color: Theme.textSecondary
        visible: ring.centerText.length > 0
    }

    Accessible.role: Accessible.ProgressBar
    Accessible.name: qsTr("%1 percent").arg(Math.round(ring.progress * 100))
}
