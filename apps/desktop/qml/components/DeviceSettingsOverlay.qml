pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Google Meet / Microsoft Teams style permission & device picker modal:
// lets the user grant microphone/camera permissions, inspect Windows Privacy
// status, pick any detected microphone or webcam, and test audio/video live.
ModalOverlay {
    id: root

    property string pendingAction: ""
    property bool isUk: true

    signal approvedAction(string action)

    dialogWidth: 620
    dialogPadding: 22
    backdropOpacity: 0.78

    onOpenChanged: {
        if (open) {
            micService.refreshDevices()
            webcamService.refreshDevices()
        } else {
            pendingAction = ""
            micService.stopMicTest()
        }
    }

    onCloseRequested: {
        pendingAction = ""
        open = false
    }

    function permissionStatusLabel(granted, osAllowed) {
        if (!osAllowed) {
            return root.isUk ? "Заблоковано Windows" : qsTr("Blocked in Windows")
        }
        if (granted) {
            return root.isUk ? "Дозволено ✓" : qsTr("Allowed ✓")
        }
        return root.isUk ? "Потрібен дозвіл" : qsTr("Permission required")
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        // ------------------------------------------------------------- Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                implicitWidth: 34
                implicitHeight: 34
                radius: Theme.radiusMd
                color: Theme.alpha(Theme.blue500, 0.14)
                border.width: 1
                border.color: Theme.alpha(Theme.blue500, 0.35)

                Icon {
                    anchors.centerIn: parent
                    name: "shield-check"
                    size: 18
                    color: Theme.blue400
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                AppText {
                    text: root.isUk
                        ? "Мікрофон, камера та дозволи"
                        : qsTr("Microphone, Camera & Permissions")
                    font.pixelSize: Typography.base
                    font.weight: Typography.bold
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                }
                AppText {
                    Layout.fillWidth: true
                    text: root.pendingAction === "mic"
                        ? (root.isUk ? "Оберіть мікрофон та надайте доступ перед записом голосу" : qsTr("Grant access and select a microphone before recording"))
                        : (root.pendingAction === "cam"
                            ? (root.isUk ? "Оберіть веб-камеру та надайте доступ перед увімкненням відео" : qsTr("Grant access and select a webcam before enabling video"))
                            : (root.isUk ? "Налаштування аудіо- та відеопристроїв як у Google Meet / Teams" : qsTr("Configure audio & video devices like Google Meet / Teams")))
                    font.pixelSize: Typography.xs
                    color: Theme.textMuted
                    elide: Text.ElideRight
                }
            }

            TactileButton {
                variant: "secondary"
                iconLeft: "refresh-cw"
                iconSize: 13
                text: root.isUk ? "Оновити" : qsTr("Refresh")
                fontSize: Typography.xs
                implicitHeight: 32
                leftPadding: 10
                rightPadding: 10
                onClicked: {
                    micService.refreshDevices()
                    webcamService.refreshDevices()
                }
            }

            TactileButton {
                variant: "ghost"
                iconLeft: "x"
                iconSize: 15
                implicitWidth: 32
                implicitHeight: 32
                leftPadding: 0
                rightPadding: 0
                onClicked: root.closeRequested()
            }
        }

        // --------------------------------------------- OS Privacy Block Alert
        Rectangle {
            visible: !micService.osPermissionAllowed || !webcamService.osPermissionAllowed
            Layout.fillWidth: true
            implicitHeight: osAlertRow.implicitHeight + 20
            radius: Theme.radiusMd
            color: Theme.alpha(Theme.rose500, 0.12)
            border.width: 1
            border.color: Theme.alpha(Theme.rose500, 0.4)

            RowLayout {
                id: osAlertRow
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Icon {
                    name: "triangle-alert"
                    size: 16
                    color: Theme.rose400
                }

                AppText {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    font.pixelSize: Typography.xs
                    color: Theme.zinc200
                    text: root.isUk
                        ? "Доступ до мікрофона або камери вимкнено в налаштуваннях конфіденційності Windows. Відкрийте параметри Windows і дозвольте доступ для класичних програм."
                        : qsTr("Microphone or camera access is disabled in Windows Privacy Settings. Enable desktop app access to continue.")
                }

                TactileButton {
                    variant: "danger"
                    iconLeft: "external-link"
                    iconSize: 13
                    text: root.isUk ? "Параметри Windows" : qsTr("Windows Privacy")
                    fontSize: Typography.micro
                    implicitHeight: 30
                    leftPadding: 10
                    rightPadding: 10
                    onClicked: {
                        if (!micService.osPermissionAllowed)
                            micService.openPrivacySettings()
                        else
                            webcamService.openPrivacySettings()
                    }
                }
            }
        }

        // ----------------------------------------------------- Microphone Card
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: micCol.implicitHeight + 24
            radius: Theme.radiusLg
            color: "#111622"
            border.width: 1
            border.color: root.pendingAction === "mic"
                ? Theme.alpha(Theme.blue500, 0.55)
                : Theme.borderGlass

            ColumnLayout {
                id: micCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Icon {
                        name: "mic"
                        size: 15
                        color: Theme.blue400
                    }

                    AppText {
                        text: root.isUk ? "Мікрофон (Parakeet STT)" : qsTr("Microphone (Parakeet STT)")
                        font.pixelSize: Typography.sm
                        font.weight: Typography.semibold
                        color: Theme.textPrimary
                    }

                    Badge {
                        tone: !micService.osPermissionAllowed ? "rose" : (micService.permissionGranted ? "emerald" : "amber")
                        text: root.permissionStatusLabel(micService.permissionGranted, micService.osPermissionAllowed)
                        fontSize: Typography.micro
                    }

                    Item { Layout.fillWidth: true }

                    TactileButton {
                        variant: micService.permissionGranted ? "secondary" : "primary"
                        text: micService.permissionGranted
                            ? (root.isUk ? "Заборонити" : qsTr("Revoke"))
                            : (root.isUk ? "Дозволити доступ" : qsTr("Allow Mic"))
                        fontSize: Typography.micro
                        implicitHeight: 28
                        leftPadding: 10
                        rightPadding: 10
                        onClicked: {
                            if (micService.permissionGranted) {
                                micService.setPermissionGranted(false)
                            } else {
                                micService.grantPermission()
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    SelectBox {
                        id: micSelect
                        Layout.fillWidth: true
                        enabled: micService.availableMicrophones.length > 0
                        model: micService.availableMicrophones.length > 0
                            ? micService.availableMicrophones
                            : [{ value: -1, label: root.isUk ? "Мікрофони не знайдено" : qsTr("No microphones detected") }]
                        currentIndex: micService.availableMicrophones.length > 0
                            ? Math.max(0, Math.min(micService.selectedMicrophoneIndex, micService.availableMicrophones.length - 1))
                            : 0
                        onActivated: (idx) => {
                            if (micService.availableMicrophones.length > 0) {
                                micService.selectMicrophone(idx)
                            }
                        }
                    }

                    TactileButton {
                        variant: micService.testingMic ? "danger" : "soft"
                        enabled: micService.availableMicrophones.length > 0 && micService.osPermissionAllowed && !micService.recording
                        iconLeft: micService.testingMic ? "mic-off" : "mic"
                        iconSize: 14
                        text: micService.testingMic
                            ? (root.isUk ? "Стоп тест" : qsTr("Stop Test"))
                            : (root.isUk ? "Перевірити" : qsTr("Test Mic"))
                        fontSize: Typography.xs
                        implicitHeight: 36
                        leftPadding: 12
                        rightPadding: 12
                        onClicked: {
                            if (!micService.permissionGranted) {
                                micService.grantPermission()
                            }
                            micService.toggleMicTest()
                        }
                    }
                }

                // Live Meet-style audio level bar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    MonoText {
                        text: root.isUk ? "РІВЕНЬ" : "LEVEL"
                        font.pixelSize: Typography.micro
                        color: Theme.textMuted
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 8
                        radius: 4
                        color: Theme.zinc800
                        border.width: 1
                        border.color: Theme.zinc700

                        Rectangle {
                            width: parent.width * Math.min(1.0, Math.max(0.0, micService.inputLevel))
                            height: parent.height
                            radius: 4
                            color: micService.inputLevel > 0.72
                                ? Theme.amber400
                                : (micService.testingMic || micService.recording ? Theme.emerald500 : Theme.blue500)
                            Behavior on width { NumberAnimation { duration: 70 } }
                        }
                    }

                    MonoText {
                        Layout.preferredWidth: 42
                        horizontalAlignment: Text.AlignRight
                        text: Math.round(micService.inputLevel * 100) + "%"
                        font.pixelSize: Typography.micro
                        color: Theme.zinc300
                    }
                }
            }
        }

        // --------------------------------------------------------- Camera Card
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: camCol.implicitHeight + 24
            radius: Theme.radiusLg
            color: "#111622"
            border.width: 1
            border.color: root.pendingAction === "cam"
                ? Theme.alpha(Theme.blue500, 0.55)
                : Theme.borderGlass

            ColumnLayout {
                id: camCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Icon {
                        name: "video"
                        size: 15
                        color: Theme.blue400
                    }

                    AppText {
                        text: root.isUk ? "Веб-камера" : qsTr("Webcam")
                        font.pixelSize: Typography.sm
                        font.weight: Typography.semibold
                        color: Theme.textPrimary
                    }

                    Badge {
                        tone: !webcamService.osPermissionAllowed ? "rose" : (webcamService.permissionGranted ? "emerald" : "amber")
                        text: root.permissionStatusLabel(webcamService.permissionGranted, webcamService.osPermissionAllowed)
                        fontSize: Typography.micro
                    }

                    Item { Layout.fillWidth: true }

                    TactileButton {
                        variant: webcamService.permissionGranted ? "secondary" : "primary"
                        text: webcamService.permissionGranted
                            ? (root.isUk ? "Заборонити" : qsTr("Revoke"))
                            : (root.isUk ? "Дозволити доступ" : qsTr("Allow Cam"))
                        fontSize: Typography.micro
                        implicitHeight: 28
                        leftPadding: 10
                        rightPadding: 10
                        onClicked: {
                            if (webcamService.permissionGranted) {
                                webcamService.setPermissionGranted(false)
                            } else {
                                webcamService.grantPermission()
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    SelectBox {
                        id: camSelect
                        Layout.fillWidth: true
                        enabled: webcamService.availableCameras.length > 0
                        model: webcamService.availableCameras.length > 0
                            ? webcamService.availableCameras
                            : [{ value: -1, label: root.isUk ? "Веб-камери не знайдено" : qsTr("No webcams detected") }]
                        currentIndex: webcamService.availableCameras.length > 0
                            ? Math.max(0, Math.min(webcamService.selectedCameraIndex, webcamService.availableCameras.length - 1))
                            : 0
                        onActivated: (idx) => {
                            if (webcamService.availableCameras.length > 0) {
                                webcamService.selectCamera(idx)
                            }
                        }
                    }

                    TactileButton {
                        variant: webcamService.active ? "success" : "soft"
                        enabled: webcamService.availableCameras.length > 0 && webcamService.osPermissionAllowed
                        iconLeft: webcamService.active ? "video" : "video-off"
                        iconSize: 14
                        text: webcamService.active
                            ? (root.isUk ? "Прев'ю" : qsTr("Preview On"))
                            : (root.isUk ? "Увімк. прев'ю" : qsTr("Preview"))
                        fontSize: Typography.xs
                        implicitHeight: 36
                        leftPadding: 12
                        rightPadding: 12
                        onClicked: {
                            if (!webcamService.permissionGranted) {
                                webcamService.grantPermission()
                            }
                            webcamService.toggleCamera()
                        }
                    }
                }

                // Compact live Meet-style camera preview box
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 150
                    radius: Theme.radiusMd
                    color: "#090c14"
                    border.width: 1
                    border.color: webcamService.active && webcamService.hasFrame
                        ? Theme.alpha(Theme.emerald500, 0.45)
                        : Theme.borderFaint
                    clip: true

                    Image {
                        id: modalCamPreview
                        anchors.fill: parent
                        anchors.margins: 1
                        visible: webcamService.active && webcamService.hasFrame
                        source: webcamService.active ? ("image://webcam/frame?" + webcamService.frameRevision) : ""
                        cache: false
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        transform: Scale { origin.x: modalCamPreview.width / 2; xScale: -1 }
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: !(webcamService.active && webcamService.hasFrame)
                        spacing: 6

                        Icon {
                            Layout.alignment: Qt.AlignHCenter
                            name: webcamService.active ? "eye" : "eye-off"
                            size: 20
                            color: Theme.textMuted
                        }

                        AppText {
                            Layout.alignment: Qt.AlignHCenter
                            font.pixelSize: Typography.xs
                            color: Theme.textMuted
                            text: webcamService.statusText.length > 0
                                ? webcamService.statusText
                                : (webcamService.availableCameras.length > 0
                                    ? (root.isUk ? "Натисніть «Увімк. прев'ю» для перевірки вибраної камери" : qsTr("Click Preview to test the selected camera"))
                                    : (root.isUk ? "Підключіть камеру та натисніть «Оновити»" : qsTr("Connect a camera and click Refresh")))
                        }
                    }
                }

                // Official Camera Scoring Toggle Switch
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        RowLayout {
                            spacing: 6
                            AppText {
                                text: root.isUk
                                    ? "Враховувати камеру при оцінці відповідей"
                                    : qsTr("Include camera in answer scoring")
                                font.pixelSize: Typography.xs
                                font.weight: Typography.medium
                                color: Theme.textPrimary
                            }
                            Badge {
                                tone: webcamService.includeInScore ? "emerald" : "zinc"
                                mono: true
                                text: webcamService.includeInScore
                                    ? (root.isUk ? "15% БАЛУ" : "15% WEIGHT")
                                    : (root.isUk ? "ВИМК." : "OFF")
                                fontSize: Typography.micro
                            }
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: root.isUk
                                ? "Активно лише коли камера увімкнена: агент оцінює кадр, освітлення, поставу та емоції."
                                : qsTr("Active only when camera is ON: agent scores presence, lighting, posture & emotion.")
                            font.pixelSize: Typography.micro
                            color: Theme.zinc400
                            wrapMode: Text.WordWrap
                        }
                    }

                    ToggleSwitch {
                        checked: webcamService.includeInScore
                        onToggled: webcamService.includeInScore = !webcamService.includeInScore
                    }
                }
            }
        }

        // -------------------------------------------------------------- Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            MonoText {
                Layout.fillWidth: true
                font.pixelSize: Typography.micro
                color: Theme.textMuted
                text: root.isUk
                    ? "Знайдено: %1 мікр. · %2 камер".arg(micService.availableMicrophones.length).arg(webcamService.availableCameras.length)
                    : qsTr("Detected: %1 mic(s) · %2 camera(s)").arg(micService.availableMicrophones.length).arg(webcamService.availableCameras.length)
            }

            TactileButton {
                variant: "secondary"
                text: root.isUk ? "Закрити" : qsTr("Cancel")
                fontSize: Typography.xs
                implicitHeight: 36
                leftPadding: 14
                rightPadding: 14
                onClicked: root.closeRequested()
            }

            TactileButton {
                variant: "primary"
                glow: true
                iconLeft: "circle-check-big"
                iconSize: 15
                text: (!micService.permissionGranted || !webcamService.permissionGranted)
                    ? (root.isUk ? "Дозволити та продовжити" : qsTr("Allow All & Continue"))
                    : (root.isUk ? "Готово" : qsTr("Done"))
                fontSize: Typography.xs
                implicitHeight: 36
                leftPadding: 16
                rightPadding: 16
                onClicked: {
                    if (!micService.permissionGranted) {
                        micService.grantPermission()
                    }
                    if (!webcamService.permissionGranted) {
                        webcamService.grantPermission()
                    }
                    const nextAction = root.pendingAction
                    root.pendingAction = ""
                    root.open = false
                    if (nextAction.length > 0) {
                        root.approvedAction(nextAction)
                    }
                }
            }
        }
    }
}
