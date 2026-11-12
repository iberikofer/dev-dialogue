import QtQuick

// Faint technical dot grid behind the landing and login pages.
Image {
    property int cell: 28

    source: Qt.resolvedUrl("../../resources/patterns/dot-grid.svg")
    sourceSize.width: cell
    sourceSize.height: cell
    fillMode: Image.Tile
    opacity: 0.04
}
