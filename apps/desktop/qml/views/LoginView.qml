import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Sign in / sign up page (web app/login/page.tsx). Shown over the content
// area while authVM.isLoginModalOpen is set; the navbar stays available.
Item {
    id: view

    // Emitted for messages that should outlive the page (shown as a toast).
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
            localError = qsTr("Please fill in all required fields.")
            return
        }
        if (!validators.isValidEmail(email)) {
            localError = qsTr("Please enter a valid email address.")
            return
        }
        if (signUp) {
            const nickname = nicknameField.text.trim()
            if (nickname === "") {
                localError = qsTr("Please provide your nickname to sign up.")
                return
            }
            if (!validators.isValidNickname(nickname)) {
                localError = qsTr("Nickname must be at least 3 characters long and contain only English letters (and numbers).")
                return
            }
            if (!validators.isValidPassword(password)) {
                localError = qsTr("Password must be between 8 and 72 characters and include both letters and numbers.")
                return
            }
            if (password !== confirmField.text) {
                localError = qsTr("Passwords do not match. Please verify your password confirmation.")
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
            view.notify(qsTr("Signed in as Candidate: the administrator demo is disabled in this build."), "info")
    }
    function providerUnavailable(name) {
        successMessage = ""
        localError = qsTr("Sign-in with %1 is not available in the desktop app yet. Use email and password or the Demo mode below.").arg(name)
    }

    onVisibleChanged: {
        if (visible) {
            resetMessages()
            passwordField.text = ""
            confirmField.text = ""
            scroller.contentY = 0
            cardEntrance.restart()
            emailField.forceActiveFocus()
        }
    }

    // A successful sign-in (real or demo) continues to the dashboard.
    Connections {
        target: authVM
        function onAuthStateChanged() {
            if (authVM.isAuthenticated && authVM.isLoginModalOpen)
                navVM.navigateToDashboard()
        }
    }

    Keys.onEscapePressed: authVM.isLoginModalOpen = false

    // --------------------------------------------------------- Backdrop
    Rectangle { anchors.fill: parent; color: Theme.zinc950 }
    DotGrid { anchors.fill: parent; cell: 24; opacity: 0.035 }
    GlowBlob {
        width: 1000
        height: 620
        x: (parent.width - width) / 2
        y: parent.height * 0.25 - height / 2
        color: Theme.blue600
        strength: 0.12
    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
    }

    component FieldLabel: RowLayout {
        property string text
        property bool required: false
        Layout.fillWidth: true
        Layout.bottomMargin: 6
        spacing: 4
        AppText {
            text: parent.text
            color: Theme.zinc300
            font.pixelSize: Typography.xs
            font.weight: Typography.medium
        }
        AppText {
            visible: parent.required
            text: "*"
            color: Theme.red500
            font.pixelSize: Typography.xs
            font.weight: Typography.bold
        }
        Item { Layout.fillWidth: true }
    }
    component FieldHint: AppText {
        Layout.fillWidth: true
        Layout.topMargin: 4
        color: Theme.textMuted
        font.pixelSize: Typography.tiny
        wrapMode: Text.WordWrap
    }
    component ProviderButton: AbstractButton {
        id: provider
        property string iconName
        property bool tint: false

        Layout.fillWidth: true
        implicitHeight: 42
        hoverEnabled: true
        HoverHandler { cursorShape: Qt.PointingHandCursor }
        scale: down ? 0.97 : 1.0
        Behavior on scale {
            NumberAnimation { duration: Theme.durationFast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
        }

        background: Rectangle {
            radius: Theme.radiusMd
            color: provider.hovered ? Theme.zinc800 : Theme.alpha(Theme.zinc900, 0.9)
            border.width: 1
            border.color: provider.visualFocus ? Theme.focusRing : (provider.hovered ? Theme.borderHover : Theme.borderStrong)
            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
        }
        contentItem: Item {
            RowLayout {
                anchors.centerIn: parent
                spacing: 10
                Icon {
                    name: provider.iconName
                    size: 16
                    colorize: provider.tint
                    color: Theme.textOnAccent
                }
                AppText {
                    text: provider.text
                    font.weight: Typography.medium
                }
            }
        }
    }

    component DemoButton: AbstractButton {
        id: demo
        property string iconName
        property bool accent: false
        property color iconColor: Theme.blue400

        Layout.fillWidth: true
        Layout.preferredWidth: 100
        implicitHeight: 34
        hoverEnabled: true
        HoverHandler { cursorShape: Qt.PointingHandCursor }
        scale: down ? 0.97 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.durationFast } }

        background: Rectangle {
            radius: Theme.radiusSm
            color: demo.accent
                ? Qt.tint(Theme.zinc950, Theme.alpha(demo.hovered ? Theme.blue900 : Theme.blue950, demo.hovered ? 0.4 : 0.3))
                : (demo.hovered ? Theme.zinc800 : Theme.alpha(Theme.zinc900, 0.7))
            border.width: 1
            border.color: demo.visualFocus ? Theme.focusRing
                : (demo.accent ? Theme.alpha(Theme.blue500, 0.3) : Theme.borderGlass)
            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
        }
        contentItem: Item {
            RowLayout {
                anchors.centerIn: parent
                spacing: 6
                Icon { name: demo.iconName; size: 14; color: demo.iconColor }
                AppText {
                    text: demo.text
                    color: demo.accent
                        ? (demo.hovered ? Theme.blue200 : Theme.blue300)
                        : (demo.hovered ? Theme.textOnAccent : Theme.zinc300)
                    font.pixelSize: Typography.xs
                    font.weight: Typography.medium
                }
            }
        }
    }

    Flickable {
        id: scroller
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, card.height + 96)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ThemedScrollBar {}

        SoftGlow {
            anchors.fill: card
            color: "#000000"
            radius: Theme.radiusLg
            blur: 40
            offsetY: 20
            strength: 0.6
        }

        Rectangle {
            id: card
            width: Math.min(448, scroller.width - 32)
            height: form.implicitHeight + 72
            x: (scroller.width - width) / 2
            y: Math.max(48, (scroller.contentHeight - height) / 2) + entranceOffset
            property real entranceOffset: 0
            radius: Theme.radiusLg
            color: Theme.alpha(Theme.zinc950, 0.85)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.1)

            ParallelAnimation {
                id: cardEntrance
                NumberAnimation { target: card; property: "opacity"; from: 0; to: 1; duration: Theme.durationEnter }
                NumberAnimation {
                    target: card; property: "entranceOffset"; from: 8; to: 0
                    duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
                }
            }

            ColumnLayout {
                id: form
                x: 36
                y: 36
                width: parent.width - 72
                spacing: 0

                // ------------------------------------------------ Header
                BrandMark {
                    Layout.alignment: Qt.AlignHCenter
                    tileSize: 32
                    glyphSize: 16
                    tileRadius: Theme.radiusMd
                    textSize: Typography.lg
                    textWeight: Typography.bold
                    onClicked: {
                        authVM.isLoginModalOpen = false
                        navVM.navigateToLanding()
                    }
                }
                AppText {
                    Layout.topMargin: 16
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: view.signUp ? qsTr("Create an Account") : qsTr("Sign In to Dev Dialogue")
                    font.pixelSize: Typography.xl2
                    font.weight: Typography.bold
                    font.letterSpacing: Typography.tracking(Typography.xl2, Typography.trackingTight)
                    wrapMode: Text.WordWrap
                }
                AppText {
                    Layout.topMargin: 6
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: view.signUp
                        ? qsTr("Join the platform to practice realistic technical interviews with AI.")
                        : qsTr("Sign in to sync your interview sessions, scores, and learning progress.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }

                SegmentedControl {
                    Layout.topMargin: 32
                    Layout.fillWidth: true
                    model: [
                        { value: "signin", label: qsTr("Sign In") },
                        { value: "signup", label: qsTr("Sign Up") }
                    ]
                    currentValue: view.mode
                    onActivated: (value) => view.switchMode(value)
                }

                AlertBox {
                    visible: view.errorText !== ""
                    Layout.topMargin: 24
                    Layout.fillWidth: true
                    tone: "error"
                    text: view.errorText
                }
                AlertBox {
                    visible: view.successMessage !== ""
                    Layout.topMargin: 24
                    Layout.fillWidth: true
                    tone: "success"
                    text: view.successMessage
                }

                // -------------------------------------- Social providers
                ProviderButton {
                    Layout.topMargin: 24
                    text: qsTr("Continue with GitHub")
                    iconName: "brand-github"
                    tint: true
                    onClicked: view.providerUnavailable("GitHub")
                }
                RowLayout {
                    Layout.topMargin: 10
                    Layout.fillWidth: true
                    spacing: 10
                    ProviderButton {
                        Layout.preferredWidth: 100
                        text: "Google"
                        iconName: "brand-google"
                        onClicked: view.providerUnavailable("Google")
                    }
                    ProviderButton {
                        Layout.preferredWidth: 100
                        text: "LinkedIn"
                        iconName: "brand-linkedin"
                        onClicked: view.providerUnavailable("LinkedIn")
                    }
                }

                // ------------------------------------------------ Divider
                Item {
                    Layout.topMargin: 24
                    Layout.bottomMargin: 24
                    Layout.fillWidth: true
                    implicitHeight: dividerLabel.implicitHeight

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 1
                        color: Theme.borderGlass
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: dividerLabel.implicitWidth + 24
                        height: dividerLabel.implicitHeight
                        color: Theme.zinc950
                        MonoText {
                            id: dividerLabel
                            anchors.centerIn: parent
                            text: qsTr("or continue with email")
                            caps: true
                            color: Theme.textMuted
                            font.pixelSize: Typography.tiny
                        }
                    }
                }

                // --------------------------------------------- Email form
                FieldLabel {
                    visible: view.signUp
                    text: qsTr("Nickname")
                    required: true
                }
                DarkTextField {
                    id: nicknameField
                    visible: view.signUp
                    Layout.fillWidth: true
                    leadingIcon: "user"
                    placeholderText: qsTr("Nickname")
                    onAccepted: emailField.forceActiveFocus()
                }
                FieldHint {
                    visible: view.signUp
                    Layout.bottomMargin: 16
                    text: qsTr("Min 3 characters, English letters only")
                }

                FieldLabel {
                    text: qsTr("Email Address")
                    required: view.signUp
                }
                DarkTextField {
                    id: emailField
                    Layout.fillWidth: true
                    leadingIcon: "mail"
                    placeholderText: "you@example.com"
                    inputMethodHints: Qt.ImhEmailCharactersOnly
                    onAccepted: passwordField.forceActiveFocus()
                }

                FieldLabel {
                    Layout.topMargin: 16
                    text: qsTr("Password")
                    required: view.signUp
                }
                DarkTextField {
                    id: passwordField
                    Layout.fillWidth: true
                    leadingIcon: "lock"
                    password: true
                    placeholderText: "••••••••"
                    onAccepted: view.signUp ? confirmField.forceActiveFocus() : view.submit()
                }
                FieldHint {
                    visible: view.signUp
                    text: qsTr("8 to 72 characters, letters & numbers")
                }

                FieldLabel {
                    visible: view.signUp
                    Layout.topMargin: 16
                    text: qsTr("Confirm Password")
                    required: true
                }
                DarkTextField {
                    id: confirmField
                    visible: view.signUp
                    Layout.fillWidth: true
                    leadingIcon: "lock"
                    password: true
                    placeholderText: "••••••••"
                    onAccepted: view.submit()
                }

                TactileButton {
                    Layout.topMargin: 24
                    Layout.fillWidth: true
                    text: view.signUp ? qsTr("Create Account") : qsTr("Sign In")
                    iconRight: "arrow-right"
                    implicitHeight: 46
                    busy: authVM.isBusy
                    enabled: !authVM.isBusy
                    glow: true
                    onClicked: view.submit()
                }

                RowLayout {
                    Layout.topMargin: 16
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 4

                    AppText {
                        text: view.signUp ? qsTr("Already have an account?") : qsTr("No account?")
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                    }
                    AppText {
                        text: view.signUp ? qsTr("Sign In!") : qsTr("Sign Up!")
                        color: switchHover.hovered ? Theme.blue300 : Theme.blue400
                        font.pixelSize: Typography.xs
                        font.weight: Typography.semibold
                        font.underline: true
                        HoverHandler { id: switchHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: view.switchMode(view.signUp ? "signin" : "signup") }
                    }
                }

                // --------------------------------------------- Demo access
                Rectangle {
                    Layout.topMargin: 32
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.borderGlass
                }
                RowLayout {
                    Layout.topMargin: 24
                    Layout.fillWidth: true
                    spacing: 6

                    Icon { name: "sparkles"; size: 12; color: Theme.blue400 }
                    MonoText {
                        Layout.fillWidth: true
                        text: qsTr("Quick Test (Demo Mode)")
                        caps: true
                        font.pixelSize: Typography.tiny
                    }
                    MonoText {
                        text: qsTr("1 click")
                        font.pixelSize: Typography.micro
                    }
                }
                RowLayout {
                    Layout.topMargin: 12
                    Layout.fillWidth: true
                    spacing: 8

                    DemoButton {
                        text: qsTr("As Admin")
                        iconName: "shield-check"
                        accent: true
                        onClicked: view.demoLogin("admin")
                    }
                    DemoButton {
                        text: qsTr("As Candidate")
                        iconName: "user-check"
                        iconColor: Theme.emerald400
                        onClicked: view.demoLogin("candidate")
                    }
                }
            }
        }
    }
}
