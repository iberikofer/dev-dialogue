import QtQuick
import QtQuick.Layouts
import DevDialogue.App

DarkCard {
    id: tile

    property string title: ""
    property string value: ""
    property string suffix: ""
    property string hint: ""
    property string iconSource: Icons.chart
    property color accentColor: Theme.accentIndigo
    property real progress: -1

    implicitHeight: 132
    contentPadding: 18
    cardRadius: Theme.radiusLg

    RowLayout {
        anchors.fill: parent
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            Text {
                text: tile.title
                font.family: Theme.fontSans
                font.pixelSize: Typography.small
                font.weight: Typography.weightMedium
                color: Theme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 6
                Layout.fillWidth: true

                Text {
                    text: tile.value
                    font.family: Theme.fontSans
                    font.pixelSize: 30
                    font.weight: Typography.weightBold
                    color: Theme.textPrimary
                }

                Text {
                    text: tile.suffix
                    visible: tile.suffix.length > 0
                    font.family: Theme.fontMono
                    font.pixelSize: Typography.small
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 6
                }

                Item { Layout.fillWidth: true }
            }

            Text {
                text: tile.hint
                visible: tile.hint.length > 0
                font.family: Theme.fontSans
                font.pixelSize: Typography.tiny
                color: Theme.textMuted
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        ProgressRing {
            Layout.alignment: Qt.AlignVCenter
            visible: tile.progress >= 0
            progress: Math.max(0, Math.min(1, tile.progress))
            centerText: tile.progress >= 0 ? Math.round(tile.progress * 100) + "%" : ""
            toneColor: tile.accentColor
        }

        Rectangle {
            Layout.alignment: Qt.AlignTop
            visible: tile.progress < 0
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: Theme.radiusMd
            color: Qt.rgba(tile.accentColor.r, tile.accentColor.g, tile.accentColor.b, 0.14)
            border.width: 1
            border.color: Qt.rgba(tile.accentColor.r, tile.accentColor.g, tile.accentColor.b, 0.3)

            Image {
                anchors.centerIn: parent
                source: tile.iconSource
                sourceSize.width: 20
                sourceSize.height: 20
                width: 20
                height: 20
                fillMode: Image.PreserveAspectFit
            }
        }
    }
}
