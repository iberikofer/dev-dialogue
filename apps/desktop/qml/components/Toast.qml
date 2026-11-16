import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Transient notification anchored to the bottom of the window.
Item {
    id: root

    property string tone: "info"   // info | success | error
    property string actionText: ""
    property var action: null

    // actionLabel / onAction add a button such as "Undo"; the toast then
    // stays as long as the action can still be taken.
    function show(message, messageTone, actionLabel, onAction) {
        label.text = message
        tone = messageTone !== undefined ? messageTone : "info"
        actionText = actionLabel !== undefined ? actionLabel : ""
        action = onAction !== undefined ? onAction : null
        hideTimer.interval = actionText !== "" ? 5000 : 4200
        opacity = 1
        hideTimer.restart()
    }

    readonly property color accent: tone === "success" ? Theme.emerald400 : tone === "error" ? Theme.red400 : Theme.blue400

    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.bottomMargin: 28
    width: Math.min(pill.implicitWidth, (parent ? parent.width : 480) - 48)
    height: pill.implicitHeight
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    Timer {
        id: hideTimer
        onTriggered: root.opacity = 0
    }

    Rectangle {
        id: pill
        anchors.fill: parent
        implicitWidth: row.implicitWidth + 32
        implicitHeight: row.implicitHeight + 20
        radius: Theme.radiusMd
        color: Theme.zinc900
        border.width: 1
        border.color: Theme.alpha(root.accent, 0.35)

        RowLayout {
            id: row
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10

            Icon {
                name: root.tone === "success" ? "circle-check" : root.tone === "error" ? "circle-alert" : "info"
                size: 16
                color: root.accent
            }
            AppText {
                id: label
                Layout.fillWidth: true
                color: Theme.zinc200
                font.pixelSize: Typography.xs
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
            AbstractButton {
                id: actionButton
                visible: root.actionText !== ""
                Layout.leftMargin: 4
                implicitWidth: actionLabel.implicitWidth + 16
                implicitHeight: 26
                hoverEnabled: true
                Accessible.name: root.actionText
                onClicked: {
                    const run = root.action
                    hideTimer.stop()
                    root.opacity = 0
                    root.action = null
                    if (run)
                        run()
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                background: Rectangle {
                    radius: Theme.radiusSm
                    color: actionButton.hovered ? Theme.alpha(Theme.blue500, 0.16) : "transparent"
                }
                contentItem: AppText {
                    id: actionLabel
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: root.actionText
                    color: Theme.blue400
                    font.pixelSize: Typography.xs
                    font.weight: Typography.semibold
                }
            }
        }
    }
}
