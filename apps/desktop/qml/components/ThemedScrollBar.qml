import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Thin scrollbar from globals.css: 5px zinc-800 thumb, zinc-700 on hover.
ScrollBar {
    id: bar

    implicitWidth: 9
    padding: 2
    minimumSize: 0.08
    policy: ScrollBar.AsNeeded

    contentItem: Rectangle {
        implicitWidth: 5
        radius: width / 2
        color: bar.pressed || bar.hovered ? Theme.zinc700 : Theme.zinc800
        opacity: bar.size < 1.0 ? 1.0 : 0.0
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    background: Item {}
}
