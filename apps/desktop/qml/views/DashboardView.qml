pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Candidate dashboard (web DashboardView.tsx): welcome header, three KPI
// tiles and the interview history table.
PageScroll {
    id: page

    active: StackLayout.isCurrentItem
    maxContentWidth: Theme.contentWidthDefault
    onActiveChanged: if (active) dashboardVM.refresh()

    readonly property bool wide: width >= 720
    readonly property int rowHeight: 53
    readonly property int maxVisibleRows: 8
    // Column widths as fractions of the table width.
    readonly property var columns: [0.22, 0.18, 0.15, 0.17, 0.12, 0.16]

    function columnWidth(index, tableWidth) {
        return Math.floor((tableWidth - 48) * columns[index])
    }

    // ------------------------------------------------------------ Welcome
    RowLayout {
        Layout.fillWidth: true
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            AppText {
                Layout.fillWidth: true
                text: qsTr("Welcome, %1").arg(authVM.isAuthenticated && settingsVM.displayName !== ""
                                              ? settingsVM.displayName : qsTr("Candidate")) + " 👋"
                font.pixelSize: page.wide ? Typography.xl3 : Typography.xl2
                font.weight: Typography.bold
                font.letterSpacing: Typography.tracking((page.wide ? Typography.xl3 : Typography.xl2), Typography.trackingTight)
                elide: Text.ElideRight
            }
            AppText {
                Layout.fillWidth: true
                text: qsTr("Your personal technical interview preparation command center.")
                color: Theme.zinc400
                wrapMode: Text.WordWrap
            }
        }

        TactileButton {
            text: qsTr("Start New Interview")
            iconLeft: "circle-plus"
            implicitHeight: 42
            glow: true
            onClicked: navVM.navigateToSetup()
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 24
        Layout.bottomMargin: 32
        implicitHeight: 1
        color: Theme.borderGlass
    }

    // ---------------------------------------------------------- KPI tiles
    GridLayout {
        id: kpiGrid
        Layout.fillWidth: true
        Layout.bottomMargin: 32
        columns: page.wide ? 3 : 1
        columnSpacing: 16
        rowSpacing: 16

        readonly property real cellWidth: (width - columnSpacing * (columns - 1)) / columns

        KpiTile {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: kpiGrid.cellWidth
            caption: qsTr("Interviews Completed")
            description: qsTr("Total evaluated sessions")
            iconName: "history"
            iconColor: Theme.blue400
            value: dashboardVM.totalInterviews
            loading: dashboardVM.loading
            skeletonWidth: 80
        }
        KpiTile {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: kpiGrid.cellWidth
            caption: qsTr("Average Score")
            description: qsTr("Assessed by AI evaluation models")
            iconName: "trending-up"
            iconColor: Theme.emerald400
            valueColor: Theme.emerald400
            value: dashboardVM.averageScoreText !== "" ? dashboardVM.averageScoreText + "%" : "—"
            loading: dashboardVM.loading
            skeletonWidth: 96
        }
        KpiTile {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: kpiGrid.cellWidth
            caption: qsTr("Daily Limit")
            description: qsTr("Remaining sessions today")
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

    // ------------------------------------------------------ History table
    Rectangle {
        id: table
        Layout.fillWidth: true
        implicitHeight: tableColumn.implicitHeight
        radius: Theme.radiusLg
        color: Theme.surfaceCard
        border.width: 1
        border.color: Theme.borderGlass

        ColumnLayout {
            id: tableColumn
            width: parent.width
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.preferredHeight: 56

                AppText {
                    Layout.fillWidth: true
                    text: qsTr("Interview History")
                    font.pixelSize: Typography.base
                    font.weight: Typography.bold
                }
                Skeleton {
                    visible: dashboardVM.loading
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 12
                    radius: 4
                }
                MonoText {
                    visible: !dashboardVM.loading
                    text: qsTr("Total: %1").arg(dashboardVM.interviewCount)
                }
            }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.borderGlass }

            // Loading skeleton rows
            ColumnLayout {
                visible: dashboardVM.loading
                Layout.fillWidth: true
                Layout.margins: 24
                spacing: 16

                Repeater {
                    model: 3
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        Layout.bottomMargin: 8
                        spacing: 16
                        Skeleton { Layout.preferredWidth: 112; Layout.preferredHeight: 16; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 96; Layout.preferredHeight: 16; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 64; Layout.preferredHeight: 16; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 56; Layout.preferredHeight: 16; radius: 4 }
                        Item { Layout.fillWidth: true }
                        Skeleton { Layout.preferredWidth: 80; Layout.preferredHeight: 16; radius: 4 }
                    }
                }
            }

            // Empty state
            AppText {
                visible: !dashboardVM.loading && dashboardVM.interviewCount === 0
                Layout.fillWidth: true
                Layout.margins: 32
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No completed interviews found yet. Click \"Start New Interview\" to begin your practice!")
                color: Theme.zinc400
                wrapMode: Text.WordWrap
            }

            // Column headers
            Rectangle {
                visible: !dashboardVM.loading && dashboardVM.interviewCount > 0
                Layout.fillWidth: true
                implicitHeight: 41
                color: Theme.alpha(Theme.zinc950, 0.6)

                Row {
                    x: 24
                    height: parent.height

                    Repeater {
                        model: [qsTr("Date"), qsTr("Track"), qsTr("Level"), qsTr("Status"), qsTr("Score"), qsTr("Report")]
                        AppText {
                            required property string modelData
                            required property int index
                            width: page.columnWidth(index, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: index === 5 ? Text.AlignRight : Text.AlignLeft
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

            // Rows. The list is virtualised and scrolls on its own once the
            // history is longer than a screenful.
            ListView {
                id: rows
                visible: !dashboardVM.loading && dashboardVM.interviewCount > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(count, page.maxVisibleRows) * page.rowHeight
                Layout.bottomMargin: 1
                clip: true
                interactive: count > page.maxVisibleRows
                boundsBehavior: Flickable.StopAtBounds
                model: dashboardVM.interviews
                ScrollBar.vertical: ThemedScrollBar {}

                delegate: Rectangle {
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
                    color: rowHover.hovered ? Theme.alpha(Theme.zinc800, 0.3) : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                    HoverHandler { id: rowHover }

                    Rectangle {
                        visible: row.index > 0
                        width: parent.width
                        height: 1
                        color: Theme.borderFaint
                    }

                    Row {
                        x: 24
                        height: parent.height

                        MonoText {
                            width: page.columnWidth(0, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.dateText
                            elide: Text.ElideRight
                        }
                        AppText {
                            width: page.columnWidth(1, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.direction
                            font.weight: Typography.medium
                            font.capitalization: Font.Capitalize
                            elide: Text.ElideRight
                        }
                        AppText {
                            width: page.columnWidth(2, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.level
                            color: Theme.zinc300
                            font.pixelSize: Typography.xs
                            font.capitalization: Font.Capitalize
                            elide: Text.ElideRight
                        }
                        Item {
                            width: page.columnWidth(3, table.width)
                            height: parent.height
                            Badge {
                                anchors.verticalCenter: parent.verticalCenter
                                tone: row.isCompleted ? "emerald" : "blue"
                                text: row.isCompleted ? qsTr("Completed") : qsTr("Active")
                            }
                        }
                        MonoText {
                            width: page.columnWidth(4, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.hasScore ? row.score + "%" : "—"
                            color: Theme.zinc100
                            font.pixelSize: Typography.sm
                            font.weight: Typography.bold
                        }
                        Item {
                            width: page.columnWidth(5, table.width)
                            height: parent.height

                            Row {
                                id: action
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 4
                                scale: actionTap.pressed ? 0.97 : 1.0

                                AppText {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: row.isCompleted ? qsTr("View Report") : qsTr("Resume")
                                    color: actionHover.hovered ? Theme.blue300 : Theme.blue400
                                    font.pixelSize: Typography.xs
                                    font.weight: Typography.semibold
                                }
                                Icon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    name: row.isCompleted ? "external-link" : "circle-play"
                                    size: 12
                                    color: actionHover.hovered ? Theme.blue300 : Theme.blue400
                                }
                                HoverHandler { id: actionHover; cursorShape: Qt.PointingHandCursor }
                                TapHandler {
                                    id: actionTap
                                    onTapped: {
                                        if (row.isCompleted) {
                                            navVM.navigateToReport(row.interviewId)
                                        } else {
                                            chatVM.openSession(row.interviewId)
                                            navVM.navigateToChat(row.interviewId)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
