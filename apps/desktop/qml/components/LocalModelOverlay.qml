import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Control center for verifying, starting, and restarting the API Backend,
// 3D Avatar & Lip-Sync servers (:8080/:8085/WebView2), and local AI models
// (Ollama lfm3-omni:2b + Parakeet TDT 0.6B v3 STT).
ModalOverlay {
    id: dialog

    dialogWidth: 590
    closeOnBackdrop: !backendVM.isDownloadingModel

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14

        // ------------------------------------------------------------- Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                implicitWidth: 44
                implicitHeight: 44
                radius: Theme.radiusMd
                color: backendVM.allServicesReady
                    ? Theme.alpha(Theme.emerald500, 0.14)
                    : Theme.alpha(Theme.amber500, 0.14)
                border.width: 1
                border.color: backendVM.allServicesReady
                    ? Theme.alpha(Theme.emerald500, 0.35)
                    : Theme.alpha(Theme.amber500, 0.35)

                Icon {
                    anchors.centerIn: parent
                    name: backendVM.allServicesReady ? "circle-check" : "server"
                    size: 22
                    color: backendVM.allServicesReady ? Theme.emerald400 : Theme.amber400
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                AppText {
                    Layout.fillWidth: true
                    text: backendVM.allServicesReady
                        ? qsTr("Сервери, 3D-модель та локальні AI-моделі готові")
                        : qsTr("Перевірка та керування серверами і моделями")
                    font.pixelSize: Typography.lg
                    font.weight: Typography.semibold
                }

                MonoText {
                    text: backendVM.servicesActionText !== ""
                        ? backendVM.servicesActionText
                        : qsTr("Backend :5238 · 3D Avatar (:8080/:8085) · Ollama (%1) · Parakeet STT").arg(backendVM.localModelName)
                    color: Theme.zinc400
                    font.pixelSize: Typography.xs
                    elide: Text.ElideRight
                }
            }
        }

        // ------------------------------------ 4-Service Checklist & Controls
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            // 1. Main API Backend (:5238)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 54
                radius: Theme.radiusMd
                color: Theme.zinc900
                border.width: 1
                border.color: !backendVM.isOffline ? Theme.alpha(Theme.emerald500, 0.35) : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    PulseDot {
                        size: 9
                        color: !backendVM.isOffline ? Theme.emerald500 : Theme.amber500
                        pingColor: !backendVM.isOffline ? Theme.emerald400 : Theme.amber400
                        pinging: backendVM.isOffline || backendVM.isManagingServices
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        AppText {
                            text: qsTr("Основний API Backend сервер")
                            font.pixelSize: Typography.sm
                            font.weight: Typography.semibold
                        }
                        MonoText {
                            text: !backendVM.isOffline
                                ? qsTr("Активний (http://localhost:5238 · SQLite / REST API)")
                                : qsTr("Офлайн — натисніть «Запустити / Перезапустити»")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                    }

                    Badge {
                        tone: !backendVM.isOffline ? "emerald" : "amber"
                        mono: true
                        text: !backendVM.isOffline ? qsTr("ОНЛАЙН") : qsTr("ОФЛАЙН")
                        fontSize: Typography.micro
                    }

                    TactileButton {
                        variant: "secondary"
                        iconLeft: "refresh-cw"
                        iconSize: 12
                        text: !backendVM.isOffline ? qsTr("Перезапустити") : qsTr("Запустити")
                        fontSize: Typography.micro
                        implicitHeight: 28
                        leftPadding: 10
                        rightPadding: 10
                        enabled: !backendVM.isManagingServices
                        onClicked: backendVM.restartApiBackend()
                    }
                }
            }

            // 2. 3D Avatar & Lip-Sync Servers (:8080 / :8085 / WebView2)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 54
                radius: Theme.radiusMd
                color: Theme.zinc900
                border.width: 1
                border.color: (backendVM.avatarServerReady && avatarHost.ready)
                    ? Theme.alpha(Theme.emerald500, 0.35)
                    : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    PulseDot {
                        size: 9
                        color: (backendVM.avatarServerReady && avatarHost.ready) ? Theme.emerald500 : Theme.amber500
                        pingColor: Theme.emerald400
                        pinging: !backendVM.avatarServerReady || !avatarHost.ready
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        AppText {
                            text: qsTr("Back-end сервери 3D-моделі та Lip-Sync")
                            font.pixelSize: Typography.sm
                            font.weight: Typography.semibold
                        }
                        MonoText {
                            text: qsTr("WebGL :8080 (%1) · TTS/Rhubarb :8085 (%2) · WebView2 (%3)")
                                .arg(backendVM.avatarHttp8080Running ? "ON" : "AUTO")
                                .arg(backendVM.avatarTts8085Running || backendVM.rhubarbAvailable ? "ON" : "OFF")
                                .arg(avatarHost.ready ? "Ready" : "Loading")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                    }

                    Badge {
                        tone: (backendVM.avatarServerReady && avatarHost.ready) ? "emerald" : "amber"
                        mono: true
                        text: (backendVM.avatarServerReady && avatarHost.ready) ? qsTr("ГОТОВО") : qsTr("ОЧІКУЄ")
                        fontSize: Typography.micro
                    }

                    TactileButton {
                        variant: "secondary"
                        iconLeft: "refresh-cw"
                        iconSize: 12
                        text: qsTr("Перезапустити 3D")
                        fontSize: Typography.micro
                        implicitHeight: 28
                        leftPadding: 10
                        rightPadding: 10
                        enabled: !backendVM.isManagingServices
                        onClicked: {
                            backendVM.restartAvatarServers()
                            avatarHost.reload()
                        }
                    }
                }
            }

            // 3. Local LLM / Vision Model (Ollama :11434 + lfm3-omni:2b)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 54
                radius: Theme.radiusMd
                color: Theme.zinc900
                border.width: 1
                border.color: (backendVM.localModelReady && backendVM.ollamaRunning)
                    ? Theme.alpha(Theme.emerald500, 0.35)
                    : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    PulseDot {
                        size: 9
                        color: (backendVM.localModelReady && backendVM.ollamaRunning) ? Theme.emerald500 : Theme.amber500
                        pingColor: Theme.amber400
                        pinging: !backendVM.localModelReady || !backendVM.ollamaRunning || backendVM.isDownloadingModel
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        AppText {
                            text: qsTr("Локальна AI-модель співбесіди та зору (%1)").arg(backendVM.localModelName)
                            font.pixelSize: Typography.sm
                            font.weight: Typography.semibold
                        }
                        MonoText {
                            text: qsTr("Ollama :11434 (%1) · Модель: %2")
                                .arg(backendVM.ollamaRunning ? "Запущено" : "Зупинено")
                                .arg(backendVM.localModelReady ? "Встановлена" : "Не завантажена")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                    }

                    Badge {
                        tone: (backendVM.localModelReady && backendVM.ollamaRunning) ? "emerald" : "amber"
                        mono: true
                        text: (backendVM.localModelReady && backendVM.ollamaRunning)
                            ? qsTr("АКТИВНО")
                            : (backendVM.localModelReady ? qsTr("ЗАПУСК") : qsTr("ПОТРІБНО СКАЧАТИ"))
                        fontSize: Typography.micro
                    }

                    TactileButton {
                        variant: backendVM.localModelReady ? "secondary" : "primary"
                        iconLeft: backendVM.localModelReady ? "refresh-cw" : "bot"
                        iconSize: 12
                        text: backendVM.localModelReady ? qsTr("Перезапустити") : qsTr("Завантажити")
                        fontSize: Typography.micro
                        implicitHeight: 28
                        leftPadding: 10
                        rightPadding: 10
                        enabled: !backendVM.isManagingServices && !backendVM.isDownloadingModel
                        onClicked: {
                            if (backendVM.localModelReady)
                                backendVM.restartLocalModels()
                            else
                                backendVM.downloadLocalModel()
                        }
                    }
                }
            }

            // 4. Local Speech-to-Text Model (Parakeet TDT 0.6B v3 ONNX)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 50
                radius: Theme.radiusMd
                color: Theme.zinc900
                border.width: 1
                border.color: backendVM.sttModelReady
                    ? Theme.alpha(Theme.emerald500, 0.35)
                    : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    PulseDot {
                        size: 9
                        color: backendVM.sttModelReady ? Theme.emerald500 : Theme.amber500
                        pingColor: Theme.emerald400
                        pinging: false
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        AppText {
                            text: qsTr("Локальна STT-модель мікрофона (%1)").arg(backendVM.sttModelName)
                            font.pixelSize: Typography.sm
                            font.weight: Typography.semibold
                        }
                        MonoText {
                            text: backendVM.sttModelReady
                                ? qsTr("SherpaOnnx INT8 Transducer — готова до розпізнавання мовлення")
                                : qsTr("Файли моделі не знайдено")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                    }

                    Badge {
                        tone: backendVM.sttModelReady ? "emerald" : "rose"
                        mono: true
                        text: backendVM.sttModelReady ? qsTr("ГОТОВО") : qsTr("ВІДСУТНЯ")
                        fontSize: Typography.micro
                    }
                }
            }
        }

        // Download progress box when pulling local Ollama model
        Rectangle {
            visible: backendVM.isDownloadingModel
            Layout.fillWidth: true
            implicitHeight: statusCol.implicitHeight + 24
            radius: Theme.radiusMd
            color: Theme.zinc900
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                id: statusCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    PulseDot {
                        size: 8
                        color: Theme.blue400
                        pinging: true
                    }

                    AppText {
                        Layout.fillWidth: true
                        text: backendVM.localModelStatus !== ""
                            ? backendVM.localModelStatus
                            : qsTr("Завантаження %1... %2%").arg(backendVM.localModelName).arg(backendVM.modelDownloadProgress)
                        font.pixelSize: Typography.xs
                        color: Theme.zinc200
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 6
                    radius: 3
                    color: Theme.zinc800

                    Rectangle {
                        height: parent.height
                        radius: 3
                        width: parent.width * Math.max(0.05, Math.min(1.0, backendVM.modelDownloadProgress / 100.0))
                        color: Theme.blue500
                        Behavior on width { NumberAnimation { duration: 250 } }
                    }
                }
            }
        }

        AlertBox {
            visible: backendVM.localModelError !== ""
            Layout.fillWidth: true
            text: backendVM.localModelError
        }

        // ------------------------------------------------------------- Footer
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 10

            TactileButton {
                variant: "secondary"
                iconLeft: "shield-check"
                iconSize: 14
                text: backendVM.isManagingServices ? qsTr("Перевірка...") : qsTr("Перевірити стан")
                implicitHeight: 40
                enabled: !backendVM.isDownloadingModel && !backendVM.isManagingServices
                onClicked: backendVM.checkAllServices()
            }

            TactileButton {
                Layout.fillWidth: true
                variant: backendVM.allServicesReady ? "secondary" : "primary"
                iconLeft: "refresh-cw"
                iconSize: 14
                text: backendVM.allServicesReady
                    ? qsTr("Перезапустити всі сервери і моделі")
                    : qsTr("Запустити / Перезапустити все")
                busy: backendVM.isManagingServices
                enabled: !backendVM.isDownloadingModel && !backendVM.isManagingServices
                implicitHeight: 40
                onClicked: backendVM.restartAllServices()
            }

            TactileButton {
                variant: backendVM.allServicesReady ? "primary" : "ghost"
                glow: backendVM.allServicesReady
                text: backendVM.allServicesReady ? qsTr("Продовжити") : qsTr("Закрити")
                implicitHeight: 40
                enabled: !backendVM.isDownloadingModel
                onClicked: dialog.closeRequested()
            }
        }
    }
}
