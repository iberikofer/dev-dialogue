pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Performance scorecard (web PerformanceReport.tsx): score dial with the
// summary, strengths and growth areas, per-question breakdown and the
// recommended study links.
PageScroll {
    id: page

    active: StackLayout.isCurrentItem
    maxContentWidth: Theme.contentWidthNarrow
    onActiveChanged: if (active) reportVM.load(navVM.activeInterviewId)

    readonly property bool ready: !reportVM.loading && reportVM.hasReport
    readonly property bool wide: width >= 768

    // ------------------------------------------------------------ Loading
    Item {
        visible: !page.ready
        Layout.fillWidth: true
        implicitHeight: 384

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16
            Spinner {
                Layout.alignment: Qt.AlignHCenter
                size: 24
                color: Theme.blue400
            }
            AppText {
                text: qsTr("Generating and loading report...")
                color: Theme.zinc400
            }
        }
    }

    // --------------------------------------------------------- Score card
    Rectangle {
        visible: page.ready
        Layout.fillWidth: true
        implicitHeight: hero.implicitHeight + 64
        radius: Theme.radiusXl
        border.width: 1
        border.color: Theme.borderGlass
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.zinc900 }
            GradientStop { position: 1.0; color: Theme.zinc950 }
        }

        ColumnLayout {
            id: hero
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 32
            spacing: 0

            Badge {
                Layout.alignment: Qt.AlignHCenter
                pill: true
                iconName: "trophy"
                text: qsTr("Technical Interview Final Evaluation")
                fontWeight: Typography.semibold
                horizontalPadding: 12
                verticalPadding: 5
            }

            ScoreGauge {
                id: gauge
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 16
                score: reportVM.score
                suffix: qsTr("/ 100 points")

                // Replay the dial each time a report is shown.
                Connections {
                    target: page
                    function onReadyChanged() { if (page.ready) gauge.restart() }
                }
            }

            AppText {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                Layout.fillWidth: true
                Layout.maximumWidth: 672
                horizontalAlignment: Text.AlignHCenter
                text: reportVM.summary
                color: Theme.zinc300
                font.pixelSize: Typography.base
                lineHeight: Typography.leadingRelaxed
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 32
                spacing: 16

                TactileButton {
                    text: qsTr("Retake Interview")
                    iconLeft: "rotate-ccw"
                    implicitHeight: 42
                    glow: true
                    onClicked: navVM.navigateToSetup()
                }
                TactileButton {
                    variant: "secondary"
                    text: qsTr("Back to Dashboard")
                    iconLeft: "layout-dashboard"
                    implicitHeight: 42
                    onClicked: navVM.navigateToDashboard()
                }
            }
        }
    }

    // -------------------------------------------- Strengths / growth areas
    GridLayout {
        id: verdictGrid
        visible: page.ready
        Layout.fillWidth: true
        Layout.topMargin: 32
        columns: page.wide ? 2 : 1
        columnSpacing: 24
        rowSpacing: 24

        readonly property real cellWidth: (width - columnSpacing * (columns - 1)) / columns

        component VerdictCard: DarkCard {
            id: verdict
            property string title
            property string iconName
            property color tone
            property color toneText
            property color toneDeep
            property var items: []
            property real cellWidth: 0

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: cellWidth
            interactive: true
            padding: 24
            fillColor: Qt.tint(Theme.zinc950, Theme.alpha(toneDeep, 0.1))
            strokeColor: Theme.alpha(tone, 0.2)

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 12

                RowLayout {
                    Layout.bottomMargin: 4
                    spacing: 8
                    Icon { name: verdict.iconName; size: 20; color: verdict.toneText }
                    AppText {
                        text: verdict.title
                        color: verdict.toneText
                        font.pixelSize: Typography.base
                        font.weight: Typography.bold
                    }
                }
                Repeater {
                    model: verdict.items
                    RowLayout {
                        id: point
                        required property string modelData
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 8
                            implicitWidth: 6
                            implicitHeight: 6
                            radius: 3
                            color: verdict.toneText
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: point.modelData
                            color: Theme.zinc300
                            lineHeight: Typography.leadingSnug
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        VerdictCard {
            title: qsTr("Key Strengths")
            iconName: "circle-check"
            tone: Theme.emerald500
            toneText: Theme.emerald400
            toneDeep: Theme.emerald950
            items: reportVM.strengths
            cellWidth: verdictGrid.cellWidth
        }
        VerdictCard {
            title: qsTr("Areas for Growth (Weaknesses)")
            iconName: "triangle-alert"
            tone: Theme.amber500
            toneText: Theme.amber400
            toneDeep: Theme.amber950
            items: reportVM.weaknesses
            cellWidth: verdictGrid.cellWidth
        }
    }

    // ------------------------------------------------- Question breakdown
    DarkCard {
        visible: page.ready && reportVM.questionScores.length > 0
        Layout.fillWidth: true
        Layout.topMargin: 32
        padding: 24

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 12

            AppText {
                Layout.bottomMargin: 4
                text: qsTr("Detailed Question Breakdown")
                font.pixelSize: Typography.lg
                font.weight: Typography.bold
            }

            Repeater {
                model: reportVM.questionScores

                // Click a question to fold or unfold the evaluator comment.
                Rectangle {
                    id: question
                    required property var modelData
                    property bool expanded: true

                    Layout.fillWidth: true
                    implicitHeight: questionColumn.implicitHeight + 32
                    radius: Theme.radiusMd
                    color: Theme.alpha(Theme.zinc950, 0.6)
                    border.width: 1
                    border.color: questionHover.hovered ? Qt.rgba(1, 1, 1, 0.14) : Theme.borderGlass
                    clip: true
                    Behavior on implicitHeight { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                    Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }

                    HoverHandler { id: questionHover; cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: question.expanded = !question.expanded }

                    ColumnLayout {
                        id: questionColumn
                        x: 16
                        y: 16
                        width: parent.width - 32
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            AppText {
                                Layout.fillWidth: true
                                text: question.modelData.question
                                font.weight: Typography.semibold
                                wrapMode: Text.WordWrap
                            }
                            Badge {
                                mono: true
                                text: question.modelData.score + " / 100"
                                fontWeight: Typography.bold
                                radius: Theme.radiusSm
                                horizontalPadding: 10
                                verticalPadding: 4
                            }
                            Icon {
                                name: "chevron-down"
                                size: 14
                                color: Theme.textMuted
                                rotation: question.expanded ? 180 : 0
                                Behavior on rotation { NumberAnimation { duration: 200 } }
                            }
                        }
                        AppText {
                            visible: question.expanded
                            Layout.fillWidth: true
                            text: question.modelData.comment
                            color: Theme.zinc400
                            font.pixelSize: Typography.xs
                            lineHeight: Typography.leadingRelaxed
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------ Recommended reading
    DarkCard {
        visible: page.ready && reportVM.recommendations.length > 0
        Layout.fillWidth: true
        Layout.topMargin: 32
        padding: 24

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 16

            RowLayout {
                spacing: 8
                Icon { name: "book-open"; size: 20; color: Theme.blue400 }
                AppText {
                    text: qsTr("Recommended Preparation Resources")
                    font.pixelSize: Typography.lg
                    font.weight: Typography.bold
                }
            }

            GridLayout {
                id: resourceGrid
                Layout.fillWidth: true
                columns: page.width >= 640 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12

                Repeater {
                    model: reportVM.recommendations

                    DarkCard {
                        id: resource
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: (resourceGrid.width - resourceGrid.columnSpacing * (resourceGrid.columns - 1)) / resourceGrid.columns
                        interactive: true
                        padding: 16
                        cardRadius: Theme.radiusMd
                        fillColor: Theme.alpha(Theme.zinc950, 0.8)
                        hoverFillColor: Theme.zinc900
                        hoverStrokeColor: Theme.alpha(Theme.blue500, 0.3)

                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: Qt.openUrlExternally(resource.modelData.resource) }

                        RowLayout {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                AppText {
                                    Layout.fillWidth: true
                                    text: resource.modelData.topic
                                    color: resource.hovered ? Theme.blue400 : Theme.zinc200
                                    font.weight: Typography.semibold
                                    elide: Text.ElideRight
                                    Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                                }
                                AppText {
                                    Layout.fillWidth: true
                                    Layout.maximumWidth: 280
                                    text: resource.modelData.resource
                                    color: Theme.textMuted
                                    font.pixelSize: Typography.xs
                                    elide: Text.ElideRight
                                }
                            }
                            Icon {
                                name: "external-link"
                                size: 16
                                color: resource.hovered ? Theme.blue400 : Theme.textMuted
                            }
                        }
                    }
                }
            }
        }
    }
}
