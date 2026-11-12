import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: dashboardView

    onVisibleChanged: {
        if (visible && authVM.isAuthenticated)
            dashboardVM.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        // ---------------------------------------------------------------
        // Welcome header
        // ---------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: qsTr("Welcome back, %1").arg(authVM.userFullName)
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.h1
                    font.weight: Typography.weightBold
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: qsTr("Track your progress and jump into the next practice session.")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.body
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            TactileButton {
                visible: authVM.isAdmin
                text: qsTr("Question bank")
                outlined: true
                implicitWidth: 150
                onClicked: dashboardVM.openAdmin()
            }

            TactileButton {
                text: qsTr("Refresh")
                outlined: true
                implicitWidth: 110
                iconSource: Icons.refresh
                enabled: !dashboardVM.isBusy
                onClicked: dashboardVM.refresh()
            }

            TactileButton {
                text: qsTr("Start interview")
                implicitWidth: 180
                iconSource: Icons.plus
                enabled: !dashboardVM.isBusy
                onClicked: dashboardVM.startNewInterview()
            }
        }

        ErrorBanner {
            Layout.fillWidth: true
            errorMessage: dashboardVM.errorMessage
            onDismissed: dashboardVM.clearError()
        }

        // ---------------------------------------------------------------
        // KPI tiles
        // ---------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            KpiTile {
                Layout.fillWidth: true
                Layout.preferredHeight: 132
                title: qsTr("Interviews completed")
                value: dashboardVM.totalInterviews
                hint: qsTr("Total sessions finished")
                iconSource: Icons.chart
                accentColor: Theme.accentIndigo
            }

            KpiTile {
                Layout.fillWidth: true
                Layout.preferredHeight: 132
                title: qsTr("Average score")
                value: dashboardVM.averageScore.toFixed(1)
                suffix: qsTr("/ 100")
                hint: Labels.scoreLabel(dashboardVM.averageScore)
                accentColor: Labels.scoreColor(dashboardVM.averageScore)
                progress: dashboardVM.averageScore / 100
            }

            KpiTile {
                Layout.fillWidth: true
                Layout.preferredHeight: 132
                title: qsTr("Daily limit")
                value: dashboardVM.dailyInterviewsCount + " / " + dashboardVM.dailyLimit
                hint: qsTr("%1 remaining today").arg(dashboardVM.remainingToday)
                accentColor: Theme.accentAmber
                progress: dashboardVM.dailyLimit > 0
                          ? dashboardVM.dailyInterviewsCount / dashboardVM.dailyLimit
                          : 0
            }
        }

        // ---------------------------------------------------------------
        // Session history
        // ---------------------------------------------------------------
        DarkCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentPadding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 16
                    Layout.bottomMargin: 12
                    spacing: 12

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Session history")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.h3
                        font.weight: Typography.weightSemibold
                        color: Theme.textPrimary
                    }

                    Spinner {
                        visible: dashboardVM.isBusy
                        diameter: 18
                    }

                    Text {
                        text: qsTr("%1 sessions").arg(dashboardVM.interviewsModel.count)
                        font.family: Theme.fontMono
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: historyList
                        anchors.fill: parent
                        clip: true
                        spacing: 0
                        model: dashboardVM.interviewsModel

                        delegate: Item {
                            width: ListView.view ? ListView.view.width : 0
                            height: 64

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 20
                                anchors.rightMargin: 16
                                spacing: 14

                                Rectangle {
                                    Layout.preferredWidth: 34
                                    Layout.preferredHeight: 34
                                    radius: Theme.radiusSm
                                    color: Qt.rgba(0.39, 0.4, 0.95, 0.12)
                                    border.width: 1
                                    border.color: Qt.rgba(0.39, 0.4, 0.95, 0.3)

                                    Image {
                                        anchors.centerIn: parent
                                        source: Labels.directionIcon(model.direction)
                                        sourceSize.width: 17
                                        sourceSize.height: 17
                                        width: 17
                                        height: 17
                                        fillMode: Image.PreserveAspectFit
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        text: Labels.directionLabel(model.direction) + " · "
                                              + Labels.levelLabel(model.level)
                                        font.family: Theme.fontSans
                                        font.pixelSize: Typography.body
                                        font.weight: Typography.weightMedium
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: Labels.formatDateTime(model.createdAt)
                                              + " · " + qsTr("%1 answers").arg(model.questionCount)
                                        font.family: Theme.fontMono
                                        font.pixelSize: Typography.tiny
                                        color: Theme.textMuted
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                Badge {
                                    text: Labels.statusLabel(model.status)
                                    toneColor: Labels.statusColor(model.status)
                                }

                                Text {
                                    Layout.preferredWidth: 70
                                    horizontalAlignment: Text.AlignRight
                                    text: model.hasScore ? model.score + qsTr("/100") : "—"
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.body
                                    font.weight: Typography.weightSemibold
                                    color: model.hasScore ? Labels.scoreColor(model.score)
                                                          : Theme.textMuted
                                }

                                TactileButton {
                                    implicitWidth: 104
                                    implicitHeight: 32
                                    buttonRadius: Theme.radiusSm
                                    outlined: true
                                    text: model.status === "completed"
                                          ? qsTr("Open report")
                                          : qsTr("Resume")
                                    onClicked: dashboardVM.openInterview(model.interviewId,
                                                                         model.status)
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 20
                                height: 1
                                color: Theme.borderSubtle
                                opacity: 0.6
                            }
                        }
                    }

                    EmptyState {
                        anchors.fill: parent
                        visible: dashboardVM.interviewsModel.count === 0
                                 && !dashboardVM.isBusy
                        iconSource: Icons.sparkles
                        title: qsTr("No interviews yet")
                        description: qsTr("Set up your first session and the AI interviewer will guide you through a realistic technical conversation.")
                        actionText: qsTr("Start your first interview")
                        onActionClicked: dashboardVM.startNewInterview()
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 12
                        visible: dashboardVM.interviewsModel.count === 0
                                 && dashboardVM.isBusy

                        Spinner {
                            anchors.horizontalCenter: parent.horizontalCenter
                            diameter: 26
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Loading sessions…")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.small
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }
    }
}
