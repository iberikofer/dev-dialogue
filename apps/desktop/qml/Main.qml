import QtCore
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Application window. Signed out it shows the sign-in screen; signed in it
// is a sidebar plus the current screen in a rounded content panel. A short
// intro plays on start-up while the saved session is restored.
ApplicationWindow {
    id: rootWindow

    visible: true
    width: 1280
    height: 820
    minimumWidth: 1024
    minimumHeight: 680
    title: "Dev Dialogue"
    color: Theme.zinc950
    font.family: Theme.fontSans

    // The account page is not part of NavigationViewModel; it is a local
    // overlay page that closes whenever the view model navigates.
    property bool accountOpen: false

    // Stack index per NavigationViewModel::ViewIndex. Landing (0) has no
    // screen in the app and shows Home.
    readonly property int pageIndex: accountOpen ? 6
        : (navVM.currentViewIndex === 0 ? 1 : navVM.currentViewIndex)

    readonly property string sidebarSection: {
        if (accountOpen) return "account"
        switch (pageIndex) {
        case 1: return "home"
        case 2: return "setup"
        case 3: return "session"
        case 4: return "home"
        case 5: return "admin"
        default: return ""
        }
    }

    function go(target) {
        accountOpen = target === "account"
        switch (target) {
        case "home": navVM.navigateToDashboard(); break
        case "setup": navVM.navigateToSetup(); break
        case "session":
            if (chatVM.hasActiveSession)
                navVM.navigateToChat(chatVM.interviewId)
            break
        case "admin":
            if (authVM.isAdmin && authVM.isAdminViewActive)
                navVM.navigateToAdmin()
            break
        }
    }

    Connections {
        target: navVM
        function onCurrentViewIndexChanged() { rootWindow.accountOpen = false }
    }

    // Leaving the admin view (or losing the role) must not leave the
    // question bank on screen.
    Connections {
        target: authVM
        function onAdminViewChanged() {
            if (!authVM.isAdminViewActive && navVM.currentViewIndex === 5)
                navVM.navigateToDashboard()
        }
        function onAuthStateChanged() {
            if (!authVM.isAuthenticated) {
                rootWindow.accountOpen = false
                navVM.navigateToDashboard()
            } else if (!authVM.isAdmin && navVM.currentViewIndex === 5) {
                navVM.navigateToDashboard()
            }
        }
        function onErrorOccurred(error) {
            if (error !== "" && authVM.isAuthenticated)
                toast.show(error, "info")
        }
    }

    // Window size and sidebar state survive restarts.
    Settings {
        category: "window"
        property alias width: rootWindow.width
        property alias height: rootWindow.height
        property alias sidebarExpanded: sidebar.expanded
    }

    // Sample content shown without a backend follows the interface language.
    Binding { target: reportVM; property: "uiLanguage"; value: I18n.language }
    Binding { target: adminVM; property: "uiLanguage"; value: I18n.language }

    Component.onCompleted: {
        if (navVM.currentViewIndex === 0)
            navVM.navigateToDashboard()
    }

    // ---------------------------------------------------------- Shortcuts
    Shortcut {
        sequence: "Ctrl+1"
        enabled: authVM.isAuthenticated
        onActivated: rootWindow.go("home")
    }
    Shortcut {
        sequences: ["Ctrl+N", "Ctrl+2"]
        enabled: authVM.isAuthenticated
        onActivated: rootWindow.go("setup")
    }
    Shortcut {
        sequence: "Ctrl+3"
        enabled: authVM.isAuthenticated && chatVM.hasActiveSession
        onActivated: rootWindow.go("session")
    }
    Shortcut {
        sequence: "Ctrl+,"
        enabled: authVM.isAuthenticated
        onActivated: rootWindow.go("account")
    }
    Shortcut {
        sequence: "Ctrl+B"
        enabled: authVM.isAuthenticated
        onActivated: sidebar.expanded = !sidebar.expanded
    }

    // ---------------------------------------------------------- Backdrop
    GlowBlob {
        width: 900
        height: 520
        x: parent.width * 0.55 - width / 2
        y: -300
        color: Theme.blue600
        strength: 0.10
    }

    // ------------------------------------------------------------- Shell
    Item {
        id: shell
        anchors.fill: parent
        anchors.margins: Theme.windowGap
        visible: authVM.isAuthenticated

        Sidebar {
            id: sidebar
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            current: rootWindow.sidebarSection
            onNavigate: (target) => rootWindow.go(target)
        }

        Rectangle {
            id: contentPanel
            anchors.left: sidebar.right
            anchors.leftMargin: Theme.windowGap
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: Theme.radiusXl
            color: "#0e0e11"
            border.width: 1
            border.color: Theme.borderGlass
            clip: true

            StackLayout {
                id: pages
                anchors.fill: parent
                anchors.margins: 1
                currentIndex: rootWindow.pageIndex

                Item {}              // 0: Landing, not used in the app
                DashboardView {}     // 1
                SetupView {}         // 2
                ChatRoomView {}      // 3
                ReportView {}        // 4
                AdminView {}         // 5
                AccountView {}       // 6: local page, see accountOpen
            }
        }
    }

    LoginView {
        anchors.fill: parent
        visible: !authVM.isAuthenticated
        onNotify: (message, tone) => toast.show(message, tone)
    }

    Toast {
        id: toast
        z: 2000
    }

    SplashOverlay {
        anchors.fill: parent
    }
}
