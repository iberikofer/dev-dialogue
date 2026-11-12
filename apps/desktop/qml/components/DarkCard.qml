import QtQuick
import QtQuick.Controls.Basic
import DevDialogue.App

// Card surface of the web client: translucent zinc fill, 1px hairline
// border, 16px radius. With `interactive` it gets the .card-interactive
// hover lift.
Pane {
    id: card

    property color fillColor: Theme.surfaceCard
    property color hoverFillColor: fillColor
    property color strokeColor: Theme.borderGlass
    property color hoverStrokeColor: strokeColor
    property int cardRadius: Theme.radiusLg
    property bool interactive: false
    property alias gradient: surface.gradient

    padding: 20
    hoverEnabled: true

    transform: Translate {
        y: card.interactive && card.hovered ? -2 : 0
        Behavior on y {
            NumberAnimation { duration: Theme.durationCard; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
        }
    }

    background: Rectangle {
        id: surface
        radius: card.cardRadius
        color: card.hovered ? card.hoverFillColor : card.fillColor
        border.width: 1
        border.color: card.hovered ? card.hoverStrokeColor : card.strokeColor
        Behavior on color { ColorAnimation { duration: Theme.durationCard } }
        Behavior on border.color { ColorAnimation { duration: Theme.durationCard } }
    }
}
