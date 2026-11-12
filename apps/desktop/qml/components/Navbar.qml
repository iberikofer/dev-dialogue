import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Rectangle {
    id: navbarRoot

    readonly property var navItems: {
        const items = [
            { label: qsTr("Dashboard"), index: 1 },
            { label: qsTr("New interview"), index: 2 }
        ]
        if (authVM.isAdmin)
            items.push({ label: qsTr("Admin"), index: 5 })
        return items
    }

    // A demo session always runs on the embedded engine, no matter whether
    // the backend happens to be reachable.
    readonly property bool isOffline: authVM.isDemoSession || !apiClient.isOnline

    height: 64
    color: Theme.backgroundSecondary

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.borderSubtle
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 18

        // ---------------------------------------------------------------
        // Brand
        // ---------------------------------------------------------------
        RowLayout {
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
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
                    font.weight: Typography.weightBold
                    font.pixelSize: 13
                    color: "#ffffff"
                }
            }

            Text {
                text: qsTr("Dev Dialogue")
                font.family: Theme.fontSans
                font.weight: Typography.weightBold
                font.pixelSize: 16
                color: Theme.textPrimary
            }

            Rectangle {
                radius: 4
                color: Qt.rgba(0.95, 0.25, 0.37, 0.15)
                border.width: 1
                border.color: Qt.rgba(0.95, 0.25, 0.37, 0.3)
                implicitWidth: badgeText.implicitWidth + 10
                implicitHeight: 18

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: qsTr("DESKTOP")
                    font.family: Theme.fontMono
                    font.pixelSize: 9
                    font.weight: Typography.weightBold
                    color: Theme.accentRose
                }
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }

            TapHandler {
                onTapped: {
                    if (authVM.isAuthenticated)
                        navVM.navigateToDashboard()
                    else
                        navVM.navigateToLanding()
                }
            }
        }

        Item { Layout.fillWidth: true }

        // ---------------------------------------------------------------
        // View tabs
        // ---------------------------------------------------------------
        RowLayout {
            spacing: 4

            Repeater {
                model: navbarRoot.navItems

                delegate: Rectangle {
                    id: tab
                    required property var modelData

                    readonly property bool selected: navVM.currentViewIndex === modelData.index

                    width: tabLabel.implicitWidth + 24
                    height: 34
                    radius: Theme.radiusSm
                    color: tab.selected
                           ? Theme.backgroundElevated
                           : (tabHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
                    border.width: tab.selected ? 1 : 0
                    border.color: Theme.borderHighlight

                    Text {
                        id: tabLabel
                        anchors.centerIn: parent
                        text: tab.modelData.label
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.bodySmall
                        font.weight: tab.selected ? Typography.weightSemibold
                                                  : Typography.weightRegular
                        color: tab.selected ? Theme.textPrimary : Theme.textSecondary
                    }

                    HoverHandler {
                        id: tabHover
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        onTapped: navVM.currentViewIndex = tab.modelData.index
                    }

                    Accessible.role: Accessible.PageTab
                    Accessible.name: tab.modelData.label
                    Accessible.selected: tab.selected
                }
            }
        }

        Item { Layout.fillWidth: true }

        // ---------------------------------------------------------------
        // Language toggle
        // ---------------------------------------------------------------
        TactileButton {
            implicitWidth: 56
            implicitHeight: 32
            buttonRadius: Theme.radiusSm
            outlined: true
            text: i18n.isUkrainian ? "УК" : "EN"
            textColor: Theme.textSecondary
            onClicked: i18n.toggleLanguage()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Interface language: %1").arg(i18n.currentLanguageLabel)
        }

        // ---------------------------------------------------------------
        // Connection status + user
        // ---------------------------------------------------------------
        RowLayout {
            spacing: 6

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                Layout.alignment: Qt.AlignVCenter
                radius: 4
                color: navbarRoot.isOffline ? Theme.accentAmber : Theme.accentEmerald
            }

            Text {
                text: navbarRoot.isOffline ? qsTr("Offline mode") : qsTr("Online")
                font.family: Theme.fontMono
                font.pixelSize: Typography.tiny
                color: navbarRoot.isOffline ? Theme.accentAmber : Theme.accentEmerald
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 26
            Layout.alignment: Qt.AlignVCenter
            color: Theme.borderSubtle
        }

        RowLayout {
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignVCenter
                radius: 16
                color: Qt.rgba(0.39, 0.4, 0.95, 0.18)
                border.width: 1
                border.color: Qt.rgba(0.39, 0.4, 0.95, 0.4)

                Text {
                    anchors.centerIn: parent
                    text: authVM.userInitials
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.small
                    font.weight: Typography.weightSemibold
                    color: Theme.accentIndigo
                }
            }

            ColumnLayout {
                spacing: 0

                Text {
                    text: authVM.userFullName
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.small
                    font.weight: Typography.weightMedium
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.maximumWidth: 170
                }

                Text {
                    text: Labels.roleLabel(authVM.userRoleLabel)
                    font.family: Theme.fontMono
                    font.pixelSize: Typography.tiny
                    color: Theme.textMuted
                }
            }
        }

        TactileButton {
            text: qsTr("Sign out")
            outlined: true
            implicitWidth: 104
            implicitHeight: 32
            buttonRadius: Theme.radiusSm
            textColor: Theme.textSecondary
            iconSource: Icons.logout
            onClicked: authVM.logout()
        }
    }
}
