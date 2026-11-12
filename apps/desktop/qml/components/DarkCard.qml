import QtQuick
import DevDialogue.App

Rectangle {
    id: card

    default property alias contentData: contentArea.data

    property int contentPadding: 20
    property int cardRadius: Theme.radiusLg
    property color fillColor: Theme.backgroundCard
    property color outlineColor: Theme.borderSubtle

    color: fillColor
    radius: cardRadius
    border.width: 1
    border.color: outlineColor

    Item {
        id: contentArea
        anchors.fill: parent
        anchors.margins: card.contentPadding
    }
}
