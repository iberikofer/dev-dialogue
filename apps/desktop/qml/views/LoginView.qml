pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Sign-in screen shown while nobody is signed in: the product on the left,
// the email form and the demo access on the right.
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

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.windowGap
        spacing: Theme.windowGap

        // ------------------------------------------------------ Product
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radiusXl
            border.width: 1
            border.color: Theme.borderGlass
            clip: true
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#121a2b" }
                GradientStop { position: 1.0; color: "#0b0d13" }
            }

            GlowBlob {
                width: parent.width * 1.1
                height: parent.height * 0.8
                x: -parent.width * 0.2
                y: -parent.height * 0.2
                color: Theme.blue600
                strength: 0.22
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 56
                spacing: 0

                Image {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    source: Qt.resolvedUrl("../../resources/app/app-icon.svg")
                    sourceSize.width: 144
                    sourceSize.height: 144
                    smooth: true
                }
                AppText {
                    Layout.topMargin: 28
                    Layout.fillWidth: true
                    text: qsTr("Practise technical interviews with an AI interviewer")
                    font.pixelSize: Typography.xl4
                    font.weight: Typography.extrabold
                    font.letterSpacing: Typography.tracking(Typography.xl4, Typography.trackingTighter)
                    lineHeight: Typography.leadingTight
                    wrapMode: Text.WordWrap
                }
                AppText {
                    Layout.topMargin: 16
                    Layout.fillWidth: true
                    Layout.maximumWidth: 520
                    text: qsTr("Pick a track and a level, answer follow-up questions, and get a scorecard with what to study next.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.base
                    lineHeight: Typography.leadingRelaxed
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: [
                        { icon: "code-xml", text: qsTr("Frontend, Backend, DevOps, QA and PM tracks") },
                        { icon: "bot", text: qsTr("Adaptive questions in Ukrainian or English") },
                        { icon: "trophy", text: qsTr("Scorecard with per-question feedback") }
                    ]
                    RowLayout {
                        id: point
                        required property var modelData
                        required property int index
                        Layout.topMargin: index === 0 ? 32 : 14
                        spacing: 12
                        Rectangle {
                            implicitWidth: 32
                            implicitHeight: 32
                            radius: Theme.radiusSm
                            color: Theme.alpha(Theme.blue500, 0.12)
                            border.width: 1
                            border.color: Theme.alpha(Theme.blue500, 0.25)
                            Icon { anchors.centerIn: parent; name: point.modelData.icon; size: 16; color: Theme.blue300 }
                        }
                        AppText {
                            text: point.modelData.text
                            color: Theme.zinc300
                        }
                    }
                }
            }
        }

        // --------------------------------------------------------- Form
        Rectangle {
            Layout.preferredWidth: 460
            Layout.fillHeight: true
            radius: Theme.radiusXl
            color: "#0e0e11"
            border.width: 1
            border.color: Theme.borderGlass

            Flickable {
                id: scroller
                anchors.fill: parent
                contentWidth: width
                contentHeight: Math.max(height, form.implicitHeight + 96)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ThemedScrollBar {}

                ColumnLayout {
                    id: form
                    width: scroller.width - 96
                    x: 48
                    y: Math.max(48, (scroller.contentHeight - implicitHeight) / 2)
                    spacing: 0

                    AppText {
                        Layout.fillWidth: true
                        text: view.signUp ? qsTr("Create an account") : qsTr("Sign in")
                        font.pixelSize: Typography.xl2
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
                        Layout.topMargin: 24
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
                        iconRight: "arrow-right"
                        implicitHeight: 46
                        busy: authVM.isBusy
                        enabled: !authVM.isBusy
                        glow: true
                        onClicked: view.submit()
                    }

                    // ------------------------------------------ Demo access
                    RowLayout {
                        Layout.topMargin: 32
                        Layout.fillWidth: true
                        spacing: 12
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.borderGlass }
                        MonoText {
                            text: qsTr("or try without an account")
                            caps: true
                            color: Theme.textMuted
                            font.pixelSize: Typography.micro
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.borderGlass }
                    }
                    RowLayout {
                        Layout.topMargin: 16
                        Layout.fillWidth: true
                        spacing: 10

                        TactileButton {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            variant: "secondary"
                            text: qsTr("Demo candidate")
                            iconLeft: "user-check"
                            fontSize: Typography.xs
                            implicitHeight: 40
                            onClicked: view.demoLogin("candidate")
                        }
                        TactileButton {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            variant: "secondary"
                            text: qsTr("Demo admin")
                            iconLeft: "shield-check"
                            fontSize: Typography.xs
                            implicitHeight: 40
                            onClicked: view.demoLogin("admin")
                        }
                    }
                }
            }
        }
    }
}
