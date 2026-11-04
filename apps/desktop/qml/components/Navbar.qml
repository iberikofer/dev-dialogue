import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DevDialogue.App

Rectangle {
    id: navbarRoot
    height: 64
    color: Theme.backgroundSecondary
    border.color: Theme.borderGlass
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 16

        // Brand Logo & Title
        RowLayout {
            spacing: 10
            
            Rectangle {
                width: 32
                height: 32
                radius: Theme.radiusSm
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.accentRose }
                    GradientStop { position: 1.0; color: Theme.accentIndigo }
                }

                Text {
                    anchors.centerIn: parent
                    text: "DD"
                    font.family: Theme.fontSans
                    font.weight: Font.Bold
                    font.pixelSize: 13
                    color: "#ffffff"
                }
            }

            Text {
                text: "Dev Dialogue"
                font.family: Theme.fontSans
                font.weight: Font.Bold
                font.pixelSize: 16
                color: Theme.textPrimary
            }

            Rectangle {
                radius: 4
                color: Qt.rgba(0.95, 0.25, 0.37, 0.15)
                border.color: Qt.rgba(0.95, 0.25, 0.37, 0.3)
                implicitWidth: badgeText.implicitWidth + 8
                implicitHeight: 18

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: "DESKTOP"
                    font.family: Theme.fontMono
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: Theme.accentRose
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Navigation Links
        RowLayout {
            spacing: 8

            Button {
                text: qsTr("Головна")
                flat: true
                contentItem: Text {
                    text: parent.text
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    font.weight: navVM.currentViewIndex === 0 ? Font.Bold : Font.Normal
                    color: navVM.currentViewIndex === 0 ? Theme.textPrimary : Theme.textSecondary
                }
                onClicked: navVM.navigateToLanding()
            }

            Button {
                text: qsTr("Дашборд")
                flat: true
                contentItem: Text {
                    text: parent.text
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    font.weight: navVM.currentViewIndex === 1 ? Font.Bold : Font.Normal
                    color: navVM.currentViewIndex === 1 ? Theme.textPrimary : Theme.textSecondary
                }
                onClicked: navVM.navigateToDashboard()
            }

            Button {
                text: qsTr("Почати інтерв'ю")
                contentItem: Text {
                    text: parent.text
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: "#ffffff"
                }
                background: Rectangle {
                    radius: Theme.radiusSm
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Theme.accentRose }
                        GradientStop { position: 1.0; color: Theme.accentIndigo }
                    }
                }
                onClicked: navVM.navigateToSetup()
            }
        }
    }
}

