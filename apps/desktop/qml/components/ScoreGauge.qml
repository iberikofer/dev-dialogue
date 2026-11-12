import QtQuick
import QtQuick.Shapes
import DevDialogue.App

Item {
    id: gauge

    property real value: 0
    property int diameter: 168
    property int strokeWidth: 12
    property string caption: qsTr("Overall score")
    property bool showCaption: true

    readonly property int score: Math.round(value)
    readonly property color gaugeColor: Labels.scoreColor(value)
    readonly property real radius: Math.max(1, diameter / 2 - strokeWidth / 2 - 4)

    property real animatedValue: 0

    implicitWidth: diameter
    implicitHeight: diameter + (showCaption ? 54 : 24)

    Behavior on animatedValue {
        NumberAnimation { duration: 900; easing.type: Easing.OutCubic }
    }

    onValueChanged: animatedValue = value
    Component.onCompleted: animatedValue = value

    Shape {
        width: gauge.diameter
        height: gauge.diameter
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        layer.enabled: true
        layer.samples: 4
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.backgroundElevated
            strokeWidth: gauge.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: gauge.diameter / 2
                centerY: gauge.diameter / 2
                radiusX: gauge.radius
                radiusY: gauge.radius
                startAngle: -90
                sweepAngle: 359.99
            }
        }

        ShapePath {
            strokeColor: gauge.gaugeColor
            strokeWidth: gauge.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: gauge.diameter / 2
                centerY: gauge.diameter / 2
                radiusX: gauge.radius
                radiusY: gauge.radius
                startAngle: -90
                sweepAngle: Math.max(0, Math.min(359.99, gauge.animatedValue * 3.6))
            }
        }
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: gauge.showCaption ? -8 : 0
        spacing: 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: gauge.score
            font.family: Theme.fontSans
            font.pixelSize: Math.round(gauge.diameter * 0.26)
            font.weight: Typography.weightBold
            color: gauge.gaugeColor
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: gauge.showCaption
            text: qsTr("out of 100")
            font.family: Theme.fontMono
            font.pixelSize: Typography.tiny
            color: Theme.textMuted
        }
    }

    Text {
        anchors.top: parent.top
        anchors.topMargin: gauge.diameter + 10
        anchors.horizontalCenter: parent.horizontalCenter
        visible: gauge.showCaption
        text: gauge.caption
        font.family: Theme.fontSans
        font.pixelSize: Typography.small
        font.weight: Typography.weightMedium
        color: Theme.textSecondary
    }

    Accessible.role: Accessible.ProgressBar
    Accessible.name: qsTr("Score %1 out of 100").arg(gauge.score)
}
