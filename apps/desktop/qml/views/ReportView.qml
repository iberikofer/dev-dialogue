pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Minimalistic, high-signal interview report:
// Hero verdict & pacing overview, side-by-side strengths/gaps,
// per-question timing + criteria breakdown, and targeted study links.
AppPage {
    id: page

    active: StackLayout.isCurrentItem
    onActiveChanged: if (active) reportVM.load(navVM.activeInterviewId)
    Connections {
        target: backendVM
        function onStatusChanged() {
            if (backendVM.isOffline && reportVM.loading)
                reportVM.load(navVM.activeInterviewId)
            else if (!backendVM.isOffline && page.active && page.failed)
                reportVM.load(navVM.activeInterviewId)
        }
    }

    readonly property bool ready: !reportVM.loading && reportVM.hasReport
    readonly property bool failed: !reportVM.loading && reportVM.errorMessage !== ""

    function trackName(direction) {
        switch (direction) {
        case "frontend": return "Frontend"
        case "backend": return "Backend"
        case "devops": return "DevOps"
        case "qa": return qsTr("Quality Assurance")
        case "pm": return qsTr("Project Management")
        default: return ""
        }
    }

    function formatSeconds(sec) {
        if (sec <= 0) return "—"
        if (sec < 60) return sec + "s"
        const m = Math.floor(sec / 60)
        const r = sec % 60
        return r === 0 ? (m + "m") : (m + "m " + r + "s")
    }

    function emotionLabel(code) {
        switch ((code || "").toLowerCase()) {
        case "smiling":
        case "smile":
        case "happy": return qsTr("Посмішка")
        case "confident": return qsTr("Впевненість")
        case "focused": return qsTr("Зосередженість")
        case "tense":
        case "nervous": return qsTr("Напруженість")
        case "distracted": return qsTr("Відволікання")
        case "absent": return qsTr("Відсутній у кадрі")
        default: return qsTr("Спокій")
        }
    }

    readonly property string context: {
        const parts = [trackName(reportVM.direction),
                       reportVM.level.charAt(0).toUpperCase() + reportVM.level.slice(1),
                       reportVM.dateText]
        return parts.filter(part => part !== "").join("  ·  ")
    }

    title: qsTr("Interview report")
    subtitle: ready ? (context !== "" ? context : qsTr("AI performance summary & pacing overview."))
                    : failed ? qsTr("The report is not available right now.")
                    : qsTr("Analyzing answers and preparing the report…")
    badgeText: ready && reportVM.isDemoData ? qsTr("Demo data") : ""
    scrollable: true

    actions: [
        TactileButton {
            visible: page.ready
            variant: "secondary"
            text: qsTr("Export PDF")
            iconLeft: "book-open"
            implicitHeight: 40
            onClicked: reportVM.exportPdf()
        },
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
        visible: !page.ready && !page.failed
        Layout.fillWidth: true
        implicitHeight: 360
        Spinner {
            anchors.centerIn: parent
            size: 28
            color: Theme.blue400
        }
    }

    // -------------------------------------------------------------- Error
    ColumnLayout {
        visible: page.failed
        Layout.fillWidth: true
        Layout.topMargin: 96
        spacing: 14

        Icon {
            Layout.alignment: Qt.AlignHCenter
            name: "circle-alert"
            size: 32
            color: Theme.amber400
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 460
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: reportVM.errorMessage
            color: Theme.zinc300
        }
        TactileButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6
            variant: "secondary"
            text: backendVM.isChecking ? qsTr("Checking...") : qsTr("Try again")
            iconLeft: "refresh-cw"
            iconSize: 14
            implicitHeight: 36
            enabled: !backendVM.isChecking
            onClicked: backendVM.isOffline ? backendVM.checkHealth() : reportVM.load(navVM.activeInterviewId)
        }
    }

    // -------------------------------------------------------------- Content
    ColumnLayout {
        visible: page.ready
        Layout.fillWidth: true
        Layout.maximumWidth: 1080
        spacing: 18

        // 1. Minimalistic Hero Overview Card
        DarkCard {
            Layout.fillWidth: true
            padding: 24
            fillColor: Theme.alpha(Theme.zinc900, 0.72)

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 28

                ScoreGauge {
                    id: gauge
                    Layout.alignment: Qt.AlignVCenter
                    score: reportVM.score
                    suffix: qsTr("/ 100")
                    Connections {
                        target: page
                        function onReadyChanged() { if (page.ready) gauge.restart() }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Repeater {
                            model: [
                                { label: qsTr("Questions"), value: String(reportVM.questionScores.length) },
                                { label: qsTr("Avg response"), value: page.formatSeconds(reportVM.averageResponseSeconds) },
                                { label: qsTr("Session time"), value: page.formatSeconds(reportVM.totalDurationSeconds) }
                            ]
                            Rectangle {
                                id: statPill
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 54
                                radius: Theme.radiusMd
                                color: Theme.alpha(Theme.zinc950, 0.65)
                                border.width: 1
                                border.color: Theme.borderGlass

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    anchors.topMargin: 8
                                    anchors.bottomMargin: 8
                                    spacing: 2

                                    MonoText {
                                        caps: true
                                        text: statPill.modelData.label
                                        color: Theme.zinc500
                                        font.pixelSize: Typography.tiny
                                    }
                                    MonoText {
                                        text: statPill.modelData.value
                                        color: Theme.zinc100
                                        font.pixelSize: Typography.base
                                        font.weight: Typography.bold
                                    }
                                }
                            }
                        }
                    }

                    AppText {
                        Layout.fillWidth: true
                        text: reportVM.summary
                        color: Theme.zinc200
                        lineHeight: Typography.leadingRelaxed
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        visible: reportVM.timingAnalysis !== ""
                        Layout.fillWidth: true
                        spacing: 8

                        Icon {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 2
                            name: "clock"
                            size: 14
                            color: Theme.blue400
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: reportVM.timingAnalysis
                            color: Theme.zinc400
                            font.pixelSize: Typography.xs
                            lineHeight: Typography.leadingNormal
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        // 2. Side-by-side Strengths & Gaps
        component VerdictCard: DarkCard {
            id: verdict
            property string heading
            property string iconName
            property color tone
            property color toneText
            property var items: []

            visible: items.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            padding: 20
            fillColor: Qt.tint(Theme.zinc950, Theme.alpha(tone, 0.05))
            strokeColor: Theme.alpha(tone, 0.22)

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 10

                RowLayout {
                    spacing: 8
                    Icon { name: verdict.iconName; size: 16; color: verdict.toneText }
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
                            font.pixelSize: Typography.sm
                            lineHeight: Typography.leadingSnug
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        RowLayout {
            visible: reportVM.strengths.length > 0 || reportVM.weaknesses.length > 0
            Layout.fillWidth: true
            spacing: 16

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

        // 2.5 Visual & Camera Presence Breakdown (30 FPS CV + lfm3-omni:2b)
        DarkCard {
            visible: reportVM.hasVisualAssessment
            Layout.fillWidth: true
            padding: 22

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Icon { name: "video"; size: 17; color: Theme.blue400 }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        AppText {
                            text: qsTr("Оцінка візуалу по камері")
                            font.pixelSize: Typography.base
                            font.weight: Typography.bold
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: qsTr("Локальний аналіз присутності, рухів рук, освітлення, вигляду, постави й емоцій (%1 · %2 знімків)")
                                .arg(reportVM.visualModel.length > 0 ? reportVM.visualModel : "gemma4:e4b + 30 FPS CV")
                                .arg(reportVM.visualSamples)
                            color: Theme.zinc400
                            font.pixelSize: Typography.xs
                            wrapMode: Text.WordWrap
                        }
                    }

                    Badge {
                        visible: reportVM.visualIncludedInScore
                        tone: "emerald"
                        mono: true
                        text: qsTr("Враховано в балі (15%)")
                        fontSize: Typography.tiny
                    }

                    Badge {
                        tone: reportVM.visualDominantEmotion === "absent" ? "rose" : "blue"
                        mono: true
                        text: page.emotionLabel(reportVM.visualDominantEmotion)
                        fontSize: Typography.tiny
                    }

                    Badge {
                        tone: reportVM.visualScore >= 75 ? "emerald" : (reportVM.visualScore >= 45 ? "amber" : "rose")
                        mono: true
                        text: reportVM.visualScore + "/100"
                        fontSize: Typography.sm
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 12
                    rowSpacing: 8

                    Repeater {
                        model: [
                            { label: qsTr("Наявність у кадрі"), score: reportVM.visualPresence },
                            { label: qsTr("Руки та жести"), score: reportVM.visualHands },
                            { label: qsTr("Освітлення"), score: reportVM.visualLighting },
                            { label: qsTr("Зовнішній вигляд"), score: reportVM.visualAppearance },
                            { label: qsTr("Постава і погляд"), score: reportVM.visualPosture },
                            { label: qsTr("Емоції та міміка"), score: reportVM.visualEmotionScore }
                        ]
                        Rectangle {
                            id: visMetric
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: visMetricCol.implicitHeight + 16
                            radius: Theme.radiusMd
                            color: Theme.zinc900
                            border.width: 1
                            border.color: Theme.borderFaint

                            ColumnLayout {
                                id: visMetricCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 10
                                spacing: 6

                                AppText {
                                    Layout.fillWidth: true
                                    text: visMetric.modelData.label
                                    color: Theme.zinc300
                                    font.pixelSize: Typography.xs
                                    elide: Text.ElideRight
                                }

                                MonoText {
                                    text: visMetric.modelData.score + "/100"
                                    color: Theme.scoreColor(visMetric.modelData.score)
                                    font.pixelSize: Typography.base
                                    font.weight: Typography.bold
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 4
                                    radius: 2
                                    color: Theme.zinc800
                                    Rectangle {
                                        width: parent.width * Math.max(0, Math.min(100, visMetric.modelData.score)) / 100
                                        height: parent.height
                                        radius: 2
                                        color: Theme.scoreColor(visMetric.modelData.score)
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    visible: reportVM.visualSummary.length > 0
                    Layout.fillWidth: true
                    spacing: 8
                    Icon { name: "shield-check"; size: 14; color: Theme.emerald400 }
                    AppText {
                        Layout.fillWidth: true
                        text: reportVM.visualSummary
                        color: Theme.zinc300
                        font.pixelSize: Typography.xs
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // 3. Per-question Breakdown
        DarkCard {
            visible: reportVM.questionScores.length > 0
            Layout.fillWidth: true
            padding: 22

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("Question breakdown")
                        font.pixelSize: Typography.base
                        font.weight: Typography.bold
                    }
                    MonoText {
                        text: qsTr("%1 evaluated").arg(reportVM.questionScores.length)
                        color: Theme.zinc500
                        font.pixelSize: Typography.xs
                    }
                }

                Repeater {
                    model: reportVM.questionScores
                    Rectangle {
                        id: question
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: questionColumn.implicitHeight + 28
                        radius: Theme.radiusMd
                        color: Theme.alpha(Theme.zinc950, 0.55)
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
                                spacing: 10

                                AppText {
                                    Layout.fillWidth: true
                                    text: (question.index + 1) + ". " + question.modelData.question
                                    font.weight: Typography.semibold
                                    wrapMode: Text.WordWrap
                                }

                                Rectangle {
                                    visible: question.modelData.responseSeconds > 0
                                    implicitWidth: timeRow.implicitWidth + 16
                                    implicitHeight: 24
                                    radius: 6
                                    color: Theme.zinc900
                                    border.width: 1
                                    border.color: Theme.borderSubtle

                                    RowLayout {
                                        id: timeRow
                                        anchors.centerIn: parent
                                        spacing: 4
                                        Icon { name: "clock"; size: 11; color: Theme.zinc400 }
                                        MonoText {
                                            text: page.formatSeconds(question.modelData.responseSeconds)
                                            color: Theme.zinc300
                                            font.pixelSize: Typography.tiny
                                        }
                                    }
                                }

                                MonoText {
                                    text: question.modelData.score + "/100"
                                    color: Theme.scoreColor(question.modelData.score)
                                    font.pixelSize: Typography.sm
                                    font.weight: Typography.bold
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 3
                                radius: 2
                                color: Theme.zinc800
                                Rectangle {
                                    width: parent.width * Math.max(0, Math.min(100, question.modelData.score)) / 100
                                    height: parent.height
                                    radius: 2
                                    color: Theme.scoreColor(question.modelData.score)
                                }
                            }

                            MonoText {
                                visible: question.modelData.hasCriteria === true
                                Layout.fillWidth: true
                                text: question.modelData.hasVisual === true
                                    ? qsTr("Accuracy %1  ·  Completeness %2  ·  Terminology %3  ·  Clarity %4  ·  Camera %5")
                                        .arg(question.modelData.accuracy)
                                        .arg(question.modelData.completeness)
                                        .arg(question.modelData.terminology)
                                        .arg(question.modelData.clarity)
                                        .arg(question.modelData.visual)
                                    : qsTr("Accuracy %1  ·  Completeness %2  ·  Terminology %3  ·  Clarity %4")
                                        .arg(question.modelData.accuracy)
                                        .arg(question.modelData.completeness)
                                        .arg(question.modelData.terminology)
                                        .arg(question.modelData.clarity)
                                color: Theme.blue300
                                font.pixelSize: Typography.tiny
                                wrapMode: Text.WordWrap
                            }

                            AppText {
                                visible: question.modelData.userAnswer !== undefined && question.modelData.userAnswer !== ""
                                Layout.fillWidth: true
                                text: qsTr("Your answer: “%1”").arg(question.modelData.userAnswer)
                                color: Theme.zinc500
                                font.pixelSize: Typography.xs
                                wrapMode: Text.WordWrap
                            }

                            AppText {
                                Layout.fillWidth: true
                                text: question.modelData.comment
                                color: Theme.zinc300
                                font.pixelSize: Typography.xs
                                lineHeight: Typography.leadingRelaxed
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }

        // 4. Recommended Study Resources
        DarkCard {
            visible: reportVM.recommendations.length > 0
            Layout.fillWidth: true
            padding: 20

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 8

                RowLayout {
                    Layout.bottomMargin: 4
                    spacing: 8
                    Icon { name: "book-open"; size: 16; color: Theme.blue400 }
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
                        implicitHeight: 50
                        hoverEnabled: true
                        onClicked: Qt.openUrlExternally(resource.modelData.resource)
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        AppToolTip {
                            visible: resource.hovered
                            text: qsTr("Open in browser")
                        }

                        background: Rectangle {
                            radius: Theme.radiusMd
                            color: resource.hovered ? Theme.zinc900 : Theme.alpha(Theme.zinc950, 0.5)
                            border.width: 1
                            border.color: resource.hovered ? Theme.alpha(Theme.blue500, 0.35) : Theme.borderGlass
                        }
                        contentItem: RowLayout {
                            spacing: 10
                            Item { implicitWidth: 6 }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                AppText {
                                    Layout.fillWidth: true
                                    text: resource.modelData.topic
                                    color: resource.hovered ? Theme.blue300 : Theme.zinc200
                                    font.pixelSize: Typography.sm
                                    font.weight: Typography.semibold
                                    elide: Text.ElideRight
                                }
                                AppText {
                                    Layout.fillWidth: true
                                    text: resource.modelData.resource
                                    color: Theme.textMuted
                                    font.pixelSize: Typography.tiny
                                    elide: Text.ElideRight
                                }
                            }
                            Icon {
                                name: "external-link"
                                size: 14
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
