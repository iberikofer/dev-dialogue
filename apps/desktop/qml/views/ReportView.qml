import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: reportView

    onVisibleChanged: {
        if (visible && navVM.activeInterviewId !== ""
                && reportVM.interviewId !== navVM.activeInterviewId)
            reportVM.loadReport(navVM.activeInterviewId)
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 48
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: contentColumn
            x: 24
            y: 24
            width: parent.width - 48
            spacing: 18

            // -----------------------------------------------------------
            // Header
            // -----------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        spacing: 8

                        Text {
                            text: qsTr("Interview report")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.h1
                            font.weight: Typography.weightBold
                            color: Theme.textPrimary
                        }

                        Badge {
                            text: Labels.directionLabel(reportVM.direction)
                            toneColor: Theme.accentIndigo
                        }

                        Badge {
                            text: Labels.levelLabel(reportVM.level)
                            toneColor: Theme.accentRose
                        }

                        Badge {
                            text: Labels.languageLabel(reportVM.language)
                            toneColor: Theme.accentTeal
                        }
                    }

                    Text {
                        text: qsTr("Detailed scorecard generated from your interview transcript.")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.body
                        color: Theme.textSecondary
                    }
                }

                TactileButton {
                    text: qsTr("Dashboard")
                    outlined: true
                    implicitWidth: 120
                    onClicked: reportVM.backToDashboard()
                }

                TactileButton {
                    text: qsTr("New interview")
                    implicitWidth: 160
                    iconSource: Icons.plus
                    onClicked: reportVM.startNewInterview()
                }
            }

            ErrorBanner {
                Layout.fillWidth: true
                errorMessage: reportVM.errorMessage
                onDismissed: reportVM.clearError()
            }

            // -----------------------------------------------------------
            // Score + summary
            // -----------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 18
                visible: reportVM.hasReport

                DarkCard {
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 250
                    contentPadding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        ScoreGauge {
                            Layout.alignment: Qt.AlignHCenter
                            diameter: 150
                            value: reportVM.score
                            caption: Labels.scoreLabel(reportVM.score)
                        }

                        Item { Layout.fillHeight: true }

                        Badge {
                            Layout.alignment: Qt.AlignHCenter
                            text: Labels.statusLabel(reportVM.status)
                            toneColor: Labels.statusColor(reportVM.status)
                        }
                    }
                }

                DarkCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 250
                    contentPadding: 20

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            text: qsTr("Executive summary")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.h3
                            font.weight: Typography.weightSemibold
                            color: Theme.textPrimary
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: reportVM.summary
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.body
                            lineHeight: Typography.lineNormal
                            color: Theme.textSecondary
                            wrapMode: Text.Wrap
                            verticalAlignment: Text.AlignTop
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.borderSubtle
                        }

                        Text {
                            text: reportVM.questionScores.length > 0
                                  ? qsTr("%1 questions evaluated").arg(reportVM.questionScores.length)
                                  : qsTr("No per-question breakdown available")
                            font.family: Theme.fontMono
                            font.pixelSize: Typography.tiny
                            color: Theme.textMuted
                        }
                    }
                }
            }

            // -----------------------------------------------------------
            // Strengths and weaknesses
            // -----------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 18
                visible: reportVM.hasReport

                DarkCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 210
                    contentPadding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        RowLayout {
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 8
                                Layout.preferredHeight: 8
                                Layout.alignment: Qt.AlignVCenter
                                radius: 4
                                color: Theme.accentEmerald
                            }

                            Text {
                                text: qsTr("Strengths")
                                font.family: Theme.fontSans
                                font.pixelSize: Typography.h3
                                font.weight: Typography.weightSemibold
                                color: Theme.textPrimary
                            }
                        }

                        Repeater {
                            model: reportVM.strengths

                            delegate: RowLayout {
                                required property string modelData
                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    text: "•"
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.body
                                    color: Theme.accentEmerald
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.bodySmall
                                    lineHeight: Typography.lineNormal
                                    color: Theme.textSecondary
                                    wrapMode: Text.Wrap
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                DarkCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 210
                    contentPadding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        RowLayout {
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 8
                                Layout.preferredHeight: 8
                                Layout.alignment: Qt.AlignVCenter
                                radius: 4
                                color: Theme.accentRose
                            }

                            Text {
                                text: qsTr("Growth areas")
                                font.family: Theme.fontSans
                                font.pixelSize: Typography.h3
                                font.weight: Typography.weightSemibold
                                color: Theme.textPrimary
                            }
                        }

                        Repeater {
                            model: reportVM.weaknesses

                            delegate: RowLayout {
                                required property string modelData
                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    text: "•"
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.body
                                    color: Theme.accentRose
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.bodySmall
                                    lineHeight: Typography.lineNormal
                                    color: Theme.textSecondary
                                    wrapMode: Text.Wrap
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // -----------------------------------------------------------
            // Recommendations
            // -----------------------------------------------------------
            DarkCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 60 + reportVM.recommendations.length * 56
                visible: reportVM.hasReport

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Text {
                        text: qsTr("Recommended study resources")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.h3
                        font.weight: Typography.weightSemibold
                        color: Theme.textPrimary
                    }

                    Repeater {
                        model: reportVM.recommendations

                        delegate: Rectangle {
                            id: recommendationRow
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            radius: Theme.radiusSm
                            color: recommendationHover.hovered
                                   ? Theme.backgroundElevated
                                   : Theme.backgroundSecondary
                            border.width: 1
                            border.color: Theme.borderSubtle

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 10

                                Image {
                                    source: Icons.sparkles
                                    sourceSize.width: 16
                                    sourceSize.height: 16
                                    Layout.preferredWidth: 16
                                    Layout.preferredHeight: 16
                                    fillMode: Image.PreserveAspectFit
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Text {
                                        text: recommendationRow.modelData.topic
                                        font.family: Theme.fontSans
                                        font.pixelSize: Typography.bodySmall
                                        font.weight: Typography.weightMedium
                                        color: Theme.textPrimary
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: recommendationRow.modelData.resource
                                        font.family: Theme.fontMono
                                        font.pixelSize: Typography.tiny
                                        color: Theme.textMuted
                                        elide: Text.ElideRight
                                    }
                                }

                                Text {
                                    text: qsTr("Open ↗")
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.tiny
                                    color: Theme.accentIndigo
                                }
                            }

                            HoverHandler {
                                id: recommendationHover
                                cursorShape: Qt.PointingHandCursor
                            }

                            TapHandler {
                                onTapped: reportVM.openRecommendation(
                                              recommendationRow.modelData.resource)
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------
            // Question breakdown
            // -----------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12
                visible: reportVM.hasReport && reportVM.questionScores.length > 0

                Text {
                    text: qsTr("Question breakdown")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.h2
                    font.weight: Typography.weightSemibold
                    color: Theme.textPrimary
                }

                Repeater {
                    model: reportVM.questionScores

                    delegate: DarkCard {
                        id: questionCard
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 124
                        contentPadding: 16

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Text {
                                    Layout.fillWidth: true
                                    text: questionCard.modelData.question
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.body
                                    font.weight: Typography.weightMedium
                                    lineHeight: Typography.lineNormal
                                    color: Theme.textPrimary
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                Badge {
                                    text: questionCard.modelData.score + qsTr("/100")
                                    toneColor: Labels.scoreColor(questionCard.modelData.score)
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: questionCard.modelData.comment
                                font.family: Theme.fontSans
                                font.pixelSize: Typography.small
                                lineHeight: Typography.lineNormal
                                color: Theme.textSecondary
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 5
                                radius: 2.5
                                color: Theme.backgroundElevated

                                Rectangle {
                                    width: parent.width * Math.max(0, Math.min(1,
                                                questionCard.modelData.score / 100))
                                    height: parent.height
                                    radius: parent.radius
                                    color: Labels.scoreColor(questionCard.modelData.score)
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------
            // Empty / loading states
            // -----------------------------------------------------------
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                visible: !reportVM.hasReport

                Column {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: reportVM.isBusy

                    Spinner {
                        anchors.horizontalCenter: parent.horizontalCenter
                        diameter: 28
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Building your scorecard…")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }
                }

                EmptyState {
                    anchors.fill: parent
                    visible: !reportVM.isBusy
                    iconSource: Icons.chart
                    title: qsTr("Report not ready")
                    description: qsTr("Finish an interview to generate a scorecard with per-question feedback and study recommendations.")
                    actionText: qsTr("Back to dashboard")
                    onActionClicked: reportVM.backToDashboard()
                }
            }
        }
    }
}
