import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Start-up intro: the app icon and name fade in, hold briefly and the
// overlay fades out. It shows no progress because nothing is measured; a
// click skips it.
Rectangle {
    id: splash

    property int holdMs: 500
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
        strength: 0.12
    }

    ColumnLayout {
        id: content
        anchors.centerIn: parent
        spacing: 16
        opacity: 0
        scale: 0.96

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 96
            Layout.preferredHeight: 96
            source: Qt.resolvedUrl("../../resources/app/app-icon.svg")
            sourceSize.width: 192
            sourceSize.height: 192
            smooth: true
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            text: "Dev Dialogue"
            font.pixelSize: Typography.xl3
            font.weight: Typography.bold
            font.letterSpacing: Typography.tracking(Typography.xl3, Typography.trackingTight)
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("AI technical interview simulator")
            color: Theme.textMuted
        }
    }

    SequentialAnimation {
        id: intro
        running: true
        ParallelAnimation {
            NumberAnimation { target: content; property: "opacity"; to: 1; duration: 320; easing.type: Easing.OutCubic }
            NumberAnimation { target: content; property: "scale"; to: 1; duration: 420; easing.type: Easing.OutQuart }
        }
        PauseAnimation { duration: splash.holdMs }
        ScriptAction { script: outro.start() }
    }

    SequentialAnimation {
        id: outro
        NumberAnimation { target: splash; property: "opacity"; to: 0; duration: 260; easing.type: Easing.InCubic }
        ScriptAction { script: splash.finished() }
    }
}
