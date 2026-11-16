import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Start-up intro: the app icon and name fade in over a progress line while
// the saved session is restored, then the overlay fades out. A click skips it.
Rectangle {
    id: splash

    property int holdMs: 1500
    signal finished()

    function skip() {
        intro.stop()
        outro.start()
    }

    color: Theme.zinc950
    opacity: 1
    visible: opacity > 0
    z: 5000

    // Block input to the app underneath while visible; a click skips.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
        onClicked: if (!outro.running) splash.skip()
    }

    GlowBlob {
        width: 720
        height: 520
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2 - 40
        color: Theme.blue600
        strength: 0.18
    }

    // The name and the line under it read as one group; the icon above and
    // the progress line below stand apart from it.
    ColumnLayout {
        id: content
        anchors.centerIn: parent
        spacing: 0
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
            Layout.topMargin: 22
            text: "Dev Dialogue"
            font.pixelSize: Typography.xl3
            font.weight: Typography.bold
            font.letterSpacing: Typography.tracking(Typography.xl3, Typography.trackingTight)
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            text: qsTr("AI technical interview simulator")
            color: Theme.zinc400
        }

        // Progress line
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 32
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
        id: intro
        running: true
        ParallelAnimation {
            NumberAnimation { target: content; property: "opacity"; to: 1; duration: 420; easing.type: Easing.OutCubic }
            // Settles into place without overshooting.
            NumberAnimation { target: content; property: "scale"; to: 1; duration: 700; easing.type: Easing.OutExpo }
            NumberAnimation { target: bar; property: "width"; to: 160; duration: splash.holdMs; easing.type: Easing.InOutCubic }
        }
        // Let the full line register before the app takes over.
        PauseAnimation { duration: 160 }
        ScriptAction { script: outro.start() }
    }

    // The mark eases forward as the overlay fades, handing over to the app.
    SequentialAnimation {
        id: outro
        ParallelAnimation {
            NumberAnimation { target: splash; property: "opacity"; to: 0; duration: 320; easing.type: Easing.InCubic }
            NumberAnimation { target: content; property: "scale"; to: 1.03; duration: 320; easing.type: Easing.OutCubic }
        }
        ScriptAction { script: splash.finished() }
    }
}
