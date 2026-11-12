import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Backend connectivity warning strip under the navbar (web
// BackendStatusBanner.tsx). Collapses to zero height when hidden.
Rectangle {
    id: banner

    readonly property bool shown: backendVM.isOffline && !backendVM.isDismissed

    implicitHeight: shown ? content.implicitHeight + 20 : 0
    height: implicitHeight
    visible: height > 0
    clip: true
    Behavior on implicitHeight { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.amber950, 0.8)) }
        GradientStop { position: 0.5; color: Theme.zinc950 }
        GradientStop { position: 1.0; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.amber950, 0.8)) }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.alpha(Theme.amber500, 0.25)
    }

    RowLayout {
        id: content
        width: Math.min(parent.width, Theme.contentWidthWide) - (parent.width >= 1024 ? 64 : 48)
        anchors.horizontalCenter: parent.horizontalCenter
        y: 10
        spacing: 12

        PulseDot { size: 10 }
        Icon { name: "server-off"; size: 16; color: Theme.amber400 }

        AppText {
            Layout.fillWidth: true
            textFormat: Text.StyledText
            text: "<b><font color=\"" + Theme.amber300 + "\">" + qsTr("Backend Offline") + ":</font></b>&nbsp; "
                + qsTr("API server is currently unreachable or starting up. The platform is running in demo mode with local mock data.")
            color: Theme.zinc300
            font.pixelSize: Typography.xs
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        AbstractButton {
            id: retry
            enabled: !backendVM.isChecking
            implicitHeight: 26
            implicitWidth: retryRow.implicitWidth + 20
            hoverEnabled: true
            opacity: enabled ? 1.0 : 0.6
            onClicked: backendVM.checkHealth()
            HoverHandler { cursorShape: retry.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
            scale: down ? 0.98 : (hovered ? 1.02 : 1.0)
            Behavior on scale { NumberAnimation { duration: Theme.durationFast } }

            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.alpha(Theme.amber500, retry.hovered ? 0.2 : 0.1)
                border.width: 1
                border.color: Theme.alpha(Theme.amber500, retry.hovered ? 0.5 : 0.3)
            }
            contentItem: Item {
                RowLayout {
                    id: retryRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon {
                        name: "refresh-cw"
                        size: 12
                        color: Theme.amber200
                        RotationAnimator on rotation {
                            from: 0; to: 360; duration: 900
                            loops: Animation.Infinite
                            running: backendVM.isChecking
                            onRunningChanged: if (!running) target.rotation = 0
                        }
                    }
                    AppText {
                        text: backendVM.isChecking ? qsTr("Checking...") : qsTr("Retry connection")
                        color: Theme.amber200
                        font.pixelSize: Typography.xs
                        font.weight: Typography.medium
                    }
                }
            }
        }

        AbstractButton {
            id: dismiss
            implicitWidth: 22
            implicitHeight: 22
            hoverEnabled: true
            onClicked: backendVM.dismissBanner()
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: qsTr("Hide notification")

            background: Rectangle {
                radius: Theme.radiusSm
                color: dismiss.hovered ? Theme.alpha(Theme.zinc800, 0.8) : "transparent"
            }
            contentItem: Item {
                Icon {
                    anchors.centerIn: parent
                    name: "x"
                    size: 14
                    color: dismiss.hovered ? Theme.zinc200 : Theme.zinc400
                }
            }
        }
    }
}
