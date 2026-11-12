import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: landingRoot

    TactileButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 24
        implicitWidth: 64
        implicitHeight: 32
        buttonRadius: Theme.radiusSm
        outlined: true
        text: i18n.isUkrainian ? "УК" : "EN"
        textColor: Theme.textSecondary
        onClicked: i18n.toggleLanguage()

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Interface language: %1").arg(i18n.currentLanguageLabel)
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 22
        width: Math.min(760, parent.width - 64)

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            radius: 100
            color: Qt.rgba(0.39, 0.4, 0.95, 0.14)
            border.color: Qt.rgba(0.39, 0.4, 0.95, 0.34)
            implicitWidth: pillRow.implicitWidth + 24
            implicitHeight: 30

            RowLayout {
                id: pillRow
                anchors.centerIn: parent
                spacing: 8

                Image {
                    source: Icons.sparkles
                    sourceSize.width: 14
                    sourceSize.height: 14
                    Layout.preferredWidth: 14
                    Layout.preferredHeight: 14
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    text: qsTr("Native C++ Desktop Edition")
                    font.family: Theme.fontMono
                    font.pixelSize: Typography.small
                    font.weight: Typography.weightMedium
                    color: Theme.accentIndigo
                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Dev Dialogue")
            font.family: Theme.fontSans
            font.pixelSize: 46
            font.weight: Typography.weightBold
            color: Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            text: qsTr("Practise technical interviews with an adaptive AI interviewer. Run fully offline with the embedded engine, or connect to the Dev Dialogue backend.")
            font.family: Theme.fontSans
            font.pixelSize: Typography.bodyLarge
            lineHeight: Typography.lineNormal
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Item { Layout.preferredHeight: 6 }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 14

            TactileButton {
                implicitWidth: 210
                implicitHeight: 48
                text: qsTr("Start an interview")
                onClicked: {
                    if (authVM.isAuthenticated)
                        navVM.navigateToSetup()
                    else
                        authVM.isLoginModalOpen = true
                }
            }

            TactileButton {
                implicitWidth: 150
                implicitHeight: 48
                text: authVM.isAuthenticated ? qsTr("Dashboard") : qsTr("Sign in")
                outlined: true
                onClicked: {
                    if (authVM.isAuthenticated)
                        navVM.navigateToDashboard()
                    else
                        authVM.isLoginModalOpen = true
                }
            }
        }

        Item { Layout.preferredHeight: 8 }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 22

            Repeater {
                model: [
                    { label: qsTr("5 interview tracks"), icon: Icons.dashboard },
                    { label: qsTr("Works offline"), icon: Icons.shield },
                    { label: qsTr("Instant scorecard"), icon: Icons.chart }
                ]

                delegate: RowLayout {
                    required property var modelData
                    spacing: 8

                    Image {
                        source: modelData.icon
                        sourceSize.width: 15
                        sourceSize.height: 15
                        Layout.preferredWidth: 15
                        Layout.preferredHeight: 15
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.8
                    }

                    Text {
                        text: modelData.label
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }
                }
            }
        }
    }
}
