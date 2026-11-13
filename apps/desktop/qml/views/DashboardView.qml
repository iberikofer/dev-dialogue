pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Home: today's numbers and the interview history. The history table fills
// the window and scrolls on its own.
AppPage {
    id: page

    active: StackLayout.isCurrentItem

    // Load when the page is shown to a signed-in user; if the server turns
    // out to be unreachable meanwhile, switch to demo data at once instead of
    // waiting for the request to time out.
    readonly property bool live: active && authVM.isAuthenticated
    onLiveChanged: if (live) dashboardVM.refresh()
    Connections {
        target: backendVM
        function onStatusChanged() {
            if (backendVM.isOffline && dashboardVM.loading)
                dashboardVM.refresh()
        }
    }

    title: qsTr("Welcome back, %1").arg(settingsVM.displayName)
    subtitle: qsTr("Your interview practice at a glance.")
    badgeText: dashboardVM.isDemoData ? qsTr("Demo data") : ""

    actions: [
        TactileButton {
            text: qsTr("New interview")
            iconLeft: "circle-plus"
            implicitHeight: 40
            glow: true
            onClicked: navVM.navigateToSetup()
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: "Ctrl+N"
        }
    ]

    readonly property int rowHeight: 52
    // Column widths as fractions of the table width.
    readonly property var columns: [0.22, 0.18, 0.15, 0.17, 0.12, 0.16]
    function columnWidth(index, tableWidth) {
        return Math.floor((tableWidth - 48) * columns[index])
    }

    // ------------------------------------------------------------ KPI tiles
    RowLayout {
        Layout.fillWidth: true
        spacing: 16

        KpiTile {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            caption: qsTr("Interviews completed")
            description: qsTr("Total evaluated sessions")
            iconName: "history"
            iconColor: Theme.blue400
            value: dashboardVM.totalInterviews
            loading: dashboardVM.loading
            skeletonWidth: 80
        }
        KpiTile {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            caption: qsTr("Average score")
            description: qsTr("Across all evaluated sessions")
            iconName: "trending-up"
            iconColor: Theme.emerald400
            valueColor: Theme.emerald400
            value: dashboardVM.averageScoreText !== "" ? dashboardVM.averageScoreText + "%" : "—"
            loading: dashboardVM.loading
            skeletonWidth: 96
        }
        KpiTile {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            caption: qsTr("Left today")
            description: qsTr("Interviews you can still start today")
            iconName: "clock"
            iconColor: Theme.blue400
            value: dashboardVM.remainingToday
            valueSuffix: "/ " + dashboardVM.dailyLimit
            loading: dashboardVM.loading
            skeletonWidth: 112

            accessory: ProgressRing {
                size: 36
                thickness: 4
                value: dashboardVM.dailyLimit > 0 ? dashboardVM.dailyInterviewsCount / dashboardVM.dailyLimit : 0
                progressColor: Theme.blue400
            }
        }
    }

    // ------------------------------------------------------- History table
    Rectangle {
        id: table
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: Theme.radiusLg
        color: Theme.surfaceCard
        border.width: 1
        border.color: Theme.borderGlass
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.preferredHeight: 56

                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Interview history")
                    font.pixelSize: Typography.base
                    font.weight: Typography.bold
                }
                MonoText {
                    visible: !dashboardVM.loading
                    text: qsTr("Total: %1").arg(dashboardVM.interviewCount)
                }
            }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.borderGlass }

            // Loading
            ColumnLayout {
                visible: dashboardVM.loading
                Layout.fillWidth: true
                Layout.margins: 24
                spacing: 24
                Repeater {
                    model: 4
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        Skeleton { Layout.preferredWidth: 112; Layout.preferredHeight: 14; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 96; Layout.preferredHeight: 14; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 64; Layout.preferredHeight: 14; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 80; Layout.preferredHeight: 14; radius: 4 }
                    }
                }
                Item { Layout.fillHeight: true }
            }

            // Empty: a direct way to start
            ColumnLayout {
                visible: !dashboardVM.loading && dashboardVM.interviewCount === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                Item { Layout.fillHeight: true }
                Icon {
                    Layout.alignment: Qt.AlignHCenter
                    name: "history"
                    size: 32
                    color: Theme.zinc600
                }
                AppText {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("No interviews yet")
                    font.pixelSize: Typography.base
                    font.weight: Typography.semibold
                }
                AppText {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Finished interviews and their scores will appear here.")
                    color: Theme.zinc400
                }
                TactileButton {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 6
                    text: qsTr("Start your first interview")
                    iconLeft: "circle-plus"
                    onClicked: navVM.navigateToSetup()
                }
                Item { Layout.fillHeight: true }
            }

            // Column headers
            Rectangle {
                visible: !dashboardVM.loading && dashboardVM.interviewCount > 0
                Layout.fillWidth: true
                implicitHeight: 40
                color: Theme.alpha(Theme.zinc950, 0.6)

                Row {
                    x: 24
                    height: parent.height
                    Repeater {
                        model: [qsTr("Date"), qsTr("Track"), qsTr("Level"), qsTr("Status"), qsTr("Score"), ""]
                        AppText {
                            required property string modelData
                            required property int index
                            width: page.columnWidth(index, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: modelData
                            color: Theme.zinc400
                            font.pixelSize: Typography.xs
                            font.weight: Typography.semibold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: Typography.tracking(Typography.xs, Typography.trackingWider)
                        }
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.borderGlass }
            }

            // Rows: the whole row opens the report or resumes the session.
            ListView {
                id: rows
                visible: !dashboardVM.loading && dashboardVM.interviewCount > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: dashboardVM.interviews
                ScrollBar.vertical: ThemedScrollBar {}

                delegate: AbstractButton {
                    id: row
                    required property int index
                    required property string interviewId
                    required property string dateText
                    required property string direction
                    required property string level
                    required property bool isCompleted
                    required property bool hasScore
                    required property int score

                    width: rows.width
                    height: page.rowHeight
                    hoverEnabled: true
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    onClicked: {
                        if (row.isCompleted) {
                            navVM.navigateToReport(row.interviewId)
                        } else {
                            chatVM.openSession(row.interviewId)
                            navVM.navigateToChat(row.interviewId)
                        }
                    }

                    background: Rectangle {
                        color: row.hovered ? Theme.alpha(Theme.zinc800, 0.35) : "transparent"
                        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                        Rectangle {
                            visible: row.index > 0
                            width: parent.width
                            height: 1
                            color: Theme.borderFaint
                        }
                    }

                    contentItem: Row {
                        leftPadding: 24

                        MonoText {
                            width: page.columnWidth(0, table.width)
                            height: row.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.dateText
                            elide: Text.ElideRight
                        }
                        AppText {
                            width: page.columnWidth(1, table.width)
                            height: row.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.direction === "qa" || row.direction === "pm" ? row.direction.toUpperCase() : row.direction
                            font.weight: Typography.medium
                            font.capitalization: row.direction === "qa" || row.direction === "pm" ? Font.MixedCase : Font.Capitalize
                            elide: Text.ElideRight
                        }
                        AppText {
                            width: page.columnWidth(2, table.width)
                            height: row.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.level
                            color: Theme.zinc300
                            font.capitalization: Font.Capitalize
                            elide: Text.ElideRight
                        }
                        Item {
                            width: page.columnWidth(3, table.width)
                            height: row.height
                            Badge {
                                anchors.verticalCenter: parent.verticalCenter
                                tone: row.isCompleted ? "emerald" : "blue"
                                text: row.isCompleted ? qsTr("Completed") : qsTr("In progress")
                            }
                        }
                        MonoText {
                            width: page.columnWidth(4, table.width)
                            height: row.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.hasScore ? row.score + "%" : "—"
                            color: row.hasScore ? Theme.scoreColor(row.score) : Theme.textMuted
                            font.pixelSize: Typography.sm
                            font.weight: Typography.bold
                        }
                        Item {
                            width: page.columnWidth(5, table.width)
                            height: row.height
                            Row {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6
                                opacity: row.hovered ? 1 : 0.7
                                AppText {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: row.isCompleted ? qsTr("Open report") : qsTr("Resume")
                                    color: Theme.blue400
                                    font.pixelSize: Typography.xs
                                    font.weight: Typography.semibold
                                }
                                Icon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    name: "chevron-right"
                                    size: 14
                                    color: Theme.blue400
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
