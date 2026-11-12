import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Transient notification anchored to the bottom of the window.
Item {
    id: root

    property string tone: "info"   // info | success | error

    function show(message, messageTone) {
        label.text = message
        tone = messageTone !== undefined ? messageTone : "info"
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
        interval: 4200
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
        }
    }
}
