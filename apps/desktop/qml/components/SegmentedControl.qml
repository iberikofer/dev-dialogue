import QtQuick
import DevDialogue.App

Rectangle {
    id: control

    property var items: []
    property string currentValue: ""

    signal valueSelected(string value)

    /// Index of the active segment, or -1 when nothing is selected.
    readonly property int selectedIndex: {
        for (let index = 0; index < items.length; ++index) {
            if (items[index].value === currentValue)
                return index
        }
        return -1
    }

    /// Emits the selection request for a value. Used by the delegates and
    /// available for keyboard navigation and tests.
    function selectValue(value) {
        valueSelected(value)
    }

    implicitWidth: segmentRow.implicitWidth + 8
    implicitHeight: 40
    radius: Theme.radiusMd
    color: Theme.backgroundSecondary
    border.width: 1
    border.color: Theme.borderSubtle

    Row {
        id: segmentRow
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        Repeater {
            model: control.items

            delegate: Rectangle {
                id: segment
                required property var modelData

                // qmllint disable unqualified
                readonly property bool selected: control.currentValue === segment.modelData.value
                // qmllint enable unqualified

                width: Math.max(84, segmentLabel.implicitWidth + 26)
                height: parent.height
                radius: Theme.radiusSm
                color: segment.selected ? Theme.backgroundElevated
                                        : (hover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                border.width: segment.selected ? 1 : 0
                border.color: Theme.borderHighlight

                Text {
                    id: segmentLabel
                    anchors.centerIn: parent
                    text: segment.modelData.label
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.bodySmall
                    font.weight: segment.selected ? Typography.weightSemibold
                                                  : Typography.weightRegular
                    color: segment.selected ? Theme.textPrimary : Theme.textSecondary
                }

                HoverHandler {
                    id: hover
                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    // qmllint disable unqualified
                    onTapped: control.selectValue(segment.modelData.value)
                    // qmllint enable unqualified
                }

                Accessible.role: Accessible.Button
                Accessible.name: segmentLabel.text
                Accessible.description: segment.selected ? qsTr("Selected") : ""
            }
        }
    }
}
