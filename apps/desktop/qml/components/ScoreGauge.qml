import QtQuick
import DevDialogue.App

// Overall score dial of the report screen: 144px disc with a zinc ring, the
// score counting up in the centre and a coloured arc filling to score/100.
// Colours follow the web thresholds (>= 80 emerald, >= 60 blue, else amber).
Item {
    id: root

    property int score: 0
    property string suffix: qsTr("/ 100 points")
    property bool animated: true
    readonly property color scoreColor: Theme.scoreColor(score)

    // Number shown while counting up.
    property real displayed: 0
    function restart() {
        if (!animated) {
            displayed = score
            ring.value = score / 100
            return
        }
        countUp.stop()
        displayed = 0
        ring.animationDuration = 0
        ring.value = 0
        ring.animationDuration = 1100
        ring.value = score / 100
        countUp.start()
    }
    onScoreChanged: restart()
    Component.onCompleted: restart()

    implicitWidth: 144
    implicitHeight: 144

    NumberAnimation {
        id: countUp
        target: root
        property: "displayed"
        from: 0
        to: root.score
        duration: 1100
        easing.type: Easing.OutCubic
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.zinc950
    }

    ProgressRing {
        id: ring
        anchors.fill: parent
        size: root.width
        thickness: 4
        trackColor: Theme.zinc800
        progressColor: root.scoreColor
    }

    Column {
        anchors.centerIn: parent
        spacing: 0

        AppText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Math.round(root.displayed)
            color: root.scoreColor
            font.pixelSize: Typography.xl5
            font.weight: Typography.black
            font.letterSpacing: Typography.tracking(Typography.xl5, Typography.trackingTight)
        }
        AppText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.suffix
            color: Theme.textMuted
            font.pixelSize: Typography.xs
            font.weight: Typography.semibold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: Typography.tracking(Typography.xs, Typography.trackingWider)
        }
    }
}
