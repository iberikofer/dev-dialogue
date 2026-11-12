pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Dark drop-down matching the <select> elements of the admin dialog.
// model: [{ value: "frontend", label: "Frontend" }, ...]
ComboBox {
    id: control

    textRole: "label"
    valueRole: "value"
    implicitHeight: 36
    leftPadding: 10
    rightPadding: 30
    font.family: Theme.fontSans
    font.pixelSize: Typography.xs
    hoverEnabled: true

    HoverHandler { cursorShape: Qt.PointingHandCursor }

    contentItem: AppText {
        text: control.displayText
        color: Theme.textOnAccent
        font.pixelSize: Typography.xs
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Icon {
        x: control.width - width - 10
        y: (control.height - height) / 2
        name: "chevron-down"
        size: 14
        color: Theme.zinc300
        rotation: control.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: 200 } }
    }

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.zinc800
        border.width: 1
        border.color: control.activeFocus || control.popup.visible ? Theme.alpha(Theme.blue500, 0.6) : Theme.zinc700
    }

    delegate: ItemDelegate {
        id: option
        required property var modelData
        required property int index

        width: control.width - 8
        height: 30
        highlighted: control.highlightedIndex === index
        padding: 0

        contentItem: AppText {
            leftPadding: 10
            text: option.modelData.label
            color: Theme.textOnAccent
            font.pixelSize: Typography.xs
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radiusSm
            color: option.highlighted ? Theme.primary : "transparent"
        }
    }

    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: contentItem.implicitHeight + 8
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
        }
        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.zinc800
            border.width: 1
            border.color: Theme.zinc700
        }
    }
}
