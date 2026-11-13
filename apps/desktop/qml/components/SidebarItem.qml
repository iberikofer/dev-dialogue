import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// One navigation entry of the sidebar. In the collapsed rail only the icon is
// shown (with a tooltip); the active entry gets the glowing pill and the
// indicator bar on the panel edge.
AbstractButton {
    id: item

    property string iconName
    property bool active: false
    property bool expanded: true
    property int badgeCount: 0
    property bool live: false          // pulsing dot, e.g. a running interview
    property string shortcutHint

    implicitHeight: 44
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    ToolTip.visible: hovered && !expanded
    ToolTip.delay: 300
    ToolTip.text: shortcutHint !== "" ? text + "  ·  " + shortcutHint : text

    background: Item {
        // Glow under the active pill
        SoftGlow {
            anchors.fill: pill
            visible: item.active
            color: Theme.blue500
            radius: pill.radius
            blur: 18
            offsetY: 0
            strength: 0.35
        }

        Rectangle {
            id: pill
            anchors.fill: parent
            radius: height / 2
            color: item.active ? "transparent" : (item.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
            border.width: item.active || item.visualFocus ? 1 : 0
            border.color: item.visualFocus && !item.active ? Theme.focusRing : Theme.alpha(Theme.blue400, 0.45)
            gradient: item.active ? activeFill : null
            Behavior on color { ColorAnimation { duration: Theme.durationFast } }

            Gradient {
                id: activeFill
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.alpha(Theme.blue600, 0.55) }
                GradientStop { position: 0.55; color: Theme.alpha(Theme.blue600, 0.18) }
                GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.03) }
            }
        }
    }

    contentItem: Item {
        // Icon stays centred in the rail and left-aligned when expanded.
        Icon {
            id: glyph
            x: item.expanded ? 14 : (parent.width - width) / 2
            anchors.verticalCenter: parent.verticalCenter
            name: item.iconName
            size: 20
            color: item.active ? Theme.textOnAccent : (item.hovered ? Theme.zinc200 : Theme.zinc400)
            Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        }

        Rectangle {
            visible: item.live
            width: 8
            height: 8
            radius: 4
            color: Theme.emerald400
            border.width: 2
            border.color: Theme.zinc900
            x: glyph.x + glyph.width - 4
            y: glyph.y - 2
        }

        AppText {
            anchors.left: glyph.right
            anchors.leftMargin: 12
            anchors.right: badge.visible ? badge.left : parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: item.text
            color: item.active ? Theme.textOnAccent : (item.hovered ? Theme.zinc100 : Theme.zinc300)
            font.weight: item.active ? Typography.semibold : Typography.medium
            elide: Text.ElideRight
            opacity: item.expanded ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 140 } }
        }

        Rectangle {
            id: badge
            visible: item.badgeCount > 0 && item.expanded
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(22, badgeText.implicitWidth + 12)
            height: 20
            radius: 10
            color: "transparent"
            border.width: 1
            border.color: Theme.borderStrong
            MonoText {
                id: badgeText
                anchors.centerIn: parent
                text: item.badgeCount
                color: Theme.zinc300
                font.pixelSize: Typography.tiny
            }
        }
    }
}
