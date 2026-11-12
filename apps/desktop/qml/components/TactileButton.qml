import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

Button {
    id: control

    property color bgGradientStart: Theme.accentIndigo
    property color bgGradientEnd: Theme.accentIndigoDeep
    property int buttonRadius: Theme.radiusMd
    property bool outlined: false
    property color textColor: Theme.textPrimary
    property string iconSource: ""

    implicitHeight: 42
    implicitWidth: Math.max(120, contentRow.implicitWidth + 36)
    hoverEnabled: enabled

    scale: down ? 0.97 : (hovered && enabled ? 1.02 : 1.0)
    Behavior on scale {
        NumberAnimation { duration: Theme.durationFast; easing.type: Easing.OutBack }
    }

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 8

            Image {
                visible: control.iconSource !== ""
                source: control.iconSource
                sourceSize.width: 16
                sourceSize.height: 16
                width: 16
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
            }

            Text {
                text: control.text
                font.family: Theme.fontSans
                font.pixelSize: Typography.body
                font.weight: Typography.weightSemibold
                color: control.enabled ? control.textColor : Theme.textMuted
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
        radius: control.buttonRadius
        color: "transparent"
        border.width: 1
        border.color: control.outlined ? Theme.borderHighlight
                                       : Qt.rgba(1, 1, 1, 0.15)
        opacity: control.enabled ? 1.0 : 0.5

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: control.outlined ? "transparent"
                                        : (control.down ? Qt.darker(control.bgGradientStart, 1.15)
                                                        : control.bgGradientStart)
            }
            GradientStop {
                position: 1.0
                color: control.outlined ? "transparent"
                                        : (control.down ? Qt.darker(control.bgGradientEnd, 1.15)
                                                        : control.bgGradientEnd)
            }
        }
    }
}
