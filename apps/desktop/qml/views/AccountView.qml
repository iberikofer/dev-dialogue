pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Account: who is signed in, the admin view switch, the server connection,
// keyboard shortcuts and sign out. Everything here acts on real state.
AppPage {
    id: page

    active: StackLayout.isCurrentItem
    title: qsTr("Account")
    subtitle: qsTr("Your profile, connection and shortcuts.")
    scrollable: true

    component Section: DarkCard {
        id: section
        default property alias rows: sectionColumn.data
        property string heading
        Layout.fillWidth: true
        Layout.maximumWidth: 760
        padding: 24

        ColumnLayout {
            id: sectionColumn
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 14

            MonoText {
                caps: true
                text: section.heading
                font.weight: Typography.semibold
            }
        }
    }

    // ------------------------------------------------------------ Profile
    Section {
        heading: qsTr("Profile")

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            UserAvatar {
                size: 56
                glyphSize: 30
                avatarUrl: settingsVM.avatarUrl
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                RowLayout {
                    spacing: 10
                    AppText {
                        text: settingsVM.displayName
                        font.pixelSize: Typography.lg
                        font.weight: Typography.semibold
                    }
                    Badge {
                        tone: settingsVM.role === "admin" ? "blue" : "neutral"
                        mono: true
                        caps: true
                        text: settingsVM.role === "admin" ? qsTr("Admin") : qsTr("Candidate")
                        fontSize: Typography.tiny
                    }
                    Badge {
                        visible: settingsVM.isDemoAccount
                        tone: "amber"
                        mono: true
                        text: qsTr("Demo account")
                        fontSize: Typography.tiny
                    }
                }
                AppText {
                    text: settingsVM.email
                    color: Theme.zinc400
                }
            }
        }

        // Real admins can work as a candidate without signing out.
        RowLayout {
            visible: authVM.isAdmin
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                AppText { text: qsTr("Admin view"); font.weight: Typography.medium }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Shows the question bank in the sidebar. Turn off to use the app as a candidate.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }
            }
            ToggleSwitch {
                checked: authVM.isAdminViewActive
                onToggled: authVM.toggleAdminView()
            }
        }

        // Demo accounts can swap roles to see both sides of the app.
        RowLayout {
            visible: settingsVM.isDemoAccount
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                AppText { text: qsTr("Demo role"); font.weight: Typography.medium }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Switch between the candidate and the administrator demo.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }
            }
            TactileButton {
                variant: "secondary"
                text: authVM.isAdmin ? qsTr("Switch to candidate") : qsTr("Switch to admin")
                fontSize: Typography.xs
                implicitHeight: 34
                onClicked: authVM.mockLogin(authVM.isAdmin ? "candidate" : "admin")
            }
        }
    }

    // ---------------------------------------------------------- Connection
    Section {
        heading: qsTr("Server")

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            PulseDot {
                size: 10
                color: backendVM.isOffline ? Theme.amber500 : Theme.emerald500
                pingColor: backendVM.isOffline ? Theme.amber400 : Theme.emerald400
                pinging: backendVM.isChecking
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                AppText {
                    text: backendVM.isOffline ? qsTr("Offline") : qsTr("Connected")
                    font.weight: Typography.medium
                }
                AppText {
                    Layout.fillWidth: true
                    text: backendVM.isOffline
                        ? qsTr("The server can't be reached. Screens show demo data until it is back.")
                        : qsTr("Interviews, scores and the question bank are synced with the server.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }
            }
            TactileButton {
                variant: "secondary"
                text: backendVM.isChecking ? qsTr("Checking...") : qsTr("Check now")
                iconLeft: "refresh-cw"
                iconSize: 14
                fontSize: Typography.xs
                implicitHeight: 34
                enabled: !backendVM.isChecking
                onClicked: backendVM.checkHealth()
            }
        }
    }

    // ----------------------------------------------------------- Shortcuts
    Section {
        heading: qsTr("Keyboard shortcuts")

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 32
            rowSpacing: 10

            Repeater {
                model: [
                    { keys: "Ctrl+1", action: qsTr("Home") },
                    { keys: "Ctrl+N", action: qsTr("New interview") },
                    { keys: "Ctrl+3", action: qsTr("Current interview") },
                    { keys: "Ctrl+,", action: qsTr("Account") },
                    { keys: "Ctrl+B", action: qsTr("Collapse or expand the sidebar") },
                    { keys: "Enter", action: qsTr("Send an answer") },
                    { keys: "Shift+Enter", action: qsTr("New line in an answer") }
                ]
                RowLayout {
                    id: shortcutRow
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 12
                    Rectangle {
                        implicitWidth: 104
                        implicitHeight: 26
                        radius: Theme.radiusXs
                        color: Theme.zinc900
                        border.width: 1
                        border.color: Theme.borderStrong
                        MonoText {
                            anchors.centerIn: parent
                            text: shortcutRow.modelData.keys
                            color: Theme.zinc200
                            font.pixelSize: Typography.tiny
                        }
                    }
                    AppText {
                        Layout.fillWidth: true
                        text: shortcutRow.modelData.action
                        color: Theme.zinc300
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------ Sign out
    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: 760
        spacing: 12

        MonoText {
            Layout.fillWidth: true
            text: "Dev Dialogue " + Qt.application.version
            color: Theme.textMuted
        }
        TactileButton {
            variant: "danger"
            text: qsTr("Sign out")
            iconLeft: "log-out"
            iconSize: 14
            fontSize: Typography.xs
            implicitHeight: 36
            onClicked: authVM.logout()
        }
    }
}
