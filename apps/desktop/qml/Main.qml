import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Root window: navbar, backend status banner, the routed views, and the
// overlays (sign-in page, settings dialog, toast).
ApplicationWindow {
    id: rootWindow

    visible: true
    width: 1200
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    title: qsTr("Dev Dialogue — AI Interview Simulator")
    color: Theme.backgroundPrimary
    font.family: Theme.fontSans

    function openSettings(tab) {
        settingsDialog.openTab(tab)
    }

    Navbar {
        id: navbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 30
        onSettingsRequested: (tab) => rootWindow.openSettings(tab)
    }

    StatusBanner {
        id: statusBanner
        anchors.top: navbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        z: 9
    }

    // View routing: the index order matches NavigationViewModel::ViewIndex.
    StackLayout {
        id: viewStack
        anchors.top: statusBanner.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: navVM.currentViewIndex

        LandingView {}
        DashboardView {}
        SetupView {}
        ChatRoomView {}
        ReportView {}
        AdminView {}
    }

    LoginView {
        id: loginView
        anchors.top: statusBanner.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        z: 20
        visible: authVM.isLoginModalOpen
        onNotify: (message, tone) => toast.show(message, tone)
    }

    UserSettingsDialog {
        id: settingsDialog
    }

    Toast {
        id: toast
        z: 2000
    }

    // Sample content shown without a backend follows the interface language.
    Binding { target: reportVM; property: "uiLanguage"; value: I18n.language }
    Binding { target: adminVM; property: "uiLanguage"; value: I18n.language }

    // Messages from the auth layer that arrive while the sign-in page is
    // closed (e.g. "confirm your email") would otherwise go unseen.
    Connections {
        target: authVM
        function onErrorOccurred(error) {
            if (error !== "" && !authVM.isLoginModalOpen)
                toast.show(error, "info")
        }
    }
}
