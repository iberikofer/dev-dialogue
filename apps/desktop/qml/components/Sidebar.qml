pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Application sidebar: a floating glass panel that is either a narrow icon
// rail or an expanded list. Every entry leads to a working screen; entries
// that need a role or a running session only appear when they apply.
Rectangle {
    id: sidebar

    property bool expanded: true
    // Which entry is highlighted: "home", "setup", "session", "admin", "account".
    property string current
    signal navigate(string target)

    readonly property bool showAdmin: authVM.isAdmin && authVM.isAdminViewActive

    implicitWidth: expanded ? Theme.sidebarExpanded : Theme.sidebarCollapsed
    width: implicitWidth
    Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

    radius: Theme.radiusXl
    border.width: 1
    border.color: Theme.borderGlass
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#17171b" }
        GradientStop { position: 1.0; color: "#101013" }
    }
    clip: true

    // Indicator bar on the panel edge next to the active entry.
    Rectangle {
        id: indicator
        x: 0
        width: 3
        height: 20
        radius: 2
        color: Theme.blue400
        visible: sidebar.activeItem !== null
        // Entries are direct children of the column, which sits 12px in.
        y: sidebar.activeItem ? 12 + sidebar.activeItem.y + (sidebar.activeItem.height - height) / 2 : 0
        Behavior on y { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        SoftGlow {
            anchors.fill: parent
            color: Theme.blue400
            radius: 2
            blur: 10
            offsetY: 0
            strength: 0.9
        }
    }
    property Item activeItem: {
        switch (current) {
        case "home": return homeItem
        case "setup": return setupItem
        case "session": return sessionItem.visible ? sessionItem : null
        case "admin": return adminItem.visible ? adminItem : null
        case "account": return accountItem
        default: return null
        }
    }

    component Separator: Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 10
        Layout.bottomMargin: 10
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        implicitHeight: 1
        color: Theme.borderFaint
    }
    component GroupLabel: MonoText {
        Layout.leftMargin: 14
        Layout.bottomMargin: 6
        visible: sidebar.expanded
        caps: true
        color: Theme.textMuted
        font.pixelSize: Typography.micro
        font.weight: Typography.semibold
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4

        // ------------------------------------------------------ App mark
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            spacing: 10

            Image {
                Layout.leftMargin: sidebar.expanded ? 8 : (parent.width - width) / 2
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                source: Qt.resolvedUrl("../../resources/app/app-icon.svg")
                sourceSize.width: 60
                sourceSize.height: 60
                smooth: true
            }
            AppText {
                Layout.fillWidth: true
                visible: sidebar.expanded
                text: "Dev Dialogue"
                font.pixelSize: Typography.base
                font.weight: Typography.semibold
                elide: Text.ElideRight
            }
        }

        Separator {}

        // ----------------------------------------------------- Workspace
        GroupLabel { text: qsTr("Workspace") }

        SidebarItem {
            id: homeItem
            Layout.fillWidth: true
            expanded: sidebar.expanded
            text: qsTr("Home")
            iconName: "layout-dashboard"
            shortcutHint: "Ctrl+1"
            active: sidebar.current === "home"
            onClicked: sidebar.navigate("home")
        }
        SidebarItem {
            id: setupItem
            Layout.fillWidth: true
            expanded: sidebar.expanded
            text: qsTr("New interview")
            iconName: "circle-plus"
            shortcutHint: "Ctrl+N"
            active: sidebar.current === "setup"
            onClicked: sidebar.navigate("setup")
        }
        SidebarItem {
            id: sessionItem
            Layout.fillWidth: true
            visible: chatVM.hasActiveSession
            expanded: sidebar.expanded
            text: qsTr("Current interview")
            iconName: "bot"
            shortcutHint: "Ctrl+3"
            live: true
            badgeCount: chatVM.candidateAnswersCount
            active: sidebar.current === "session"
            onClicked: sidebar.navigate("session")
        }

        // ---------------------------------------------------- Administration
        Separator { visible: sidebar.showAdmin }
        GroupLabel { visible: sidebar.showAdmin && sidebar.expanded; text: qsTr("Administration") }
        SidebarItem {
            id: adminItem
            Layout.fillWidth: true
            visible: sidebar.showAdmin
            expanded: sidebar.expanded
            text: qsTr("Question bank")
            iconName: "shield-check"
            active: sidebar.current === "admin"
            onClicked: sidebar.navigate("admin")
        }

        Item { Layout.fillHeight: true }

        // ------------------------------------------------- Backend status
        AbstractButton {
            id: status
            Layout.fillWidth: true
            implicitHeight: 34
            hoverEnabled: true
            enabled: !backendVM.isChecking
            onClicked: backendVM.checkHealth()
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            ToolTip.visible: hovered
            ToolTip.delay: 300
            ToolTip.text: backendVM.isOffline
                ? qsTr("Server unreachable: showing demo data. Click to check again.")
                : qsTr("Connected to the server. Click to check again.")

            background: Rectangle {
                radius: height / 2
                color: status.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent"
            }
            contentItem: Item {
                PulseDot {
                    id: statusDot
                    x: sidebar.expanded ? 20 : (parent.width - width) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    size: 8
                    color: backendVM.isOffline ? Theme.amber500 : Theme.emerald500
                    pingColor: backendVM.isOffline ? Theme.amber400 : Theme.emerald400
                    pinging: backendVM.isOffline || backendVM.isChecking
                }
                MonoText {
                    anchors.left: statusDot.right
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    visible: sidebar.expanded
                    text: backendVM.isChecking ? qsTr("Checking...")
                        : backendVM.isOffline ? qsTr("Offline · demo") : qsTr("Online")
                    color: backendVM.isOffline ? Theme.amber300 : Theme.zinc400
                    font.pixelSize: Typography.tiny
                }
            }
        }

        Separator {}

        // ------------------------------------------------------- Account
        SidebarItem {
            id: accountItem
            Layout.fillWidth: true
            implicitHeight: 52
            expanded: sidebar.expanded
            text: settingsVM.displayName
            iconName: "user"
            shortcutHint: "Ctrl+,"
            active: sidebar.current === "account"
            onClicked: sidebar.navigate("account")
        }

        // Collapse / expand
        AbstractButton {
            id: collapse
            Layout.fillWidth: true
            implicitHeight: 32
            hoverEnabled: true
            onClicked: sidebar.expanded = !sidebar.expanded
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            ToolTip.visible: hovered
            ToolTip.delay: 300
            ToolTip.text: (sidebar.expanded ? qsTr("Collapse sidebar") : qsTr("Expand sidebar")) + "  ·  Ctrl+B"

            background: Rectangle {
                radius: height / 2
                color: collapse.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent"
            }
            contentItem: Item {
                Icon {
                    x: sidebar.expanded ? 16 : (parent.width - width) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron-right"
                    size: 16
                    color: collapse.hovered ? Theme.zinc200 : Theme.textMuted
                    rotation: sidebar.expanded ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 220 } }
                }
                AppText {
                    x: 44
                    anchors.verticalCenter: parent.verticalCenter
                    visible: sidebar.expanded
                    text: qsTr("Collapse")
                    color: Theme.textMuted
                    font.pixelSize: Typography.xs
                }
            }
        }
    }
}
