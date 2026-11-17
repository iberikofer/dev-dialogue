pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Account: tabs for Profile & Server Connection vs Voice/Camera Settings & Custom Shortcuts.
AppPage {
    id: page

    property int activeTab: 0 // 0 = Profile, 1 = Settings & Shortcuts
    property string listeningAction: ""
    property bool serversMenuOpen: false

    function startShortcutCapture(actionId) {
        listeningAction = actionId
        settingsVM.recordingShortcut = true
        keyCaptureFocus.forceActiveFocus()
    }

    function stopShortcutCapture() {
        listeningAction = ""
        settingsVM.recordingShortcut = false
    }

    function applyShortcut(actionId, seq) {
        if (actionId === "micTalk") {
            settingsVM.micTalkShortcut = seq
        } else if (actionId === "micMute") {
            settingsVM.micMuteShortcut = seq
        } else if (actionId === "cameraToggle") {
            settingsVM.cameraToggleShortcut = seq
        } else if (actionId === "aiMute") {
            settingsVM.aiMuteShortcut = seq
        }
    }

    active: StackLayout.isCurrentItem
    onActiveChanged: {
        if (active) {
            backendVM.refreshDbMode()
        } else {
            stopShortcutCapture()
        }
    }
    Component.onCompleted: backendVM.refreshDbMode()

    title: qsTr("Account")
    subtitle: qsTr("Your profile, connection, voice mode and keyboard shortcuts.")
    scrollable: true

    // Hidden focus target while user presses a key to rebind a shortcut.
    Item {
        id: keyCaptureFocus
        visible: false
        focus: page.listeningAction.length > 0
        Keys.onPressed: (event) => {
            if (page.listeningAction.length === 0) {
                return
            }
            event.accepted = true
            if (event.key === Qt.Key_Escape) {
                page.stopShortcutCapture()
                return
            }
            const formatted = settingsVM.formatKeyEvent(event.key, event.modifiers)
            if (formatted.length > 0) {
                page.applyShortcut(page.listeningAction, formatted)
                page.stopShortcutCapture()
            }
        }
    }

    component Section: DarkCard {
        id: section
        default property alias rows: sectionColumn.data
        property string heading
        Layout.fillWidth: true
        Layout.maximumWidth: 760
        padding: 24

        ColumnLayout {
            id: sectionColumn
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 14

            MonoText {
                caps: true
                text: section.heading
                font.weight: Typography.semibold
            }
        }
    }

    // ------------------------------------------------------------ Profile Tabs
    Rectangle {
        Layout.fillWidth: true
        Layout.maximumWidth: 760
        implicitHeight: tabRow.implicitHeight + 10
        radius: Theme.radiusMd
        color: Theme.zinc900
        border.width: 1
        border.color: Theme.borderGlass

        RowLayout {
            id: tabRow
            anchors.fill: parent
            anchors.margins: 5
            spacing: 6

            TactileButton {
                Layout.fillWidth: true
                variant: page.activeTab === 0 ? "primary" : "ghost"
                iconLeft: "user"
                iconSize: 14
                text: qsTr("Профіль і сервер")
                fontSize: Typography.xs
                implicitHeight: 34
                onClicked: {
                    page.stopShortcutCapture()
                    page.activeTab = 0
                }
            }

            TactileButton {
                Layout.fillWidth: true
                variant: page.activeTab === 1 ? "primary" : "ghost"
                iconLeft: "sliders-horizontal"
                iconSize: 14
                text: qsTr("Налаштування і гарячі клавіші")
                fontSize: Typography.xs
                implicitHeight: 34
                onClicked: page.activeTab = 1
            }
        }
    }

    // =========================================================================
    // TAB 0: PROFILE & SERVER
    // =========================================================================
    Section {
        visible: page.activeTab === 0
        heading: qsTr("Profile")

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            UserAvatar {
                size: 56
                glyphSize: 30
                avatarUrl: settingsVM.avatarUrl
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                RowLayout {
                    spacing: 10
                    AppText {
                        text: settingsVM.displayName
                        font.pixelSize: Typography.lg
                        font.weight: Typography.semibold
                    }
                    Badge {
                        tone: settingsVM.role === "admin" ? "blue" : "neutral"
                        mono: true
                        caps: true
                        text: settingsVM.role === "admin" ? qsTr("Admin") : qsTr("Candidate")
                        fontSize: Typography.tiny
                    }
                    Badge {
                        visible: settingsVM.isDemoAccount
                        tone: "amber"
                        mono: true
                        text: qsTr("Demo account")
                        fontSize: Typography.tiny
                    }
                }
                AppText {
                    text: settingsVM.email
                    color: Theme.zinc400
                }
            }
        }

        RowLayout {
            visible: authVM.isAdmin
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                AppText { text: qsTr("Admin view"); font.weight: Typography.medium }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Shows the question bank in the sidebar. Turn off to use the app as a candidate.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }
            }
            ToggleSwitch {
                checked: authVM.isAdminViewActive
                onToggled: authVM.toggleAdminView()
            }
        }

        RowLayout {
            visible: settingsVM.isDemoAccount
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                AppText { text: qsTr("Demo role"); font.weight: Typography.medium }
                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Switch between the candidate and the administrator demo.")
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    wrapMode: Text.WordWrap
                }
            }
            TactileButton {
                variant: "secondary"
                text: authVM.isAdmin ? qsTr("Switch to candidate") : qsTr("Switch to admin")
                fontSize: Typography.xs
                implicitHeight: 34
                onClicked: authVM.mockLogin(authVM.isAdmin ? "candidate" : "admin")
            }
        }
    }

    Section {
        visible: page.activeTab === 0
        heading: qsTr("Server")

        // Minimalistic status indicator + toggle button for hidden servers menu
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item {
                Layout.fillWidth: true
                implicitHeight: serverSummaryRow.implicitHeight

                HoverHandler { id: serverSummaryHover }

                RowLayout {
                    id: serverSummaryRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 10

                    PulseDot {
                        size: 9
                        color: backendVM.allServicesReady ? Theme.emerald500 : (backendVM.isOffline ? Theme.amber500 : Theme.blue400)
                        pingColor: backendVM.allServicesReady ? Theme.emerald400 : Theme.amber400
                        pinging: backendVM.isOffline || backendVM.isChecking || backendVM.isManagingServices
                    }

                    AppText {
                        text: backendVM.allServicesReady
                            ? qsTr("Сервери працюють")
                            : (backendVM.isOffline ? qsTr("Offline") : qsTr("Connected"))
                        font.weight: Typography.medium
                    }

                    Item { Layout.fillWidth: true }
                }

                AppToolTip {
                    visible: serverSummaryHover.hovered
                    text: backendVM.isOffline
                        ? qsTr("API :5238 недоступний (демо-режим).")
                        : qsTr("API :5238 · 3D Avatar (:8080/:8085) · %1 · Parakeet STT").arg(backendVM.localModelName)
                }
            }

            TactileButton {
                variant: "secondary"
                iconLeft: "zap"
                iconSize: 13
                implicitWidth: 32
                implicitHeight: 32
                leftPadding: 0
                rightPadding: 0
                onClicked: systemMonitorVM.toggle()
                AppToolTip {
                    visible: parent.hovered
                    text: qsTr("Монітор навантаження (Shift+Alt+X)")
                }
            }

            TactileButton {
                variant: "secondary"
                iconLeft: "server"
                iconSize: 13
                text: page.serversMenuOpen ? qsTr("Сховати") : qsTr("Сервери")
                fontSize: Typography.xs
                implicitHeight: 32
                leftPadding: 10
                rightPadding: 10
                onClicked: {
                    page.serversMenuOpen = !page.serversMenuOpen
                    if (page.serversMenuOpen)
                        backendVM.checkAllServices()
                }
            }
        }

        // Hidden collapsible menu with minimal status & per-service restart
        Rectangle {
            visible: page.serversMenuOpen
            Layout.fillWidth: true
            implicitHeight: serversMenuCol.implicitHeight + 20
            radius: Theme.radiusMd
            color: Theme.zinc900
            border.width: 1
            border.color: Theme.borderFaint

            ColumnLayout {
                id: serversMenuCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                Repeater {
                    model: [
                        {
                            id: "api",
                            name: qsTr("API Backend"),
                            ok: !backendVM.isOffline,
                            tip: qsTr("Основний сервер (:5238 · SQLite / REST API)"),
                            canRestart: true
                        },
                        {
                            id: "avatar",
                            name: qsTr("3D-модель"),
                            ok: backendVM.avatarServerReady,
                            tip: qsTr("WebGL :8080 · TTS/Rhubarb :8085 · WebView2"),
                            canRestart: true
                        },
                        {
                            id: "llm",
                            name: backendVM.localModelName,
                            ok: backendVM.localModelReady && backendVM.ollamaRunning,
                            tip: qsTr("Ollama (:11434) · локальний AI та аналіз камери"),
                            canRestart: true
                        },
                        {
                            id: "stt",
                            name: qsTr("Parakeet STT"),
                            ok: backendVM.sttModelReady,
                            tip: qsTr("Локальне розпізнавання голосу (SherpaOnnx)"),
                            canRestart: false
                        }
                    ]

                    Rectangle {
                        id: srvRow
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 34
                        radius: Theme.radiusSm
                        color: srvRowHover.hovered ? Theme.alpha(Theme.zinc800, 0.55) : "transparent"

                        HoverHandler { id: srvRowHover }

                        AppToolTip {
                            visible: srvRowHover.hovered
                            text: srvRow.modelData.tip
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 6
                            spacing: 8

                            Rectangle {
                                implicitWidth: 7
                                implicitHeight: 7
                                radius: 4
                                color: srvRow.modelData.ok ? Theme.emerald500 : Theme.amber500
                            }

                            AppText {
                                Layout.fillWidth: true
                                text: srvRow.modelData.name
                                font.pixelSize: Typography.xs
                                color: Theme.zinc200
                                elide: Text.ElideRight
                            }

                            TactileButton {
                                visible: srvRow.modelData.canRestart
                                variant: "ghost"
                                iconLeft: "refresh-cw"
                                iconSize: 12
                                implicitWidth: 26
                                implicitHeight: 26
                                leftPadding: 0
                                rightPadding: 0
                                enabled: !backendVM.isManagingServices
                                onClicked: {
                                    if (srvRow.modelData.id === "api") {
                                        backendVM.restartApiBackend()
                                    } else if (srvRow.modelData.id === "avatar") {
                                        backendVM.restartAvatarServers()
                                        avatarHost.reload()
                                    } else if (srvRow.modelData.id === "llm") {
                                        if (backendVM.localModelReady)
                                            backendVM.restartLocalModels()
                                        else
                                            backendVM.downloadLocalModel()
                                    }
                                }
                                AppToolTip {
                                    visible: parent.hovered
                                    text: qsTr("Перезапустити")
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 2
                    spacing: 8

                    TactileButton {
                        Layout.fillWidth: true
                        variant: "secondary"
                        iconLeft: "shield-check"
                        iconSize: 12
                        text: backendVM.isManagingServices ? qsTr("Перевірка...") : qsTr("Перевірити")
                        fontSize: Typography.micro
                        implicitHeight: 28
                        enabled: !backendVM.isManagingServices
                        onClicked: backendVM.checkAllServices()
                    }

                    TactileButton {
                        Layout.fillWidth: true
                        variant: "secondary"
                        iconLeft: "refresh-cw"
                        iconSize: 12
                        text: qsTr("Перезапустити все")
                        fontSize: Typography.micro
                        implicitHeight: 28
                        enabled: !backendVM.isManagingServices
                        onClicked: backendVM.restartAllServices()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 12

            Item {
                Layout.fillWidth: true
                implicitHeight: dbLabelRow.implicitHeight
                HoverHandler { id: dbRowHover }

                RowLayout {
                    id: dbLabelRow
                    spacing: 8
                    AppText {
                        text: qsTr("Encrypted SQLite database")
                        font.weight: Typography.medium
                    }
                    Badge {
                        visible: backendVM.useSqlite
                        tone: "emerald"
                        mono: true
                        text: "SQLCipher AES-256"
                        fontSize: Typography.tiny
                    }
                }

                AppToolTip {
                    visible: dbRowHover.hovered
                    text: backendVM.isOffline
                        ? qsTr("Available when connected to the server.")
                        : (backendVM.databaseProvider.length > 0
                            ? backendVM.databaseProvider
                            : qsTr("Switch between default storage and encrypted SQLite."))
                }
            }
            ToggleSwitch {
                enabled: !backendVM.isOffline && !backendVM.isSwitchingDb
                checked: backendVM.useSqlite
                onToggled: backendVM.toggleDatabaseMode()
            }
        }

        // Minimalistic app version & auto-update row
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 12

            Item {
                Layout.fillWidth: true
                implicitHeight: verRow.implicitHeight
                HoverHandler { id: verHover }

                RowLayout {
                    id: verRow
                    spacing: 8
                    AppText {
                        text: qsTr("Оновлення")
                        font.weight: Typography.medium
                    }
                    MonoText {
                        text: Qt.application.version + " (" + backendVM.currentCommit + ")"
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                    }
                    Badge {
                        visible: backendVM.updateAvailable
                        tone: "blue"
                        mono: true
                        text: backendVM.latestVersion
                        fontSize: Typography.tiny
                    }
                }

                AppToolTip {
                    visible: verHover.hovered
                    text: backendVM.updateStatusText !== ""
                        ? backendVM.updateStatusText
                        : qsTr("Перевірка нових збірок з гілки main")
                }
            }

            TactileButton {
                variant: backendVM.updateAvailable ? "primary" : "secondary"
                iconLeft: backendVM.updateAvailable ? "arrow-up-right" : "refresh-cw"
                iconSize: 12
                text: backendVM.isUpdating
                    ? (backendVM.updateProgress + "%")
                    : (backendVM.updateAvailable ? qsTr("Оновити") : qsTr("Перевірити"))
                fontSize: Typography.xs
                implicitHeight: 30
                leftPadding: 10
                rightPadding: 10
                busy: backendVM.isUpdating
                enabled: !backendVM.isUpdating
                onClicked: {
                    if (backendVM.updateAvailable)
                        backendVM.installUpdate()
                    else
                        backendVM.checkForUpdates(false)
                }
            }
        }
    }

    // =========================================================================
    // TAB 1: SETTINGS (MIC MODE + CUSTOM SHORTCUTS — MINIMAL, DETAILS ON HOVER)
    // =========================================================================
    Section {
        visible: page.activeTab === 1
        heading: qsTr("Мікрофон і камера")

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Hold-to-Talk Mode Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: holdRow.implicitHeight + 20
                radius: Theme.radiusMd
                color: settingsVM.micMode === "hold" ? Theme.alpha(Theme.blue500, 0.14) : Theme.zinc900
                border.width: 1
                border.color: settingsVM.micMode === "hold" ? Theme.alpha(Theme.blue500, 0.65) : Theme.borderGlass

                MouseArea {
                    id: holdArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: settingsVM.micMode = "hold"
                }

                AppToolTip {
                    visible: holdArea.containsMouse
                    text: qsTr("Натисніть і тримайте клавішу або кнопку, поки говорите. Відпустіть — запис одразу розпізнається й надішлеться.")
                }

                RowLayout {
                    id: holdRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    spacing: 8
                    Icon { name: "mic"; size: 15; color: settingsVM.micMode === "hold" ? Theme.blue400 : Theme.zinc400 }
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("Утримання (Push-to-Talk)")
                        font.weight: Typography.semibold
                        font.pixelSize: Typography.sm
                    }
                    Badge {
                        visible: settingsVM.micMode === "hold"
                        tone: "blue"
                        text: qsTr("Активно")
                        fontSize: Typography.tiny
                    }
                }
            }

            // Toggle Mode Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: toggleRow.implicitHeight + 20
                radius: Theme.radiusMd
                color: settingsVM.micMode === "toggle" ? Theme.alpha(Theme.emerald500, 0.14) : Theme.zinc900
                border.width: 1
                border.color: settingsVM.micMode === "toggle" ? Theme.alpha(Theme.emerald500, 0.65) : Theme.borderGlass

                MouseArea {
                    id: toggleArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: settingsVM.micMode = "toggle"
                }

                AppToolTip {
                    visible: toggleArea.containsMouse
                    text: qsTr("Перше натискання вмикає мікрофон, повторне натискання вимикає його та надсилає відповідь AI-моделі.")
                }

                RowLayout {
                    id: toggleRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    spacing: 8
                    Icon { name: "mic"; size: 15; color: settingsVM.micMode === "toggle" ? Theme.emerald400 : Theme.zinc400 }
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("Натиснув — говориш (Toggle)")
                        font.weight: Typography.semibold
                        font.pixelSize: Typography.sm
                    }
                    Badge {
                        visible: settingsVM.micMode === "toggle"
                        tone: "emerald"
                        text: qsTr("Активно")
                        fontSize: Typography.tiny
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 2
            spacing: 12

            Item {
                Layout.fillWidth: true
                implicitHeight: muteLabel.implicitHeight
                HoverHandler { id: muteHover }
                AppText {
                    id: muteLabel
                    text: qsTr("Мут мікрофона")
                    font.weight: Typography.medium
                }
                AppToolTip {
                    visible: muteHover.hovered
                    text: qsTr("Коли увімкнено — мікрофон блокується й не записує голос навіть при натисканні хоткея.")
                }
            }
            ToggleSwitch {
                checked: micService.muted
                onToggled: micService.setMuted(!micService.muted)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item {
                Layout.fillWidth: true
                implicitHeight: camScoreRow.implicitHeight
                HoverHandler { id: camScoreHover }

                RowLayout {
                    id: camScoreRow
                    spacing: 8
                    AppText {
                        text: qsTr("Враховувати камеру в оцінці")
                        font.weight: Typography.medium
                    }
                    Badge {
                        tone: settingsVM.includeCameraInScore ? "emerald" : "zinc"
                        mono: true
                        text: settingsVM.includeCameraInScore ? "15%" : qsTr("ВИМК.")
                        fontSize: Typography.tiny
                    }
                }
                AppToolTip {
                    visible: camScoreHover.hovered
                    text: qsTr("Тільки коли веб-камера увімкнена, AI-інтерв'юер враховує присутність у кадрі, освітлення, поставу, жести й емоції як 5-й критерій при оцінці кожної відповіді та у фінальному звіті.")
                }
            }
            ToggleSwitch {
                checked: settingsVM.includeCameraInScore
                onToggled: settingsVM.includeCameraInScore = !settingsVM.includeCameraInScore
            }
        }
    }

    Section {
        visible: page.activeTab === 1
        heading: qsTr("Гарячі клавіші")

        RowLayout {
            visible: page.listeningAction.length > 0
            Layout.fillWidth: true
            AppText {
                Layout.fillWidth: true
                text: qsTr("Натисніть клавішу або комбінацію (Esc — скасувати)")
                color: Theme.blue300
                font.pixelSize: Typography.xs
            }
        }

        Repeater {
            model: [
                {
                    id: "micTalk",
                    icon: "mic",
                    title: qsTr("Мікрофон (говорити)"),
                    desc: settingsVM.micMode === "hold"
                        ? qsTr("Утримання запускає запис (Parakeet STT), відпускання — надсилає.")
                        : qsTr("Натискання перемикає запис увімк./вимк. та надсилає текст."),
                    keys: settingsVM.micTalkShortcut
                },
                {
                    id: "micMute",
                    icon: "mic-off",
                    title: qsTr("Мут мікрофона"),
                    desc: qsTr("Швидко блокує або відновлює мікрофон."),
                    keys: settingsVM.micMuteShortcut
                },
                {
                    id: "cameraToggle",
                    icon: "video",
                    title: qsTr("Веб-камера"),
                    desc: qsTr("Запускає або зупиняє відеопотік вибраної камери."),
                    keys: settingsVM.cameraToggleShortcut
                },
                {
                    id: "aiMute",
                    icon: "bot",
                    title: qsTr("Голос AI"),
                    desc: qsTr("Перемикає озвучення AI-інтерв'юера Владислава."),
                    keys: settingsVM.aiMuteShortcut
                }
            ]

            Rectangle {
                id: bindRow
                required property var modelData
                readonly property bool isListening: page.listeningAction === modelData.id

                Layout.fillWidth: true
                implicitHeight: bindLayout.implicitHeight + 12
                radius: Theme.radiusMd
                color: isListening ? Theme.alpha(Theme.blue500, 0.12) : Theme.zinc900
                border.width: 1
                border.color: isListening ? Theme.blue500 : Theme.borderGlass

                HoverHandler { id: bindHover }

                AppToolTip {
                    visible: bindHover.hovered && !bindRow.isListening
                    text: bindRow.modelData.desc
                }

                RowLayout {
                    id: bindLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 10
                    spacing: 10

                    Icon {
                        name: bindRow.modelData.icon
                        size: 15
                        color: bindRow.isListening ? Theme.blue400 : Theme.zinc300
                    }

                    AppText {
                        Layout.fillWidth: true
                        text: bindRow.modelData.title
                        font.weight: Typography.medium
                        font.pixelSize: Typography.sm
                    }

                    Rectangle {
                        implicitWidth: Math.max(84, keyLabel.implicitWidth + 20)
                        implicitHeight: 28
                        radius: Theme.radiusXs
                        color: bindRow.isListening ? Theme.alpha(Theme.blue600, 0.25) : Theme.zinc800
                        border.width: 1
                        border.color: bindRow.isListening ? Theme.blue400 : Theme.borderStrong

                        MonoText {
                            id: keyLabel
                            anchors.centerIn: parent
                            text: bindRow.isListening ? qsTr("Натисніть...") : bindRow.modelData.keys
                            color: bindRow.isListening ? Theme.blue300 : Theme.zinc100
                            font.pixelSize: Typography.xs
                        }
                    }

                    TactileButton {
                        variant: bindRow.isListening ? "danger" : "secondary"
                        text: bindRow.isListening ? qsTr("Скасувати") : qsTr("Змінити")
                        fontSize: Typography.xs
                        implicitHeight: 28
                        leftPadding: 10
                        rightPadding: 10
                        onClicked: {
                            if (bindRow.isListening) {
                                page.stopShortcutCapture()
                            } else {
                                page.startShortcutCapture(bindRow.modelData.id)
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            TactileButton {
                variant: "ghost"
                iconLeft: "rotate-ccw"
                iconSize: 12
                text: qsTr("Скинути типові")
                fontSize: Typography.micro
                implicitHeight: 28
                leftPadding: 10
                rightPadding: 10
                onClicked: {
                    page.stopShortcutCapture()
                    settingsVM.resetShortcutsToDefaults()
                }
            }
        }
    }

    Section {
        visible: page.activeTab === 1
        heading: qsTr("Навігаційні комбінації")

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 32
            rowSpacing: 10

            Repeater {
                model: [
                    { keys: "Ctrl+1", action: qsTr("Home") },
                    { keys: "Ctrl+N, Ctrl+2", action: qsTr("New interview") },
                    { keys: "Ctrl+3", action: qsTr("Current interview") },
                    { keys: "Ctrl+,", action: qsTr("Account") },
                    { keys: "Ctrl+B", action: qsTr("Collapse or expand the sidebar") },
                    { keys: "Enter", action: qsTr("Send an answer") },
                    { keys: "Shift+Enter", action: qsTr("New line in an answer") }
                ]
                RowLayout {
                    id: shortcutRow
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 12
                    Rectangle {
                        implicitWidth: 128
                        implicitHeight: 26
                        radius: Theme.radiusXs
                        color: Theme.zinc900
                        border.width: 1
                        border.color: Theme.borderStrong
                        MonoText {
                            anchors.centerIn: parent
                            text: shortcutRow.modelData.keys
                            color: Theme.zinc200
                            font.pixelSize: Typography.tiny
                        }
                    }
                    AppText {
                        Layout.fillWidth: true
                        text: shortcutRow.modelData.action
                        color: Theme.zinc300
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------ Sign out
    RowLayout {
        visible: page.activeTab === 0
        Layout.fillWidth: true
        Layout.maximumWidth: 760
        spacing: 12

        MonoText {
            Layout.fillWidth: true
            text: "Dev Dialogue " + Qt.application.version
            color: Theme.textMuted
        }
        TactileButton {
            variant: "danger"
            text: qsTr("Sign out")
            iconLeft: "log-out"
            iconSize: 14
            fontSize: Typography.xs
            implicitHeight: 36
            onClicked: authVM.logout()
        }
    }
}
