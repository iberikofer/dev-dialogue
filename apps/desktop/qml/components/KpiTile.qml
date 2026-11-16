import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Dashboard statistics card: mono caption with an icon, a large value and a
// one-line description. Shows a skeleton while the data is loading.
DarkCard {
    id: tile

    property string caption
    property string value
    property string valueSuffix
    property string description
    property string iconName
    property color iconColor: Theme.blue400
    property color valueColor: Theme.textPrimary
    property bool loading: false
    property int skeletonWidth: 80
    // Optional trailing item next to the value (e.g. a progress ring).
    property alias accessory: accessorySlot.data

    interactive: true
    padding: 20

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 8

            MonoText {
                Layout.fillWidth: true
                text: tile.caption
                caps: true
                font.weight: Typography.semibold
                elide: Text.ElideRight
            }
            Icon {
                name: tile.iconName
                size: 16
                color: tile.iconColor
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 8

            Skeleton {
                visible: tile.loading
                Layout.preferredWidth: tile.skeletonWidth
                Layout.preferredHeight: 36
            }

            MonoText {
                id: valueText
                visible: !tile.loading
                text: tile.value
                color: tile.valueColor
                font.pixelSize: Typography.xl3
                font.weight: Typography.extrabold

                // animate-fade-in-up when the value arrives
                onVisibleChanged: if (visible) valueEntrance.restart()
                transform: Translate { id: valueShift }
                ParallelAnimation {
                    id: valueEntrance
                    NumberAnimation { target: valueText; property: "opacity"; from: 0; to: 1; duration: Theme.durationEnter }
                    NumberAnimation {
                        target: valueShift; property: "y"; from: 8; to: 0
                        duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
                    }
                }
            }
            AppText {
                visible: !tile.loading && tile.valueSuffix !== ""
                Layout.alignment: Qt.AlignBaseline
                text: tile.valueSuffix
                color: Theme.textMuted
                font.family: Theme.fontMono
                font.pixelSize: Typography.sm
            }

            Item { Layout.fillWidth: true }

            Item {
                id: accessorySlot
                visible: !tile.loading && children.length > 0
                implicitWidth: childrenRect.width
                implicitHeight: childrenRect.height
            }
        }

        AppText {
            Layout.fillWidth: true
            Layout.topMargin: 4
            text: tile.description
            color: Theme.textMuted
            font.pixelSize: Typography.xs
            elide: Text.ElideRight
        }
    }
}
