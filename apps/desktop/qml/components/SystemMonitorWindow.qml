pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import DevDialogue.App

// Separate floating mini-window toggled via Shift+Alt+X (works even during startup).
// Displays real-time CPU and RAM footprint of Dev Dialogue's own processes only.
Window {
    id: win

    visible: systemMonitorVM.open
    width: 380
    height: 356
    minimumWidth: 340
    minimumHeight: 320
    title: qsTr("Dev Dialogue · Монітор навантаження")
    color: Theme.zinc950
    flags: Qt.Window | Qt.WindowStaysOnTopHint

    onVisibleChanged: {
        if (visible) {
            win.raise()
            win.requestActivate()
        }
    }

    onClosing: (close) => {
        close.accepted = false
        systemMonitorVM.open = false
    }

    function formatMem(mb) {
        if (mb >= 1024)
            return (mb / 1024).toFixed(2) + " GB"
        return Math.round(mb) + " MB"
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.zinc950
        border.width: 1
        border.color: Theme.borderGlass

        DragHandler {
            target: null
            onActiveChanged: {
                if (active)
                    win.startSystemMove()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // Header
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                PulseDot {
                    size: 8
                    color: Theme.emerald500
                    pingColor: Theme.emerald400
                    pinging: win.visible
                }

                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Навантаження застосунку")
                    font.pixelSize: Typography.sm
                    font.weight: Typography.bold
                }

                Badge {
                    tone: "blue"
                    mono: true
                    text: "Shift+Alt+X"
                    fontSize: Typography.micro
                }
            }

            // 3 Primary KPI cards: CPU %, Active RAM (Working Set), Occupied Memory (Private)
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 56
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderFaint

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2
                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("CPU")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: systemMonitorVM.totalCpuPercent.toFixed(1) + "%"
                            color: systemMonitorVM.totalCpuPercent >= 65 ? Theme.amber400 : Theme.emerald400
                            font.pixelSize: Typography.base
                            font.weight: Typography.bold
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 56
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderFaint

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2
                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("RAM (активна)")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: win.formatMem(systemMonitorVM.totalWorkingSetMb)
                            color: Theme.blue300
                            font.pixelSize: Typography.base
                            font.weight: Typography.bold
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 56
                    radius: Theme.radiusMd
                    color: Theme.zinc900
                    border.width: 1
                    border.color: Theme.borderFaint

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2
                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Зайнято (Commit)")
                            color: Theme.zinc400
                            font.pixelSize: Typography.micro
                        }
                        MonoText {
                            Layout.alignment: Qt.AlignHCenter
                            text: win.formatMem(systemMonitorVM.totalPrivateMb)
                            color: Theme.zinc200
                            font.pixelSize: Typography.base
                            font.weight: Typography.bold
                        }
                    }
                }
            }

            // Live CPU Sparkline (app-only history)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 44
                radius: Theme.radiusMd
                color: Theme.zinc900
                border.width: 1
                border.color: Theme.borderFaint
                clip: true

                Row {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 3

                    Repeater {
                        model: systemMonitorVM.cpuHistory
                        Item {
                            required property var modelData
                            width: Math.max(4, (parent.width - 27 * 3) / 28)
                            height: parent.height
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: Math.max(3, parent.height * Math.min(1.0, Number(modelData) / 100.0))
                                radius: 2
                                color: Number(modelData) >= 65 ? Theme.amber400 : Theme.emerald500
                            }
                        }
                    }
                }
            }

            // Per-process breakdown (strictly Dev Dialogue processes)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radiusMd
                color: Theme.zinc900
                border.width: 1
                border.color: Theme.borderFaint

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Repeater {
                        model: [
                            {
                                name: "DevDialogue + WebView2",
                                cpu: systemMonitorVM.desktopCpuPercent,
                                ws: systemMonitorVM.desktopWorkingSetMb,
                                priv: systemMonitorVM.desktopPrivateMb
                            },
                            {
                                name: "API Backend + Parakeet STT",
                                cpu: systemMonitorVM.apiCpuPercent,
                                ws: systemMonitorVM.apiWorkingSetMb,
                                priv: systemMonitorVM.apiPrivateMb
                            },
                            {
                                name: "Ollama (" + backendVM.localModelName + ")",
                                cpu: systemMonitorVM.ollamaCpuPercent,
                                ws: systemMonitorVM.ollamaWorkingSetMb,
                                priv: systemMonitorVM.ollamaPrivateMb
                            }
                        ]

                        RowLayout {
                            id: procRow
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                implicitWidth: 6
                                implicitHeight: 6
                                radius: 3
                                color: procRow.modelData.ws > 0 ? Theme.emerald500 : Theme.zinc600
                            }

                            AppText {
                                Layout.fillWidth: true
                                text: procRow.modelData.name
                                font.pixelSize: Typography.xs
                                color: Theme.zinc200
                                elide: Text.ElideRight
                            }

                            MonoText {
                                text: procRow.modelData.cpu.toFixed(1) + "%"
                                color: procRow.modelData.cpu >= 50 ? Theme.amber400 : Theme.emerald400
                                font.pixelSize: Typography.xs
                            }

                            MonoText {
                                Layout.preferredWidth: 108
                                horizontalAlignment: Text.AlignRight
                                text: win.formatMem(procRow.modelData.ws) + " / " + win.formatMem(procRow.modelData.priv)
                                color: Theme.zinc400
                                font.pixelSize: Typography.micro
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    MonoText {
                        Layout.fillWidth: true
                        text: qsTr("Процесів застосунку: %1 · Лише власне навантаження").arg(systemMonitorVM.processCount)
                        color: Theme.textMuted
                        font.pixelSize: Typography.micro
                    }
                }
            }
        }
    }
}
