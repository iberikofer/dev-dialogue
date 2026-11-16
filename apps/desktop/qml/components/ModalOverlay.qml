import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Full-window modal layer: dimmed backdrop, centred dialog card with the
// fade-in-up entrance, closes on Escape or a click outside the card.
// It can be declared anywhere; when opened it moves itself to the window's
// content item so it also covers the navbar.
Item {
    id: overlay

    default property alias content: cardBody.data
    property bool open: false
    property int dialogWidth: 512
    property int dialogPadding: 28
    property int dialogRadius: Theme.radiusLg
    property color dialogColor: Theme.zinc950
    property color dialogBorder: Theme.borderStrong
    property real backdropOpacity: 0.8
    property bool closeOnBackdrop: true
    // Height available to the dialog body (the card is capped at 90% of the window).
    readonly property real maxBodyHeight: Math.max(120, height * 0.9 - dialogPadding * 2)
    signal closeRequested()

    // Plain geometry instead of anchors: the dialog may be declared inside a
    // layout, where it stays hidden until it is moved to the window root.
    x: 0
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    z: 1000
    visible: open || backdrop.opacity > 0.01
    enabled: open

    onOpenChanged: {
        if (open) {
            const host = overlay.ApplicationWindow.contentItem
                ? overlay.ApplicationWindow.contentItem : overlay.Window.contentItem
            if (host && overlay.parent !== host)
                overlay.parent = host
            forceActiveFocus()
            cardEntrance.restart()
        }
    }

    Keys.onEscapePressed: closeRequested()

    Rectangle {
        id: backdrop
        anchors.fill: parent
        color: "#000000"
        opacity: overlay.open ? overlay.backdropOpacity : 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }

    // Swallows clicks, wheel and hover so the page underneath stays inert.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
        onClicked: if (overlay.closeOnBackdrop) overlay.closeRequested()
    }

    Rectangle {
        id: card
        width: Math.min(overlay.dialogWidth, overlay.width - 32)
        height: cardBody.implicitHeight + overlay.dialogPadding * 2
        anchors.horizontalCenter: parent.horizontalCenter
        y: (overlay.height - height) / 2 + entranceOffset
        property real entranceOffset: 0
        radius: overlay.dialogRadius
        color: overlay.dialogColor
        border.width: 1
        border.color: overlay.dialogBorder
        opacity: overlay.open ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 180 } }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onWheel: (wheel) => wheel.accepted = true
        }

        ColumnLayout {
            id: cardBody
            x: overlay.dialogPadding
            y: overlay.dialogPadding
            width: parent.width - overlay.dialogPadding * 2
            spacing: 0
        }
    }

    NumberAnimation {
        id: cardEntrance
        target: card; property: "entranceOffset"; from: 8; to: 0
        duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
    }
}
