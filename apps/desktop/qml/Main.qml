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
    property bool debugOpen: false
    property bool localModelWarningOpen: false
    property bool localModelWarningDismissed: false
    property bool updateToastShown: false
    readonly property bool hideSidebarForChat: authVM.isAuthenticated
        && pageIndex === 3
        && chatRoomView.chatOpen
        && !accountOpen
    readonly property bool avatarSurfaceActive: authVM.isAuthenticated
        && pageIndex === 3
        && !localModelWarningOpen
        && (!backendVM.isOffline || avatarHost.ready)

    function toggleDebug() {
        debugOpen = !debugOpen
    }

    function openServicesManager() {
        backendVM.checkAllServices()
        localModelWarningDismissed = false
        localModelWarningOpen = true
    }

    function syncAvatarPreferences() {
        avatarHost.setVolume(chatVM.avatarVolume)
        avatarHost.setMuted(chatVM.avatarMuted)
        avatarHost.setOutputMode(chatVM.avatarOutputMode)
        avatarHost.setSoundEffect(chatVM.avatarSoundEffect)
        avatarHost.setVoice(chatVM.avatarVoice)
        avatarHost.setGender(chatVM.avatarGender)
        avatarHost.setPreset(chatVM.avatarPreset)
        avatarHost.setAccessory("Hat", chatVM.hatVisible)
        avatarHost.setAccessory("Glasses", chatVM.glassesVisible)
        avatarHost.setAccessory("Tie", chatVM.tieVisible)
        avatarHost.setAccessory("Horns", chatVM.hornsVisible)
        avatarHost.setAccessory("Halo", chatVM.haloVisible)
    }

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
            if (chatVM.hasActiveSession) {
                backendVM.ensureAllServicesRunning()
                if (!avatarHost.ready)
                    avatarHost.reload()
                navVM.navigateToChat(chatVM.interviewId)
            }
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
                rootWindow.localModelWarningOpen = false
                navVM.navigateToDashboard()
            } else {
                if (!authVM.isAdmin && navVM.currentViewIndex === 5)
                    navVM.navigateToDashboard()
                backendVM.refreshLocalModelStatus()
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
        backendVM.ensureAllServicesRunning()
        backendVM.checkForUpdates(true)
        avatarHost.attachToWindow(rootWindow, "")
    }

    Connections {
        target: avatarHost
        function onReadyChanged() {
            if (avatarHost.ready)
                rootWindow.syncAvatarPreferences()
        }
    }

    Connections {
        target: chatVM
        function onAvatarSpeakRequested(text, emotion, actionsSequence, uiCommands) {
            avatarHost.speak(text, emotion, actionsSequence, uiCommands, chatVM.avatarVoice, chatVM.avatarGender)
        }
        function onAvatarReactRequested(emotion, actionsSequence, uiCommands) {
            avatarHost.react(emotion, actionsSequence, uiCommands, chatVM.avatarGender)
        }
        function onAvatarSpeechStopRequested() {
            avatarHost.stopSpeech()
        }
        function onAvatarVolumeChanged() { avatarHost.setVolume(chatVM.avatarVolume) }
        function onAvatarMutedChanged() { avatarHost.setMuted(chatVM.avatarMuted) }
        function onAvatarOutputModeChanged() { avatarHost.setOutputMode(chatVM.avatarOutputMode) }
        function onAvatarSoundEffectChanged() { avatarHost.setSoundEffect(chatVM.avatarSoundEffect) }
        function onAvatarVoiceChanged() { avatarHost.setVoice(chatVM.avatarVoice) }
        function onAvatarGenderChanged() { avatarHost.setGender(chatVM.avatarGender) }
        function onAvatarPresetChanged() { avatarHost.setPreset(chatVM.avatarPreset) }
        function onAvatarWardrobeChanged() {
            avatarHost.setAccessory("Hat", chatVM.hatVisible)
            avatarHost.setAccessory("Glasses", chatVM.glassesVisible)
            avatarHost.setAccessory("Tie", chatVM.tieVisible)
            avatarHost.setAccessory("Horns", chatVM.hornsVisible)
            avatarHost.setAccessory("Halo", chatVM.haloVisible)
        }
    }

    Connections {
        target: backendVM
        function onStatusChanged() {
            if (!backendVM.isOffline) {
                backendVM.refreshLocalModelStatus()
                if (!avatarHost.ready)
                    avatarHost.reload()
            }
        }
        function onAvatarReloadRequested() {
            avatarHost.reload()
        }
        function onUpdateStateChanged() {
            if (backendVM.updateAvailable && !rootWindow.updateToastShown && !backendVM.isUpdating) {
                rootWindow.updateToastShown = true
                toast.show(
                    qsTr("Вийшла нова версія %1 (%2) — потрібно оновитись").arg(backendVM.latestVersion).arg(backendVM.latestCommit),
                    "info",
                    qsTr("Оновити"),
                    () => backendVM.installUpdate()
                )
            }
        }
        function onLocalModelChanged() {
            if (backendVM.localModelReady && backendVM.ollamaRunning && !backendVM.isOffline) {
                // Keep manual dialog open if user opened it explicitly, only auto-clear warning flag
                rootWindow.localModelWarningDismissed = false
            } else if (authVM.isAuthenticated && backendVM.localModelChecked && !backendVM.localModelReady && !rootWindow.localModelWarningDismissed) {
                rootWindow.localModelWarningOpen = true
            }
        }
    }

    Connections {
        target: setupVM
        function onLocalModelRequired() {
            rootWindow.localModelWarningDismissed = false
            rootWindow.localModelWarningOpen = true
        }
    }

    Connections {
        target: chatVM
        function onLocalModelRequired() {
            rootWindow.localModelWarningDismissed = false
            rootWindow.localModelWarningOpen = true
        }
    }

    Connections {
        target: reportVM
        function onPdfExported(filePath) {
            toast.show(qsTr("PDF report saved: %1").arg(filePath), "success", qsTr("Open"),
                       () => reportVM.openLastPdf())
        }
        function onPdfExportFailed(error) {
            toast.show(error, "error")
        }
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
    Shortcut {
        sequences: ["F12", "Ctrl+D"]
        onActivated: rootWindow.toggleDebug()
    }
    Shortcut {
        sequence: "Delete"
        // Allow Delete to work normally when typing inside a text field or text area
        enabled: !(rootWindow.activeFocusItem && (rootWindow.activeFocusItem.cursorVisible !== undefined))
        onActivated: rootWindow.toggleDebug()
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
            visible: !rootWindow.hideSidebarForChat
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            current: rootWindow.sidebarSection
            onNavigate: (target) => rootWindow.go(target)
            onOpenServicesRequested: rootWindow.openServicesManager()
        }

        Rectangle {
            id: contentPanel
            anchors.left: rootWindow.hideSidebarForChat ? shell.left : sidebar.right
            anchors.leftMargin: rootWindow.hideSidebarForChat ? 0 : Theme.windowGap
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
                ChatRoomView {
                    id: chatRoomView
                    avatarAllowed: rootWindow.avatarSurfaceActive
                    sidebarExpanded: sidebar.expanded
                    onDebugRequested: rootWindow.toggleDebug()
                }      // 3
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
    Connections {
        target: adminVM
        function onQuestionDeleted(topic) {
            toast.show(qsTr("Question \u201c%1\u201d deleted").arg(topic), "info", qsTr("Undo"),
                       () => adminVM.undoDelete())
        }
        function onQuestionDeleteFailed(topic, reason) {
            toast.show(reason !== ""
                       ? qsTr("\u201c%1\u201d was not deleted: %2").arg(topic).arg(reason)
                       : qsTr("\u201c%1\u201d was not deleted: the server could not be reached.").arg(topic),
                       "error")
        }
        function onDeletesSettled() {
            if (rootWindow.closeAfterDeletes)
                rootWindow.close()
        }
    }

    // A question deleted moments ago is still inside its undo window and the
    // server does not know yet. Closing sends the delete and waits for the
    // answer (a few seconds at most) so the question does not come back.
    property bool closeAfterDeletes: false
    onClosing: (close) => {
        if (!closeAfterDeletes && adminVM.sendPendingDeletes()) {
            close.accepted = false
            closeAfterDeletes = true
            closeAnyway.start()
        }
    }
    Timer {
        id: closeAnyway
        interval: 4000
        onTriggered: rootWindow.close()
    }

    LocalModelOverlay {
        open: rootWindow.localModelWarningOpen
        onCloseRequested: {
            rootWindow.localModelWarningDismissed = true
            rootWindow.localModelWarningOpen = false
        }
    }

    DebugOverlay {
        open: rootWindow.debugOpen
        dockRight: rootWindow.pageIndex === 3
        onCloseRequested: rootWindow.debugOpen = false
    }

    SplashOverlay {
        anchors.fill: parent
    }

    SystemMonitorWindow {}
}
