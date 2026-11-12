pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Settings dialog (web UserSettingsModal.tsx) with two tabs: the profile
// form plus linked sign-in services, and the platform preferences.
ModalOverlay {
    id: dialog

    property string activeTab: "profile"
    property string feedbackText
    property string feedbackTone: "success"
    property bool savedVisible: false

    function openTab(tab) {
        activeTab = tab === "site" ? "site" : "profile"
        nicknameField.text = settingsVM.displayName
        emailField.text = settingsVM.email
        avatarField.text = settingsVM.avatarUrl
        passwordField.text = ""
        feedbackText = ""
        savedVisible = false
        bodyScroll.contentY = 0
        open = true
    }
    function showSaved() {
        savedVisible = true
        savedTimer.restart()
    }
    function showFeedback(text, tone) {
        feedbackText = text
        feedbackTone = tone
        feedbackTimer.restart()
    }
    function saveProfile() {
        const nickname = nicknameField.text.trim()
        const changed = nickname !== settingsVM.displayName
        if (changed && nickname !== "" && !validators.isValidNickname(nickname)) {
            showFeedback(qsTr("Nickname must be at least 3 characters and contain only English letters and numbers."), "error")
            return
        }
        if (!validators.isValidEmail(emailField.text.trim())) {
            showFeedback(qsTr("Please enter a valid email address."), "error")
            return
        }
        settingsVM.saveProfile(nickname, emailField.text.trim(), avatarField.text.trim())
        passwordField.text = ""
        showSaved()
    }
    function toggleProvider(provider, label) {
        const linked = settingsVM.toggleProvider(provider)
        showFeedback(label + ": " + (linked
            ? qsTr("Service linked to your profile successfully!")
            : qsTr("Service unlinked from your profile.")), "success")
    }

    dialogWidth: 512
    dialogPadding: 28
    onCloseRequested: open = false

    Timer { id: savedTimer; interval: 2500; onTriggered: dialog.savedVisible = false }
    Timer { id: feedbackTimer; interval: 3500; onTriggered: dialog.feedbackText = "" }

    component FieldLabel: AppText {
        Layout.fillWidth: true
        Layout.bottomMargin: 6
        color: Theme.zinc300
        font.pixelSize: Typography.xs
        font.weight: Typography.medium
    }
    component FieldHint: AppText {
        Layout.fillWidth: true
        Layout.topMargin: 4
        color: Theme.textMuted
        font.pixelSize: Typography.tiny
        wrapMode: Text.WordWrap
    }
    component SettingRow: Rectangle {
        default property alias rowContent: rowLayout.data
        Layout.fillWidth: true
        implicitHeight: rowLayout.implicitHeight + 28
        radius: Theme.radiusMd
        color: Theme.surfaceCard
        border.width: 1
        border.color: Theme.borderGlass
        RowLayout {
            id: rowLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 14
            spacing: 10
        }
    }
    component SettingGroup: Rectangle {
        default property alias groupContent: groupLayout.data
        Layout.fillWidth: true
        implicitHeight: groupLayout.implicitHeight + 28
        radius: Theme.radiusMd
        color: Theme.surfaceCard
        border.width: 1
        border.color: Theme.borderGlass
        ColumnLayout {
            id: groupLayout
            x: 14
            y: 14
            width: parent.width - 28
            spacing: 8
        }
    }
    // One of two mutually exclusive choices inside a SettingGroup.
    component ChoiceButton: AbstractButton {
        id: choice
        property bool selected: false

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 100
        implicitHeight: Math.max(36, choiceLabel.implicitHeight + 20)
        hoverEnabled: true
        HoverHandler { cursorShape: Qt.PointingHandCursor }

        background: Rectangle {
            radius: Theme.radiusSm
            color: choice.selected ? Qt.tint(Theme.zinc950, Theme.alpha(Theme.blue950, 0.3)) : Theme.surfaceInput
            border.width: 1
            border.color: choice.selected ? Theme.alpha(Theme.blue500, 0.6)
                : (choice.hovered || choice.visualFocus ? Theme.zinc700 : Theme.borderGlass)
            Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
        }
        contentItem: RowLayout {
            spacing: 6
            Item { implicitWidth: 4 }
            AppText {
                id: choiceLabel
                Layout.fillWidth: true
                text: choice.text
                color: choice.selected ? Theme.blue200 : Theme.zinc400
                font.pixelSize: Typography.xs
                font.weight: Typography.medium
                wrapMode: Text.WordWrap
            }
            Icon {
                visible: choice.selected
                name: "check"
                size: 14
                color: Theme.blue400
            }
            Item { implicitWidth: 4 }
        }
    }
    component ProviderRow: Rectangle {
        id: providerRow
        property string provider
        property string label
        property string description
        property string iconName
        property bool tintIcon: false
        readonly property bool linked: settingsVM.linkedProviders.indexOf(provider) >= 0
        signal toggleRequested()

        Layout.fillWidth: true
        implicitHeight: 62
        radius: Theme.radiusMd
        color: Theme.surfaceCard
        border.width: 1
        border.color: providerHover.hovered ? Qt.rgba(1, 1, 1, 0.14) : Theme.borderGlass
        HoverHandler { id: providerHover }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Rectangle {
                implicitWidth: 36
                implicitHeight: 36
                radius: Theme.radiusSm
                color: Theme.zinc900
                border.width: 1
                border.color: Theme.borderGlass
                Icon {
                    anchors.centerIn: parent
                    name: providerRow.iconName
                    size: 16
                    colorize: providerRow.tintIcon
                    color: Theme.textOnAccent
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                RowLayout {
                    spacing: 8
                    AppText {
                        text: providerRow.label
                        color: Theme.zinc200
                        font.pixelSize: Typography.xs
                        font.weight: Typography.semibold
                    }
                    Badge {
                        pill: true
                        dot: true
                        tone: providerRow.linked ? "emerald" : "neutral"
                        foreground: providerRow.linked ? Theme.emerald400 : Theme.zinc400
                        text: providerRow.linked ? qsTr("Connected") : qsTr("Not connected")
                        fontSize: Typography.micro
                    }
                }
                AppText {
                    Layout.fillWidth: true
                    text: providerRow.description
                    color: Theme.textMuted
                    font.pixelSize: Typography.tiny
                    elide: Text.ElideRight
                }
            }
            TactileButton {
                variant: providerRow.linked ? "danger" : "soft"
                text: providerRow.linked ? qsTr("Unlink") : qsTr("Link")
                iconLeft: providerRow.linked ? "unlink" : "link-2"
                iconSize: 12
                fontSize: Typography.xs
                fontWeight: Typography.medium
                buttonRadius: Theme.radiusSm
                implicitHeight: 30
                leftPadding: 12
                rightPadding: 12
                spacing: 6
                onClicked: providerRow.toggleRequested()
            }
        }
    }

    // -------------------------------------------------------------- Header
    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Rectangle {
            implicitWidth: 32
            implicitHeight: 32
            radius: Theme.radiusSm
            color: Theme.alpha(Theme.blue600, 0.1)
            border.width: 1
            border.color: Theme.alpha(Theme.blue500, 0.2)
            Icon { anchors.centerIn: parent; name: "sparkles"; size: 16; color: Theme.blue400 }
        }
        AppText {
            Layout.fillWidth: true
            text: qsTr("Settings")
            font.pixelSize: Typography.base
            font.weight: Typography.semibold
        }
        AbstractButton {
            id: closeButton
            implicitWidth: 32
            implicitHeight: 32
            hoverEnabled: true
            onClicked: dialog.open = false
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: qsTr("Close")
            background: Rectangle {
                radius: Theme.radiusSm
                color: closeButton.hovered ? Qt.rgba(1, 1, 1, 0.06) : "transparent"
            }
            contentItem: Item {
                Icon {
                    anchors.centerIn: parent
                    name: "x"
                    size: 20
                    color: closeButton.hovered ? Theme.zinc200 : Theme.zinc400
                }
            }
        }
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 16
        implicitHeight: 1
        color: Theme.borderGlass
    }

    SegmentedControl {
        Layout.fillWidth: true
        Layout.topMargin: 16
        model: [
            { value: "profile", label: qsTr("Profile"), icon: "user" },
            { value: "site", label: qsTr("Platform Settings"), icon: "palette" }
        ]
        currentValue: dialog.activeTab
        onActivated: (value) => {
            dialog.activeTab = value
            bodyScroll.contentY = 0
        }
    }

    Rectangle {
        visible: dialog.savedVisible
        Layout.fillWidth: true
        Layout.topMargin: 16
        implicitHeight: 40
        radius: Theme.radiusMd
        color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.emerald950, 0.4))
        border.width: 1
        border.color: Theme.alpha(Theme.emerald500, 0.3)
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8
            Icon { name: "check"; size: 16; color: Theme.emerald400 }
            AppText {
                Layout.fillWidth: true
                text: qsTr("Profile updated successfully!")
                color: Theme.emerald300
                font.pixelSize: Typography.xs
            }
        }
    }

    // ---------------------------------------------------- Scrollable body
    Flickable {
        id: bodyScroll
        Layout.fillWidth: true
        Layout.topMargin: 16
        // Header, tabs and paddings take roughly 170px of the dialog height.
        Layout.preferredHeight: Math.min(contentHeight, Math.max(160, dialog.maxBodyHeight - (dialog.savedVisible ? 226 : 170)))
        contentWidth: width
        contentHeight: dialog.activeTab === "profile" ? profileTab.implicitHeight : siteTab.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ThemedScrollBar {}

        // ------------------------------------------------- Profile tab
        ColumnLayout {
            id: profileTab
            visible: dialog.activeTab === "profile"
            width: bodyScroll.width - 10
            spacing: 0

            SettingRow {
                Icon { name: "shield"; size: 16; color: Theme.blue400 }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Active Role:")
                    color: Theme.zinc300
                    font.pixelSize: Typography.xs
                }
                Badge {
                    tone: settingsVM.role === "admin" ? "blue" : "neutral"
                    foreground: settingsVM.role === "admin" ? Theme.blue300 : Theme.zinc300
                    mono: true
                    caps: true
                    text: settingsVM.role === "admin" ? "admin" : "candidate"
                    fontSize: Typography.tiny
                    fontWeight: Typography.semibold
                    fillOpacity: 0.2
                    strokeOpacity: 0.3
                }
            }

            FieldLabel { Layout.topMargin: 16; text: qsTr("Nickname") }
            DarkTextField {
                id: nicknameField
                Layout.fillWidth: true
                leadingIcon: "user"
                placeholderText: qsTr("Nickname")
            }
            FieldHint { text: qsTr("Min 3 characters, English letters only") }

            FieldLabel { Layout.topMargin: 16; text: qsTr("Email Address") }
            DarkTextField {
                id: emailField
                Layout.fillWidth: true
                leadingIcon: "mail"
                placeholderText: "you@example.com"
            }

            FieldLabel { Layout.topMargin: 16; text: qsTr("Avatar URL (or leave blank)") }
            DarkTextField {
                id: avatarField
                Layout.fillWidth: true
                placeholderText: "https://images.unsplash.com/..."
            }
            FieldHint { text: qsTr("If left empty, the signature rounded square head avatar will be used.") }

            FieldLabel { Layout.topMargin: 16; text: qsTr("New Password") }
            DarkTextField {
                id: passwordField
                Layout.fillWidth: true
                leadingIcon: "lock"
                password: true
                placeholderText: "••••••••"
            }

            TactileButton {
                Layout.topMargin: 20
                Layout.fillWidth: true
                text: qsTr("Save Changes")
                iconLeft: "check"
                implicitHeight: 42
                glow: true
                onClicked: dialog.saveProfile()
            }

            // Linked sign-in services
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 24
                implicitHeight: 1
                color: Theme.borderGlass
            }
            RowLayout {
                Layout.topMargin: 16
                spacing: 8
                Icon { name: "link-2"; size: 16; color: Theme.blue400 }
                AppText {
                    text: qsTr("Connected Services (Account Linking)")
                    font.pixelSize: Typography.xs
                    font.weight: Typography.semibold
                }
            }
            AlertBox {
                Layout.topMargin: 14
                Layout.fillWidth: true
                tone: "info"
                title: qsTr("What is this used for?")
                text: qsTr("Link multiple social accounts to log in to this same profile via GitHub, Google, or LinkedIn, even if they use different email addresses. All session histories and scorecard scores remain synced.")
            }
            AlertBox {
                visible: dialog.feedbackText !== ""
                Layout.topMargin: 14
                Layout.fillWidth: true
                tone: dialog.feedbackTone
                text: dialog.feedbackText
            }
            ProviderRow {
                Layout.topMargin: 14
                provider: "github"
                onToggleRequested: dialog.toggleProvider(provider, label)
                label: "GitHub"
                iconName: "brand-github"
                tintIcon: true
                description: qsTr("For developers and code repositories")
            }
            ProviderRow {
                Layout.topMargin: 8
                provider: "google"
                onToggleRequested: dialog.toggleProvider(provider, label)
                label: "Google"
                iconName: "brand-google"
                description: qsTr("Fast sign-in via Google Workspace")
            }
            ProviderRow {
                Layout.topMargin: 8
                Layout.bottomMargin: 2
                provider: "linkedin"
                onToggleRequested: dialog.toggleProvider(provider, label)
                label: "LinkedIn"
                iconName: "brand-linkedin"
                description: qsTr("Professional network and career profile")
            }
        }

        // ----------------------------------------------- Platform tab
        ColumnLayout {
            id: siteTab
            visible: dialog.activeTab === "site"
            width: bodyScroll.width - 10
            spacing: 16

            SettingGroup {
                AppText {
                    text: qsTr("Color Theme")
                    color: Theme.zinc300
                    font.pixelSize: Typography.xs
                    font.weight: Typography.medium
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ChoiceButton {
                        text: qsTr("OLED Dark (Default)")
                        selected: settingsVM.theme === "oled"
                        onClicked: { settingsVM.theme = "oled"; dialog.showSaved() }
                    }
                    ChoiceButton {
                        text: qsTr("Slate Night")
                        selected: settingsVM.theme === "slate"
                        onClicked: { settingsVM.theme = "slate"; dialog.showSaved() }
                    }
                }
            }

            SettingRow {
                Icon { name: "volume-2"; size: 16; color: Theme.blue400 }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Sound effects during interview")
                    color: Theme.zinc300
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }
                ToggleSwitch {
                    checked: settingsVM.soundEnabled
                    onToggled: { settingsVM.soundEnabled = checked; dialog.showSaved() }
                }
            }

            SettingGroup {
                RowLayout {
                    spacing: 8
                    Icon { name: "cpu"; size: 16; color: Theme.blue400 }
                    AppText {
                        text: qsTr("AI Interviewer Persona")
                        color: Theme.zinc300
                        font.pixelSize: Typography.xs
                        font.weight: Typography.medium
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ChoiceButton {
                        text: qsTr("Balanced (Tech Lead)")
                        selected: settingsVM.aiPersona === "balanced"
                        onClicked: { settingsVM.aiPersona = "balanced"; dialog.showSaved() }
                    }
                    ChoiceButton {
                        text: qsTr("Rigorous (FAANG Principal)")
                        selected: settingsVM.aiPersona === "strict"
                        onClicked: { settingsVM.aiPersona = "strict"; dialog.showSaved() }
                    }
                }
            }

            SettingRow {
                Icon { name: "globe"; size: 16; color: Theme.blue400 }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Interface Language")
                    color: Theme.zinc300
                    font.pixelSize: Typography.xs
                }
                LanguageToggle { appearance: "inline" }
            }
        }
    }
}
