import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Top application bar (web Navbar.tsx): brand, primary navigation, admin
// view switch, backend status chip, language toggle and the profile menu
// or the Sign In button.
Rectangle {
    id: navbar

    // Emitted when a profile menu entry asks for the settings dialog.
    signal settingsRequested(string tab)

    readonly property bool showAdminFeatures: authVM.isAdmin && authVM.isAdminViewActive
    readonly property bool overlayOpen: authVM.isLoginModalOpen
    property bool menuOpen: false

    function go(navigate) {
        authVM.isLoginModalOpen = false
        menuOpen = false
        navigate()
    }

    implicitHeight: Theme.navbarHeight
    height: Theme.navbarHeight
    color: Theme.zinc950

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.borderGlass
    }

    RowLayout {
        id: bar
        width: Math.min(parent.width, Theme.contentWidthWide) - (parent.width >= 1024 ? 64 : 48)
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10

        BrandMark {
            onClicked: navbar.go(() => navVM.navigateToLanding())
        }

        Item { Layout.fillWidth: true }

        RowLayout {
            spacing: 4

            NavLink {
                text: qsTr("Dashboard")
                iconName: "layout-dashboard"
                active: !navbar.overlayOpen && navVM.currentViewIndex === 1
                onClicked: navbar.go(() => navVM.navigateToDashboard())
            }
            NavLink {
                text: qsTr("Interview")
                iconName: "circle-play"
                active: !navbar.overlayOpen && navVM.currentViewIndex >= 2 && navVM.currentViewIndex <= 4
                onClicked: navbar.go(() => navVM.navigateToSetup())
            }
            NavLink {
                visible: navbar.showAdminFeatures
                text: qsTr("Admin")
                iconName: "shield-alert"
                accent: true
                active: !navbar.overlayOpen && navVM.currentViewIndex === 5
                onClicked: navbar.go(() => navVM.navigateToAdmin())
            }
        }

        Item { Layout.fillWidth: true }

        // Admin / candidate view switch, real admins only.
        AbstractButton {
            id: viewSwitch
            visible: authVM.isAdmin
            implicitHeight: 26
            implicitWidth: viewSwitchRow.implicitWidth + 20
            hoverEnabled: true
            onClicked: authVM.toggleAdminView()
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: authVM.isAdminViewActive ? qsTr("Switch to Candidate View") : qsTr("Switch to Admin View")

            background: Rectangle {
                radius: Theme.radiusSm
                color: authVM.isAdminViewActive
                    ? Theme.alpha(Theme.blue950, viewSwitch.hovered ? 0.8 : 0.6)
                    : (viewSwitch.hovered ? Theme.zinc800 : Theme.alpha(Theme.zinc900, 0.9))
                border.width: 1
                border.color: authVM.isAdminViewActive ? Theme.alpha(Theme.blue500, 0.4) : Theme.borderGlass
                Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            }
            contentItem: Item {
                RowLayout {
                    id: viewSwitchRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon {
                        name: authVM.isAdminViewActive ? "shield-check" : "user-check"
                        size: 14
                        color: Theme.blue400
                    }
                    AppText {
                        text: authVM.isAdminViewActive ? qsTr("Admin") : qsTr("Candidate")
                        color: authVM.isAdminViewActive ? Theme.blue200 : Theme.zinc300
                        font.pixelSize: Typography.xs
                        font.weight: Typography.medium
                    }
                }
            }
        }

        // Backend status chip, only while the API is unreachable.
        AbstractButton {
            id: offlineChip
            visible: backendVM.isOffline
            implicitHeight: 30
            implicitWidth: offlineRow.implicitWidth + 20
            hoverEnabled: true
            onClicked: {
                backendVM.reopenBanner()
                backendVM.checkHealth()
            }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: qsTr("Backend offline (click to retry)")
            scale: down ? 0.97 : 1.0
            Behavior on scale { NumberAnimation { duration: Theme.durationFast } }

            background: Rectangle {
                radius: Theme.radiusMd
                color: Theme.alpha(Theme.amber500, offlineChip.hovered ? 0.2 : 0.1)
                border.width: 1
                border.color: Theme.alpha(Theme.amber500, offlineChip.hovered ? 0.5 : 0.35)
                Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            }
            contentItem: Item {
                RowLayout {
                    id: offlineRow
                    anchors.centerIn: parent
                    spacing: 6
                    PulseDot { size: 8 }
                    Icon { name: "server-off"; size: 14; color: Theme.amber400 }
                    MonoText {
                        text: qsTr("Backend offline")
                        color: Theme.amber300
                        font.pixelSize: Typography.tiny
                        font.weight: Typography.semibold
                    }
                }
            }
        }

        LanguageToggle {}

        // Signed-out state
        TactileButton {
            visible: !authVM.isAuthenticated
            text: qsTr("Sign In")
            iconLeft: "log-in"
            iconSize: 14
            fontSize: Typography.xs
            implicitHeight: 30
            leftPadding: 14
            rightPadding: 14
            glow: true
            onClicked: {
                navbar.menuOpen = false
                authVM.isLoginModalOpen = true
            }
        }

        // Signed-in state: name, role, avatar, chevron
        AbstractButton {
            id: profileButton
            visible: authVM.isAuthenticated
            Layout.leftMargin: 4
            implicitHeight: 46
            implicitWidth: profileRow.implicitWidth + 24
            hoverEnabled: true
            onClicked: navbar.menuOpen = !navbar.menuOpen
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            scale: down ? 0.97 : 1.0
            Behavior on scale { NumberAnimation { duration: Theme.durationFast } }

            background: Rectangle {
                radius: Theme.radiusMd
                color: profileButton.hovered ? Theme.alpha(Theme.zinc800, 0.9) : Theme.surfaceControl
                border.width: 1
                border.color: profileButton.visualFocus ? Theme.focusRing : (profileButton.hovered ? Theme.borderHover : Theme.borderStrong)
                Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            }
            contentItem: Item {
                RowLayout {
                    id: profileRow
                    anchors.centerIn: parent
                    spacing: 10

                    ColumnLayout {
                        spacing: 2
                        Layout.minimumWidth: 48
                        AppText {
                            Layout.alignment: Qt.AlignRight
                            Layout.maximumWidth: 160
                            text: settingsVM.displayName
                            font.pixelSize: Typography.xs
                            font.weight: Typography.semibold
                            elide: Text.ElideRight
                        }
                        MonoText {
                            Layout.alignment: Qt.AlignRight
                            visible: authVM.isAdmin
                            text: "Admin"
                            color: Theme.blue400
                            font.pixelSize: Typography.micro
                            font.weight: Typography.medium
                        }
                    }
                    UserAvatar {
                        avatarUrl: settingsVM.avatarUrl
                        highlighted: profileButton.hovered
                    }
                    Icon {
                        name: "chevron-down"
                        size: 14
                        color: navbar.menuOpen ? Theme.blue400 : (profileButton.hovered ? Theme.zinc200 : Theme.zinc400)
                        rotation: navbar.menuOpen ? 180 : 0
                        Behavior on rotation { NumberAnimation { duration: 200 } }
                    }
                }
            }
        }
    }

    // Click-away catcher for the profile menu (covers the whole window).
    MouseArea {
        visible: navbar.menuOpen
        z: 50
        width: navbar.width
        height: navbar.Window.height
        acceptedButtons: Qt.AllButtons
        onPressed: navbar.menuOpen = false
    }

    // Profile dropdown menu
    Rectangle {
        id: menu
        visible: navbar.menuOpen
        z: 51
        width: 256
        height: menuColumn.implicitHeight + 12
        x: Math.max(8, bar.x + bar.width - width)
        y: navbar.height - 4 + menuOffset
        property real menuOffset: 0
        radius: Theme.radiusLg
        color: Theme.zinc950
        border.width: 1
        border.color: Theme.borderStrong

        onVisibleChanged: if (visible) menuEntrance.restart()
        ParallelAnimation {
            id: menuEntrance
            NumberAnimation { target: menu; property: "opacity"; from: 0; to: 1; duration: Theme.durationEnter }
            NumberAnimation {
                target: menu; property: "menuOffset"; from: 8; to: 0
                duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
            }
        }

        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }

        component MenuRow: AbstractButton {
            id: menuRow
            property string iconName
            property color iconColor: Theme.blue400
            property bool destructive: false
            property string trailingTag

            Layout.fillWidth: true
            implicitHeight: 32
            hoverEnabled: true
            HoverHandler { cursorShape: Qt.PointingHandCursor }

            background: Rectangle {
                radius: Theme.radiusMd
                color: menuRow.hovered
                    ? (menuRow.destructive ? Theme.alpha(Theme.red950, 0.3) : Theme.zinc900)
                    : "transparent"
            }
            contentItem: RowLayout {
                spacing: 10
                Item { implicitWidth: 0 }
                Icon {
                    name: menuRow.iconName
                    size: 16
                    color: menuRow.destructive ? (menuRow.hovered ? Theme.red300 : Theme.red400) : menuRow.iconColor
                }
                AppText {
                    Layout.fillWidth: true
                    text: menuRow.text
                    color: menuRow.destructive
                        ? (menuRow.hovered ? Theme.red300 : Theme.red400)
                        : (menuRow.hovered ? Theme.textOnAccent : Theme.zinc300)
                    font.pixelSize: Typography.xs
                    font.weight: Typography.medium
                    elide: Text.ElideRight
                }
                Rectangle {
                    visible: menuRow.trailingTag !== ""
                    implicitWidth: tagText.implicitWidth + 12
                    implicitHeight: 20
                    radius: 4
                    color: Theme.alpha(Theme.blue950, 0.8)
                    border.width: 1
                    border.color: Theme.alpha(Theme.blue500, 0.3)
                    MonoText {
                        id: tagText
                        anchors.centerIn: parent
                        text: menuRow.trailingTag
                        color: Theme.blue300
                        font.pixelSize: Typography.micro
                    }
                }
                Item { implicitWidth: 0 }
            }
        }

        ColumnLayout {
            id: menuColumn
            x: 6
            y: 6
            width: parent.width - 12
            spacing: 2

            // Header: avatar, name, email, role
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                spacing: 10

                UserAvatar {
                    size: 36
                    glyphSize: 20
                    avatarUrl: settingsVM.avatarUrl
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    AppText {
                        Layout.fillWidth: true
                        text: settingsVM.displayName
                        font.pixelSize: Typography.xs
                        font.weight: Typography.semibold
                        elide: Text.ElideRight
                    }
                    AppText {
                        Layout.fillWidth: true
                        text: settingsVM.email
                        color: Theme.zinc400
                        font.pixelSize: Typography.tiny
                        elide: Text.ElideRight
                    }
                    MonoText {
                        text: authVM.isAdmin ? qsTr("Admin") : qsTr("Candidate")
                        caps: true
                        color: Theme.blue400
                        font.pixelSize: Typography.micro
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.bottomMargin: 2; implicitHeight: 1; color: Theme.borderFaint }

            MenuRow {
                text: qsTr("Profile & Account")
                iconName: "user"
                onClicked: { navbar.menuOpen = false; navbar.settingsRequested("profile") }
            }
            MenuRow {
                text: qsTr("Platform Settings")
                iconName: "settings"
                iconColor: Theme.indigo400
                onClicked: { navbar.menuOpen = false; navbar.settingsRequested("site") }
            }
            MenuRow {
                text: qsTr("My Interviews")
                iconName: "layout-dashboard"
                iconColor: Theme.amber400
                onClicked: navbar.go(() => navVM.navigateToDashboard())
            }
            MenuRow {
                text: qsTr("New Interview")
                iconName: "circle-play"
                iconColor: Theme.rose400
                onClicked: navbar.go(() => navVM.navigateToSetup())
            }
            MenuRow {
                text: qsTr("Switch Role (Demo)")
                iconName: "sparkles"
                trailingTag: authVM.isAdmin ? "→ User" : "→ Admin"
                onClicked: authVM.mockLogin(authVM.isAdmin ? "candidate" : "admin")
            }
            MenuRow {
                text: qsTr("Help & FAQ")
                iconName: "circle-help"
                iconColor: Theme.zinc400
                onClicked: {
                    navbar.menuOpen = false
                    Qt.openUrlExternally("https://github.com/iberikofer/dev-dialogue")
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.topMargin: 2; Layout.bottomMargin: 2; implicitHeight: 1; color: Theme.borderFaint }

            MenuRow {
                text: qsTr("Sign Out")
                iconName: "log-out"
                destructive: true
                onClicked: {
                    navbar.menuOpen = false
                    authVM.logout()
                    navVM.navigateToLanding()
                }
            }
        }
    }
}
