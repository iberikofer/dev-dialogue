import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: root

    property string title: ""
    property bool closable: true
    property int modalWidth: 560
    property int contentSpacing: 0

    default property alias contentData: body.data

    signal closed()

    visible: false
    focus: true

    onVisibleChanged: {
        if (visible)
            forceActiveFocus()
    }

    Keys.onEscapePressed: {
        if (root.closable)
            root.closed()
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.02, 0.03, 0.78)

        TapHandler {
            onTapped: {
                if (root.closable)
                    root.closed()
            }
        }
    }

    Rectangle {
        id: card

        width: Math.min(root.modalWidth, root.width - 48)
        implicitHeight: contentColumn.implicitHeight + 48
        height: Math.min(implicitHeight, root.height - 48)
        anchors.centerIn: parent
        radius: Theme.radiusXl
        color: Theme.backgroundCard
        border.width: 1
        border.color: Theme.borderSubtle

        Column {
            id: contentColumn
            width: card.width - 48
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 24
            spacing: 18

            RowLayout {
                width: parent.width
                spacing: 12

                Text {
                    Layout.fillWidth: true
                    text: root.title
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.h2
                    font.weight: Typography.weightSemibold
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                }

                TactileButton {
                    visible: root.closable
                    text: "✕"
                    outlined: true
                    implicitWidth: 34
                    implicitHeight: 34
                    buttonRadius: Theme.radiusSm
                    textColor: Theme.textSecondary
                    onClicked: root.closed()
                }
            }

            Column {
                id: body
                width: parent.width
                spacing: root.contentSpacing
            }
        }
    }

    Accessible.role: Accessible.Dialog
    Accessible.name: root.title
}
