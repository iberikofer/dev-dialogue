import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: landingRoot

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 24
        width: Math.min(800, parent.width - 48)

        // Pill badge
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            radius: 100
            color: Qt.rgba(0.2, 0.4, 0.9, 0.15)
            border.color: Qt.rgba(0.2, 0.4, 0.9, 0.3)
            implicitWidth: pillText.implicitWidth + 20
            implicitHeight: 28

            Text {
                id: pillText
                anchors.centerIn: parent
                text: "✨ C++ Native Desktop Edition"
                font.family: Theme.fontMono
                font.pixelSize: 12
                font.weight: Font.Medium
                color: "#60a5fa"
            }
        }

        // Title
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Dev Dialogue"
            font.family: Theme.fontSans
            font.pixelSize: 42
            font.weight: Font.ExtraBold
            color: Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
        }

        // Subtitle
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            text: "Платформа інтерактивної підготовки до технічних співбесід з адаптивним AI-інтерв'юером. Працює автономно або з підключенням до хмарного бекенду."
            font.family: Theme.fontSans
            font.pixelSize: 16
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Item { height: 16 }

        // Action Buttons
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 16

            Button {
                id: startBtn
                implicitHeight: 46
                implicitWidth: 200
                contentItem: Text {
                    text: "Почати співбесіду →"
                    font.family: Theme.fontSans
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radiusMd
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Theme.accentRose }
                        GradientStop { position: 1.0; color: Theme.accentIndigo }
                    }
                    scale: startBtn.down ? 0.97 : (startBtn.hovered ? 1.02 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 120 } }
                }
                onClicked: navVM.navigateToSetup()
            }

            Button {
                id: dashBtn
                implicitHeight: 46
                implicitWidth: 160
                contentItem: Text {
                    text: "Мій дашборд"
                    font.family: Theme.fontSans
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: Theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radiusMd
                    color: Theme.backgroundSecondary
                    border.color: Theme.borderSubtle
                    border.width: 1
                    scale: dashBtn.down ? 0.97 : (dashBtn.hovered ? 1.02 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 120 } }
                }
                onClicked: navVM.navigateToDashboard()
            }
        }
    }
}

