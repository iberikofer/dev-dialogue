import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Tooltip in the app's own dark style; the stock Basic tooltip is a white
// box that belongs to no theme. Declare it inside the item it describes.
// side: "top" (default) or "right" for the collapsed sidebar rail.
ToolTip {
    id: tip

    property string side: "top"

    delay: 500
    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6
    margins: 8

    x: side === "right" && parent ? parent.width + 10 : (parent ? (parent.width - width) / 2 : 0)
    y: side === "right" && parent ? (parent.height - height) / 2 : -height - 8

    contentItem: AppText {
        text: tip.text
        color: Theme.zinc200
        font.pixelSize: Typography.xs
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.zinc850
        border.width: 1
        border.color: Theme.borderStrong
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 80 }
    }
}
