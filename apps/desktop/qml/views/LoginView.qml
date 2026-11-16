pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Sign-in screen shown while nobody is signed in: a compact sign-in
// window with the email form and the demo access.
Item {
    id: view

    // Messages that should outlive the screen (shown as a toast).
    signal notify(string message, string tone)

    property string mode: "signin"
    property string localError
    property string successMessage
    readonly property bool signUp: mode === "signup"
    readonly property string errorText: localError !== "" ? localError : authVM.errorMessage

    function resetMessages() {
        localError = ""
        successMessage = ""
    }
    function switchMode(newMode) {
        mode = newMode
        resetMessages()
    }
    function submit() {
        resetMessages()
        const email = emailField.text.trim()
        const password = passwordField.text
        if (email === "" || password === "") {
            localError = qsTr("Enter your email and password.")
            return
        }
        if (!validators.isValidEmail(email)) {
            localError = qsTr("Please enter a valid email address.")
            return
        }
        if (signUp) {
            const nickname = nicknameField.text.trim()
            if (!validators.isValidNickname(nickname)) {
                localError = qsTr("Nickname must be at least 3 characters: English letters, digits or underscores.")
                return
            }
            if (!validators.isValidPassword(password)) {
                localError = qsTr("Password must be 8 to 72 characters and contain letters and digits.")
                return
            }
            if (password !== confirmField.text) {
                localError = qsTr("Passwords do not match.")
                return
            }
            authVM.registerUser(email, password, nickname)
        } else {
            authVM.loginWithEmail(email, password)
        }
    }
    function demoLogin(role) {
        resetMessages()
        authVM.mockLogin(role)
        if (role === "admin" && !authVM.isAdmin)
            view.notify(qsTr("Signed in as a candidate: the administrator demo is disabled in this build."), "info")
    }

    onVisibleChanged: {
        if (visible) {
            resetMessages()
            passwordField.text = ""
            confirmField.text = ""
            emailField.forceActiveFocus()
        }
    }

    Connections {
        target: authVM
        function onAuthStateChanged() {
            if (authVM.isAuthenticated)
                navVM.navigateToDashboard()
        }
    }

    Rectangle { anchors.fill: parent; color: Theme.zinc950 }

    // Input under the screen must not react while it is shown.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
    }

    // A sign-in window, not a landing page: the app mark, one form and the
    // demo access, centred in the window. Scrolls if the window is short.
    Flickable {
        id: scroller
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, column.implicitHeight + 96)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ThemedScrollBar {}

        ColumnLayout {
            id: column
            width: 400
            x: (scroller.width - width) / 2
            y: Math.max(48, (scroller.contentHeight - implicitHeight) / 2)
            spacing: 0

            // -------------------------------------------------- App mark
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                Image {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    source: Qt.resolvedUrl("../../resources/app/app-icon.svg")
                    sourceSize.width: 80
                    sourceSize.height: 80
                    smooth: true
                }
                AppText {
                    text: "Dev Dialogue"
                    font.pixelSize: Typography.xl
                    font.weight: Typography.semibold
                }
            }
            AppText {
                Layout.topMargin: 8
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Technical interview practice with an AI interviewer")
                color: Theme.zinc400
                wrapMode: Text.WordWrap
            }

            // ------------------------------------------------------ Form
            Rectangle {
                Layout.topMargin: 28
                Layout.fillWidth: true
                implicitHeight: form.implicitHeight + 64
                radius: Theme.radiusXl
                color: "#0e0e11"
                border.width: 1
                border.color: Theme.borderGlass

                ColumnLayout {
                    id: form
                    x: 32
                    y: 32
                    width: parent.width - 64
                    spacing: 0

                    AppText {
                        Layout.fillWidth: true
                        text: view.signUp ? qsTr("Create an account") : qsTr("Sign in")
                        font.pixelSize: Typography.xl
                        font.weight: Typography.bold
                    }
                    AppText {
                        Layout.topMargin: 6
                        Layout.fillWidth: true
                        text: view.signUp ? qsTr("Your scores and history are kept with your account.")
                                          : qsTr("Continue where you left off.")
                        color: Theme.zinc400
                        wrapMode: Text.WordWrap
                    }

                    SegmentedControl {
                        Layout.topMargin: 20
                        Layout.fillWidth: true
                        model: [
                            { value: "signin", label: qsTr("Sign in") },
                            { value: "signup", label: qsTr("Sign up") }
                        ]
                        currentValue: view.mode
                        onActivated: (value) => view.switchMode(value)
                    }

                    AlertBox {
                        visible: view.errorText !== ""
                        Layout.topMargin: 20
                        Layout.fillWidth: true
                        text: view.errorText
                    }
                    AlertBox {
                        visible: view.successMessage !== ""
                        Layout.topMargin: 20
                        Layout.fillWidth: true
                        tone: "success"
                        text: view.successMessage
                    }

                    component FieldLabel: AppText {
                        Layout.fillWidth: true
                        Layout.topMargin: 18
                        Layout.bottomMargin: 6
                        color: Theme.zinc300
                        font.pixelSize: Typography.xs
                        font.weight: Typography.medium
                    }

                    FieldLabel { visible: view.signUp; text: qsTr("Nickname") }
                    DarkTextField {
                        id: nicknameField
                        visible: view.signUp
                        Layout.fillWidth: true
                        leadingIcon: "user"
                        placeholderText: qsTr("3+ English letters or digits")
                        onAccepted: emailField.forceActiveFocus()
                    }

                    FieldLabel { text: qsTr("Email") }
                    DarkTextField {
                        id: emailField
                        Layout.fillWidth: true
                        leadingIcon: "mail"
                        placeholderText: "you@example.com"
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                        onAccepted: passwordField.forceActiveFocus()
                    }

                    FieldLabel { text: qsTr("Password") }
                    DarkTextField {
                        id: passwordField
                        Layout.fillWidth: true
                        leadingIcon: "lock"
                        password: true
                        placeholderText: view.signUp ? qsTr("8–72 characters, letters and digits") : ""
                        onAccepted: view.signUp ? confirmField.forceActiveFocus() : view.submit()
                    }

                    FieldLabel { visible: view.signUp; text: qsTr("Confirm password") }
                    DarkTextField {
                        id: confirmField
                        visible: view.signUp
                        Layout.fillWidth: true
                        leadingIcon: "lock"
                        password: true
                        onAccepted: view.submit()
                    }

                    TactileButton {
                        Layout.topMargin: 24
                        Layout.fillWidth: true
                        text: view.signUp ? qsTr("Create account") : qsTr("Sign in")
                        implicitHeight: 44
                        busy: authVM.isBusy
                        enabled: !authVM.isBusy
                        onClicked: view.submit()
                    }
                }
            }

            // ----------------------------------------------- Demo access
            AppText {
                Layout.topMargin: 24
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No account? Try the app with demo data:")
                color: Theme.textMuted
                font.pixelSize: Typography.xs
            }
            RowLayout {
                Layout.topMargin: 10
                Layout.fillWidth: true
                spacing: 10

                TactileButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    variant: "secondary"
                    text: qsTr("Demo candidate")
                    iconLeft: "user-check"
                    fontSize: Typography.xs
                    implicitHeight: 38
                    onClicked: view.demoLogin("candidate")
                }
                TactileButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    variant: "secondary"
                    text: qsTr("Demo admin")
                    iconLeft: "shield-check"
                    fontSize: Typography.xs
                    implicitHeight: 38
                    onClicked: view.demoLogin("admin")
                }
            }
        }
    }
}
