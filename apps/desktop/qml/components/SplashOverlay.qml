import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Start-up intro: the app icon and name fade in over a progress line while
// the saved session is restored, then the overlay fades out.
Rectangle {
    id: splash

    property int holdMs: 1500
    signal finished()

    color: Theme.zinc950
    opacity: 1
    visible: opacity > 0
    z: 5000

    // Block input to the app underneath while visible.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
    }

    GlowBlob {
        width: 720
        height: 520
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2 - 40
        color: Theme.blue600
        strength: 0.18
    }

    ColumnLayout {
        id: content
        anchors.centerIn: parent
        spacing: 18
        opacity: 0
        scale: 0.94

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 104
            Layout.preferredHeight: 104
            source: Qt.resolvedUrl("../../resources/app/app-icon.svg")
            sourceSize.width: 208
            sourceSize.height: 208
            smooth: true
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            text: "Dev Dialogue"
            font.pixelSize: Typography.xl3
            font.weight: Typography.bold
            font.letterSpacing: Typography.tracking(Typography.xl3, Typography.trackingTight)
        }
        MonoText {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("AI technical interview simulator")
            color: Theme.textMuted
        }

        // Progress line
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            Layout.preferredWidth: 160
            Layout.preferredHeight: 3
            radius: 2
            color: Theme.zinc800
            clip: true

            Rectangle {
                id: bar
                width: 0
                height: parent.height
                radius: 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.sky300 }
                    GradientStop { position: 1.0; color: Theme.blue500 }
                }
            }
        }
    }

    SequentialAnimation {
        running: true
        ParallelAnimation {
            NumberAnimation { target: content; property: "opacity"; to: 1; duration: 420; easing.type: Easing.OutCubic }
            NumberAnimation { target: content; property: "scale"; to: 1; duration: 520; easing.type: Easing.OutBack }
            NumberAnimation { target: bar; property: "width"; to: 160; duration: splash.holdMs; easing.type: Easing.InOutCubic }
        }
        NumberAnimation { target: splash; property: "opacity"; to: 0; duration: 320; easing.type: Easing.InCubic }
        ScriptAction { script: splash.finished() }
    }
}
