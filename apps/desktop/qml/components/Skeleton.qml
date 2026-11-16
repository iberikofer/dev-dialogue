import QtQuick
import DevDialogue.App

// Loading placeholder (web: bg-zinc-800/80 animate-pulse).
Rectangle {
    id: root

    radius: Theme.radiusSm
    color: Theme.alpha(Theme.zinc800, 0.8)

    SequentialAnimation on opacity {
        running: root.visible
        loops: Animation.Infinite
        NumberAnimation { from: 1.0; to: 0.5; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { from: 0.5; to: 1.0; duration: 1000; easing.type: Easing.InOutCubic }
    }
}
