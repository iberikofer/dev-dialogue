pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Interview report: the score and the verdict on the left, the evidence
// (per-question breakdown and study links) on the right.
AppPage {
    id: page

    active: StackLayout.isCurrentItem
    onActiveChanged: if (active) reportVM.load(navVM.activeInterviewId)
    Connections {
        target: backendVM
        function onStatusChanged() {
            if (backendVM.isOffline && reportVM.loading)
                reportVM.load(navVM.activeInterviewId)
        }
    }

    readonly property bool ready: !reportVM.loading && reportVM.hasReport

    title: qsTr("Interview report")
    subtitle: ready ? qsTr("How you did and what to study next.") : qsTr("Preparing the report…")
    badgeText: ready && reportVM.isDemoData ? qsTr("Demo data") : ""
    scrollable: true

    actions: [
        TactileButton {
            variant: "secondary"
            text: qsTr("Back to Home")
            iconLeft: "layout-dashboard"
            implicitHeight: 40
            onClicked: navVM.navigateToDashboard()
        },
        TactileButton {
            text: qsTr("New interview")
            iconLeft: "rotate-ccw"
            implicitHeight: 40
            glow: true
            onClicked: navVM.navigateToSetup()
        }
    ]

    // ------------------------------------------------------------ Loading
    Item {
        visible: !page.ready
        Layout.fillWidth: true
        implicitHeight: 360
        Spinner {
            anchors.centerIn: parent
            size: 28
            color: Theme.blue400
        }
    }

    RowLayout {
        visible: page.ready
        Layout.fillWidth: true
        spacing: 20

        // ------------------------------------------------- Left: verdict
        ColumnLayout {
            Layout.preferredWidth: 360
            Layout.maximumWidth: 360
            Layout.alignment: Qt.AlignTop
            spacing: 20

            DarkCard {
                Layout.fillWidth: true
                padding: 28
                fillColor: Theme.alpha(Theme.zinc900, 0.8)

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 18

                    ScoreGauge {
                        id: gauge
                        Layout.alignment: Qt.AlignHCenter
                        score: reportVM.score
                        suffix: qsTr("/ 100 points")
                        Connections {
                            target: page
                            function onReadyChanged() { if (page.ready) gauge.restart() }
                        }
                    }
                    AppText {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: reportVM.summary
                        color: Theme.zinc300
                        lineHeight: Typography.leadingRelaxed
                        wrapMode: Text.WordWrap
                    }
                }
            }

            component VerdictCard: DarkCard {
                id: verdict
                property string heading
                property string iconName
                property color tone
                property color toneText
                property var items: []

                Layout.fillWidth: true
                padding: 22
                fillColor: Qt.tint(Theme.zinc950, Theme.alpha(tone, 0.06))
                strokeColor: Theme.alpha(tone, 0.22)

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 12

                    RowLayout {
                        spacing: 8
                        Icon { name: verdict.iconName; size: 18; color: verdict.toneText }
                        AppText {
                            text: verdict.heading
                            color: verdict.toneText
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
                                Layout.topMargin: 7
                                implicitWidth: 5
                                implicitHeight: 5
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
                heading: qsTr("Strengths")
                iconName: "circle-check"
                tone: Theme.emerald500
                toneText: Theme.emerald400
                items: reportVM.strengths
            }
            VerdictCard {
                heading: qsTr("Areas to improve")
                iconName: "triangle-alert"
                tone: Theme.amber500
                toneText: Theme.amber400
                items: reportVM.weaknesses
            }
        }

        // ----------------------------------------------- Right: evidence
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 20

            DarkCard {
                visible: reportVM.questionScores.length > 0
                Layout.fillWidth: true
                padding: 22

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 10

                    AppText {
                        Layout.bottomMargin: 4
                        text: qsTr("Question breakdown")
                        font.pixelSize: Typography.base
                        font.weight: Typography.bold
                    }

                    Repeater {
                        model: reportVM.questionScores
                        Rectangle {
                            id: question
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: questionColumn.implicitHeight + 28
                            radius: Theme.radiusMd
                            color: Theme.alpha(Theme.zinc950, 0.6)
                            border.width: 1
                            border.color: Theme.borderGlass

                            ColumnLayout {
                                id: questionColumn
                                x: 16
                                y: 14
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
                                    MonoText {
                                        text: question.modelData.score
                                        color: Theme.scoreColor(question.modelData.score)
                                        font.pixelSize: Typography.base
                                        font.weight: Typography.bold
                                    }
                                }
                                // Score bar
                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: 4
                                    radius: 2
                                    color: Theme.zinc800
                                    Rectangle {
                                        width: parent.width * Math.max(0, Math.min(100, question.modelData.score)) / 100
                                        height: parent.height
                                        radius: 2
                                        color: Theme.scoreColor(question.modelData.score)
                                    }
                                }
                                AppText {
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

            DarkCard {
                visible: reportVM.recommendations.length > 0
                Layout.fillWidth: true
                padding: 22

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 10

                    RowLayout {
                        Layout.bottomMargin: 4
                        spacing: 8
                        Icon { name: "book-open"; size: 18; color: Theme.blue400 }
                        AppText {
                            text: qsTr("What to study next")
                            font.pixelSize: Typography.base
                            font.weight: Typography.bold
                        }
                    }

                    Repeater {
                        model: reportVM.recommendations
                        AbstractButton {
                            id: resource
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 56
                            hoverEnabled: true
                            onClicked: Qt.openUrlExternally(resource.modelData.resource)
                            HoverHandler { cursorShape: Qt.PointingHandCursor }
                            ToolTip.visible: hovered
                            ToolTip.delay: 600
                            ToolTip.text: qsTr("Open in browser")

                            background: Rectangle {
                                radius: Theme.radiusMd
                                color: resource.hovered ? Theme.zinc900 : Theme.alpha(Theme.zinc950, 0.6)
                                border.width: 1
                                border.color: resource.hovered ? Theme.alpha(Theme.blue500, 0.35) : Theme.borderGlass
                            }
                            contentItem: RowLayout {
                                spacing: 10
                                Item { implicitWidth: 6 }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    AppText {
                                        Layout.fillWidth: true
                                        text: resource.modelData.topic
                                        color: resource.hovered ? Theme.blue300 : Theme.zinc200
                                        font.weight: Typography.semibold
                                        elide: Text.ElideRight
                                    }
                                    AppText {
                                        Layout.fillWidth: true
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
                                Item { implicitWidth: 6 }
                            }
                        }
                    }
                }
            }
        }
    }
}
