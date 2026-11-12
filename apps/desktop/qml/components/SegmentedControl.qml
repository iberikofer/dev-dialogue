pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Tab switcher used for Sign In / Sign Up, the settings tabs and the track
// tabs of the landing preview: dark tray, active segment filled blue.
//
// model: [{ value: "signin", label: qsTr("Sign In"), icon: "user" }, ...]
Rectangle {
    id: control

    property var model: []
    property string currentValue
    // Compact trays hug their content; regular ones stretch segments evenly.
    property bool compact: false
    signal activated(string value)

    implicitWidth: row.implicitWidth + padding * 2
    implicitHeight: row.implicitHeight + padding * 2
    readonly property int padding: compact ? 2 : 4

    radius: compact ? Theme.radiusSm : Theme.radiusMd
    color: Theme.alpha(Theme.zinc900, 0.9)
    border.width: 1
    border.color: Theme.borderGlass

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: control.padding
        spacing: control.compact ? 4 : 0

        Repeater {
            model: control.model

            Rectangle {
                id: segment
                required property var modelData
                readonly property bool active: modelData.value === control.currentValue

                Layout.fillWidth: !control.compact
                Layout.preferredWidth: control.compact ? implicitWidth : 100
                implicitWidth: label.implicitWidth + (control.compact ? 24 : 16)
                implicitHeight: control.compact ? 24 : 32
                radius: control.compact ? Theme.radiusXs : Theme.radiusSm
                color: active ? Theme.primary : "transparent"
                scale: tap.pressed ? 0.97 : 1.0
                activeFocusOnTab: true

                Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                Behavior on scale {
                    NumberAnimation { duration: Theme.durationFast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
                }

                Keys.onReturnPressed: control.activated(modelData.value)
                Keys.onSpacePressed: control.activated(modelData.value)

                RowLayout {
                    id: label
                    anchors.centerIn: parent
                    spacing: 8

                    Icon {
                        visible: segment.modelData.icon !== undefined && segment.modelData.icon !== ""
                        name: segment.modelData.icon !== undefined ? segment.modelData.icon : ""
                        size: 14
                        color: labelText.color
                    }
                    AppText {
                        id: labelText
                        text: segment.modelData.label
                        font.pixelSize: Typography.xs
                        font.weight: control.compact ? Typography.medium : Typography.semibold
                        color: segment.active ? Theme.textOnAccent : (hover.hovered ? Theme.zinc200 : Theme.zinc400)
                        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    radius: parent.radius + 2
                    color: "transparent"
                    border.width: 2
                    border.color: Theme.focusRing
                    visible: segment.activeFocus
                }

                HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    id: tap
                    onTapped: control.activated(segment.modelData.value)
                }
            }
        }
    }
}
