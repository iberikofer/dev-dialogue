pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Interview configuration (web InterviewSetup.tsx): career track, seniority
// level and interview language, then the launch button.
PageScroll {
    id: page

    active: StackLayout.isCurrentItem
    maxContentWidth: Theme.contentWidthNarrow
    topPadding: 48
    bottomPadding: 48

    readonly property int trackColumns: width >= 1024 ? 3 : (width >= 640 ? 2 : 1)

    readonly property var directions: [
        { id: "frontend", icon: "code-xml", title: qsTr("Frontend Development"),
          desc: qsTr("React, Next.js, TypeScript, Browser APIs, CSS Architecture & Web Vitals") },
        { id: "backend", icon: "server", title: qsTr("Backend Engineering"),
          desc: qsTr("ASP.NET Core, C#, Databases, REST/gRPC, Async, Systems Architecture") },
        { id: "devops", icon: "cloud-cog", title: qsTr("DevOps & Cloud"),
          desc: qsTr("Docker, CI/CD pipelines, Kubernetes, Monitoring, Cloud Infrastructure") },
        { id: "qa", icon: "circle-check", title: qsTr("Quality Assurance"),
          desc: qsTr("Test Design, Automation, Smoke/Regression strategies, API Testing") },
        { id: "pm", icon: "briefcase", title: qsTr("Project Management"),
          desc: qsTr("Agile/Scrum, Backlog prioritization, Risk mitigation, Team dynamics") }
    ]
    readonly property var levels: [
        { id: "trainee", title: qsTr("Trainee / Intern"), subtitle: qsTr("Core principles, language syntax, algorithms") },
        { id: "junior", title: qsTr("Junior Engineer"), subtitle: qsTr("Hands-on development, web standards, and APIs") },
        { id: "middle", title: qsTr("Middle Engineer"), subtitle: qsTr("System architecture, performance optimization, internals") }
    ]
    readonly property var languages: [
        { id: "uk", flag: "🇺🇦", title: "Українська", subtitle: qsTr("Ukrainian (Native)") },
        { id: "en", flag: "🇬🇧", title: "English", subtitle: qsTr("English (International)") }
    ]

    component StepLabel: MonoText {
        Layout.fillWidth: true
        Layout.bottomMargin: 12
        caps: true
        font.weight: Typography.semibold
    }

    // ------------------------------------------------------------- Header
    Badge {
        Layout.alignment: Qt.AlignHCenter
        pill: true
        iconName: "sparkles"
        text: qsTr("Technical Session Configuration")
        foreground: Theme.blue300
        horizontalPadding: 14
        verticalPadding: 5
        strokeOpacity: 0.3
    }
    AppText {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 12
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Interview Setup")
        font.pixelSize: Typography.xl4
        font.weight: Typography.extrabold
        font.letterSpacing: Typography.tracking(Typography.xl4, -0.03)
        wrapMode: Text.WordWrap
    }
    AppText {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 8
        Layout.fillWidth: true
        Layout.maximumWidth: 576
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Choose your engineering track, difficulty level, and conversational language for the AI interviewer.")
        color: Theme.zinc400
        font.pixelSize: Typography.base
        lineHeight: Typography.leadingNormal
        wrapMode: Text.WordWrap
    }

    AlertBox {
        visible: setupVM.errorMessage !== ""
        Layout.fillWidth: true
        Layout.topMargin: 24
        text: setupVM.errorMessage
    }

    // ------------------------------------------------------ Step 1: track
    StepLabel {
        Layout.topMargin: 40
        text: qsTr("1. Engineering Track")
    }
    GridLayout {
        id: trackGrid
        Layout.fillWidth: true
        columns: page.trackColumns
        columnSpacing: 12
        rowSpacing: 12

        Repeater {
            model: page.directions

            SelectCard {
                id: trackCard
                required property var modelData

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: (trackGrid.width - trackGrid.columnSpacing * (trackGrid.columns - 1)) / trackGrid.columns
                Layout.fillHeight: true
                selected: setupVM.direction === modelData.id
                onClicked: setupVM.direction = modelData.id

                contentItem: ColumnLayout {
                    spacing: 0

                    Rectangle {
                        Layout.bottomMargin: 12
                        implicitWidth: 40
                        implicitHeight: 40
                        radius: Theme.radiusMd
                        color: trackCard.selected ? Theme.primary : Theme.zinc800
                        border.width: trackCard.selected ? 0 : 1
                        border.color: Theme.borderFaint
                        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                        Icon {
                            anchors.centerIn: parent
                            name: trackCard.modelData.icon
                            size: 20
                            color: trackCard.selected ? Theme.textOnAccent : Theme.zinc400
                        }
                    }
                    AppText {
                        Layout.fillWidth: true
                        text: trackCard.modelData.title
                        font.weight: Typography.semibold
                        elide: Text.ElideRight
                    }
                    AppText {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        text: trackCard.modelData.desc
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                        lineHeight: Typography.leadingRelaxed
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    // ------------------------------------------------------ Step 2: level
    StepLabel {
        Layout.topMargin: 36
        text: qsTr("2. Seniority Level")
    }
    GridLayout {
        id: levelGrid
        Layout.fillWidth: true
        columns: page.width >= 640 ? 3 : 1
        columnSpacing: 12
        rowSpacing: 12

        Repeater {
            model: page.levels

            SelectCard {
                id: levelCard
                required property var modelData

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: (levelGrid.width - levelGrid.columnSpacing * (levelGrid.columns - 1)) / levelGrid.columns
                Layout.fillHeight: true
                selected: setupVM.level === modelData.id
                onClicked: setupVM.level = modelData.id

                contentItem: ColumnLayout {
                    spacing: 4
                    AppText {
                        Layout.fillWidth: true
                        text: levelCard.modelData.title
                        font.weight: Typography.semibold
                        elide: Text.ElideRight
                    }
                    AppText {
                        Layout.fillWidth: true
                        text: levelCard.modelData.subtitle
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                        lineHeight: Typography.leadingNormal
                        wrapMode: Text.WordWrap
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    // --------------------------------------------------- Step 3: language
    StepLabel {
        Layout.topMargin: 36
        text: qsTr("3. Interview Language")
    }
    RowLayout {
        id: languageRow
        Layout.fillWidth: true
        Layout.maximumWidth: 448
        spacing: 12

        Repeater {
            model: page.languages

            SelectCard {
                id: languageCard
                required property var modelData

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: (languageRow.width - languageRow.spacing) / 2
                selected: setupVM.language === modelData.id
                onClicked: setupVM.language = modelData.id

                contentItem: RowLayout {
                    spacing: 12
                    AppText {
                        text: languageCard.modelData.flag
                        font.pixelSize: Typography.xl2
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        AppText {
                            Layout.fillWidth: true
                            text: languageCard.modelData.title
                            font.weight: Typography.semibold
                            elide: Text.ElideRight
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: languageCard.modelData.subtitle
                            color: Theme.zinc400
                            font.pixelSize: Typography.tiny
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------- Launch
    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 60
        implicitHeight: 1
        color: Theme.borderGlass
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 24
        spacing: 16

        MonoText {
            Layout.fillWidth: true
            text: qsTr("Session consists of 5–7 questions followed by an in-depth scorecard.")
            wrapMode: Text.WordWrap
        }
        TactileButton {
            text: setupVM.isBusy ? qsTr("Initializing AI...") : qsTr("Start Interview")
            iconRight: setupVM.isBusy ? "" : "arrow-right"
            enabled: !setupVM.isBusy
            implicitHeight: 50
            leftPadding: 24
            rightPadding: 24
            glow: true
            onClicked: setupVM.start()
        }
    }
}
