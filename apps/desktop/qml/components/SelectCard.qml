import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Selectable option card of the interview setup screen. Selected cards get
// the blue border, tinted fill, 1px ring and glow; idle cards brighten on
// hover. Uses the .card-interactive lift and press feedback.
AbstractButton {
    id: card

    property bool selected: false
    property int cardRadius: Theme.radiusLg

    padding: 16
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    transform: [
        Translate {
            y: card.hovered && !card.down ? -2 : 0
            Behavior on y {
                NumberAnimation { duration: Theme.durationCard; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
            }
        },
        Scale {
            origin.x: card.width / 2
            origin.y: card.height / 2
            xScale: card.down ? 0.99 : 1.0
            yScale: xScale
            Behavior on xScale {
                NumberAnimation { duration: Theme.durationCard; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
            }
        }
    ]

    background: Item {
        SoftGlow {
            anchors.fill: parent
            visible: card.selected
            color: Theme.blue500
            radius: card.cardRadius
            blur: 16
            offsetY: 6
            strength: 0.12
        }

        Rectangle {
            anchors.fill: parent
            radius: card.cardRadius
            color: card.selected
                ? Qt.tint(Theme.zinc950, Theme.alpha(Theme.blue950, 0.25))
                : (card.hovered ? Theme.surfaceCardHover : Theme.surfaceCard)
            border.width: 1
            border.color: card.selected
                ? Theme.alpha(Theme.blue500, 0.7)
                : (card.hovered ? Qt.rgba(1, 1, 1, 0.16) : Theme.borderGlass)
            Behavior on color { ColorAnimation { duration: Theme.durationCard } }
            Behavior on border.color { ColorAnimation { duration: Theme.durationCard } }
        }

        // ring-1 ring-blue-500/50
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: card.cardRadius + 1
            color: "transparent"
            border.width: 1
            border.color: Theme.alpha(Theme.blue500, 0.5)
            visible: card.selected
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -4
            radius: card.cardRadius + 4
            color: "transparent"
            border.width: 2
            border.color: Theme.focusRing
            visible: card.visualFocus
        }
    }
}
