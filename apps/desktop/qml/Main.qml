import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import DevDialogue.App

ApplicationWindow {
    id: rootWindow

    visible: true
    width: 1280
    height: 860
    minimumWidth: 1024
    minimumHeight: 720
    title: qsTr("Dev Dialogue — AI Interview Simulator")
    color: Theme.backgroundPrimary

    // A restored session is already authenticated before QML loads, so the
    // landing screen has to be skipped explicitly.
    Component.onCompleted: {
        if (authVM.isAuthenticated)
            navVM.navigateToDashboard()
    }

    // Atmospheric backdrop: a subtle vertical wash instead of flat black.
    Rectangle {
        anchors.fill: parent

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0c0c12" }
            GradientStop { position: 0.55; color: "#0a0a0e" }
            GradientStop { position: 1.0; color: Theme.backgroundPrimary }
        }
    }

    Navbar {
        id: navbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: authVM.isAuthenticated
        z: 10
    }

    StatusBanner {
        id: offlineBanner
        anchors.top: navbar.visible ? navbar.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: authVM.isAuthenticated
                 && (authVM.isDemoSession || !apiClient.isOnline)
        message: authVM.isDemoSession
                 ? qsTr("This is a demo session. Dev Dialogue runs the embedded offline interviewer and a local question bank.")
                 : qsTr("The backend is unreachable. Dev Dialogue is using the embedded offline interviewer and local question bank.")
        onRetryRequested: apiClient.performHealthCheck()
        z: 9
    }

    StackLayout {
        id: viewStack
        anchors.top: authVM.isAuthenticated
                     ? (offlineBanner.visible ? offlineBanner.bottom : navbar.bottom)
                     : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: navVM.currentViewIndex

        LandingView {}
        DashboardView {}
        SetupView {}
        ChatRoomView {}
        ReportView {}
        AdminView {}
    }

    Connections {
        target: authVM

        function onAuthStateChanged() {
            if (authVM.isAuthenticated) {
                if (navVM.currentViewIndex === 0)
                    navVM.navigateToDashboard()
            } else {
                authVM.isLoginModalOpen = false
                navVM.navigateToLanding()
            }
        }
    }

    Connections {
        target: navVM

        function onCurrentViewIndexChanged(newIndex) {
            // The admin screen is only reachable for an administrator with
            // the admin view enabled.
            if (newIndex === 5 && !(authVM.isAdmin && authVM.isAdminViewActive))
                navVM.navigateToDashboard()
        }
    }

    // -------------------------------------------------------------------
    // Sign-in modal
    // -------------------------------------------------------------------
    AppModal {
        id: loginModal
        anchors.fill: parent
        z: 200
        visible: authVM.isLoginModalOpen
        title: qsTr("Welcome to Dev Dialogue")
        modalWidth: 480
        contentSpacing: 12
        onClosed: authVM.isLoginModalOpen = false

        Text {
            width: parent.width
            text: qsTr("Sign in with the web identity provider, or jump straight into a demo session. Demo sessions run entirely offline.")
            font.family: Theme.fontSans
            font.pixelSize: Typography.bodySmall
            lineHeight: Typography.lineNormal
            color: Theme.textSecondary
            wrapMode: Text.Wrap
        }

        DarkTextField {
            id: loginEmail
            width: parent.width
            placeholderText: qsTr("Email address")
            inputMethodHints: Qt.ImhEmailCharactersOnly
        }

        DarkTextField {
            id: loginPassword
            width: parent.width
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
        }

        ErrorBanner {
            width: parent.width
            errorMessage: authVM.errorMessage
            onDismissed: authVM.clearError()
        }

        RowLayout {
            width: parent.width
            spacing: 10

            Item { Layout.fillWidth: true }

            TactileButton {
                text: qsTr("Sign in")
                implicitWidth: 130
                enabled: loginEmail.text.trim().length > 0
                         && loginPassword.text.length > 0
                onClicked: authVM.loginWithEmail(loginEmail.text.trim(),
                                                 loginPassword.text)
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.borderSubtle
        }

        Text {
            width: parent.width
            text: qsTr("Continue without an account")
            font.family: Theme.fontMono
            font.pixelSize: Typography.tiny
            color: Theme.textMuted
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            width: parent.width
            spacing: 10

            TactileButton {
                Layout.fillWidth: true
                text: qsTr("Candidate demo")
                outlined: true
                onClicked: authVM.mockLogin("candidate")
            }

            TactileButton {
                Layout.fillWidth: true
                text: qsTr("Admin demo")
                outlined: true
                onClicked: authVM.mockLogin("admin")
            }
        }

        Text {
            width: parent.width
            text: qsTr("Email and password sign-in is delegated to the web identity provider. Use a demo session to explore every desktop screen.")
            font.family: Theme.fontSans
            font.pixelSize: Typography.tiny
            lineHeight: Typography.lineNormal
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
    }
}
