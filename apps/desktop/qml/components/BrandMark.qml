import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Product logo: terminal glyph in a blue outlined tile plus the wordmark.
Item {
    id: root

    property int tileSize: 28
    property int glyphSize: 14
    property int tileRadius: Theme.radiusSm
    property int textSize: Typography.base
    property int textWeight: Typography.semibold
    property bool hovered: hover.hovered
    signal clicked()

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 10

        Rectangle {
            implicitWidth: root.tileSize
            implicitHeight: root.tileSize
            radius: root.tileRadius
            color: Theme.alpha(Theme.blue500, root.hovered ? 0.2 : 0.1)
            border.width: 1
            border.color: root.hovered ? Theme.blue400 : Theme.alpha(Theme.blue500, 0.6)
            scale: root.hovered ? 1.05 : 1.0
            Behavior on scale { NumberAnimation { duration: 200 } }
            Behavior on color { ColorAnimation { duration: 200 } }

            Icon {
                anchors.centerIn: parent
                name: "logo-terminal"
                size: root.glyphSize
                color: root.hovered ? Theme.blue300 : Theme.blue400
            }
        }

        AppText {
            text: "Dev Dialogue"
            font.pixelSize: root.textSize
            font.weight: root.textWeight
            font.letterSpacing: Typography.tracking(root.textSize, Typography.trackingTight)
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.clicked() }
}
