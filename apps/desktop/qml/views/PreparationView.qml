pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Interview Preparation screen:
// Select track and level, enter free-form knowledge description,
// and receive tailored recommendations with authoritative study links.
AppPage {
    id: page

    active: StackLayout.isCurrentItem
    title: qsTr("Interview Preparation")
    subtitle: preparationVM.hasResult
        ? qsTr("Curated knowledge analysis & targeted preparation roadmap.")
        : qsTr("Describe what you know in your specialty, and we'll analyze what to learn next.")
    badgeText: qsTr("Demo Mode · Local Analysis")

    readonly property var directions: [
        { id: "frontend", icon: "code-xml", title: qsTr("Frontend"),
          desc: qsTr("React, Next.js, TypeScript, browser APIs, CSS architecture") },
        { id: "backend", icon: "server", title: qsTr("Backend"),
          desc: qsTr("ASP.NET Core, C#, databases, REST, async, system design") },
        { id: "devops", icon: "cloud-cog", title: qsTr("DevOps"),
          desc: qsTr("Docker, CI/CD, Kubernetes, monitoring, cloud") },
        { id: "qa", icon: "circle-check", title: qsTr("Quality Assurance"),
          desc: qsTr("Test design, automation, regression, API testing") },
        { id: "pm", icon: "briefcase", title: qsTr("Project Management"),
          desc: qsTr("Agile, backlog, risk management, team dynamics") }
    ]

    readonly property var levels: [
        { id: "trainee", title: "Trainee", subtitle: qsTr("Core principles and syntax") },
        { id: "junior", title: "Junior", subtitle: qsTr("Hands-on development and APIs") },
        { id: "middle", title: "Middle", subtitle: qsTr("Architecture and internals") }
    ]

    function lookup(list, id) {
        for (let i = 0; i < list.length; ++i) {
            if (list[i].id === id)
                return list[i]
        }
        return list[0]
    }

    actions: [
        TactileButton {
            visible: preparationVM.hasResult
            variant: "secondary"
            text: qsTr("Refine description")
            iconLeft: "rotate-ccw"
            implicitHeight: 40
            onClicked: preparationVM.refineDescription()
        },
        TactileButton {
            visible: preparationVM.hasResult
            text: qsTr("Practice this track")
            iconRight: "arrow-right"
            implicitHeight: 40
            glow: true
            onClicked: {
                setupVM.direction = preparationVM.direction
                setupVM.level = preparationVM.level
                navVM.navigateToSetup()
            }
        }
    ]

    component SectionLabel: MonoText {
        Layout.fillWidth: true
        caps: true
        font.weight: Typography.semibold
    }

    // -------------------------------------------------------------
    // STATE 1: Input & Configuration Form
    // -------------------------------------------------------------
    RowLayout {
        visible: !preparationVM.hasResult && !preparationVM.isAnalyzing
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 24

        // Choices Column
        Flickable {
            id: choicesFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: formColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ThemedScrollBar {}

            ColumnLayout {
                id: formColumn
                width: choicesFlickable.width - 12
                spacing: 16

                AlertBox {
                    visible: preparationVM.errorMessage !== ""
                    Layout.fillWidth: true
                    Layout.bottomMargin: 8
                    text: preparationVM.errorMessage
                }

                SectionLabel { text: qsTr("Select Track") }
                GridLayout {
                    id: trackGrid
                    Layout.fillWidth: true
                    columns: width > 760 ? 3 : 2
                    columnSpacing: 12
                    rowSpacing: 12

                    Repeater {
                        model: page.directions
                        SelectCard {
                            id: trackCard
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: (trackGrid.width - trackGrid.columnSpacing * (trackGrid.columns - 1)) / trackGrid.columns
                            selected: preparationVM.direction === modelData.id
                            onClicked: preparationVM.direction = modelData.id

                            contentItem: RowLayout {
                                spacing: 14
                                Rectangle {
                                    Layout.alignment: Qt.AlignTop
                                    implicitWidth: 40
                                    implicitHeight: 40
                                    radius: Theme.radiusMd
                                    color: trackCard.selected ? Theme.primary : Theme.zinc800
                                    Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                                    Icon {
                                        anchors.centerIn: parent
                                        name: trackCard.modelData.icon
                                        size: 20
                                        color: trackCard.selected ? Theme.textOnAccent : Theme.zinc400
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 4
                                    AppText {
                                        Layout.fillWidth: true
                                        text: trackCard.modelData.title
                                        font.weight: Typography.semibold
                                        elide: Text.ElideRight
                                    }
                                    AppText {
                                        Layout.fillWidth: true
                                        text: trackCard.modelData.desc
                                        color: Theme.zinc400
                                        font.pixelSize: Typography.xs
                                        lineHeight: Typography.leadingNormal
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }

                SectionLabel { Layout.topMargin: 12; text: qsTr("Experience Level") }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 540
                    spacing: 12

                    Repeater {
                        model: page.levels
                        SelectCard {
                            id: levelCard
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            selected: preparationVM.level === modelData.id
                            onClicked: preparationVM.level = modelData.id

                            contentItem: ColumnLayout {
                                spacing: 4
                                AppText {
                                    Layout.fillWidth: true
                                    text: levelCard.modelData.title
                                    font.weight: Typography.semibold
                                }
                                AppText {
                                    Layout.fillWidth: true
                                    text: levelCard.modelData.subtitle
                                    color: Theme.zinc400
                                    font.pixelSize: Typography.xs
                                    wrapMode: Text.WordWrap
                                }
                                Item { Layout.fillHeight: true }
                            }
                        }
                    }
                }

                SectionLabel { Layout.topMargin: 12; text: qsTr("Your Skills & Experience") }
                DarkCard {
                    Layout.fillWidth: true
                    padding: 16

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 8

                        DarkTextArea {
                            id: descInput
                            Layout.fillWidth: true
                            implicitHeight: 140
                            text: preparationVM.userDescription
                            onTextChanged: preparationVM.userDescription = text
                            placeholderText: qsTr("Describe your skills in free form. For example:\n\"I have been developing with C# and ASP.NET Core for 2 years, worked with PostgreSQL and Entity Framework Core. Built REST APIs and wrote unit tests. Never used Docker or Kubernetes, and haven't worked with microservices yet...\"")
                            invalid: !preparationVM.isValidInput && preparationVM.charCount > 0
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Icon {
                                name: preparationVM.isValidInput ? "circle-check" : "info"
                                size: 14
                                color: preparationVM.isValidInput ? Theme.emerald400
                                     : (preparationVM.charCount > preparationVM.maxCharCount ? Theme.rose400 : Theme.zinc500)
                            }
                            AppText {
                                Layout.fillWidth: true
                                text: preparationVM.validationMessage
                                color: preparationVM.isValidInput ? Theme.emerald400
                                     : (preparationVM.charCount > preparationVM.maxCharCount ? Theme.rose400 : Theme.zinc500)
                                font.pixelSize: Typography.tiny
                            }
                            MonoText {
                                text: preparationVM.charCount + " / " + preparationVM.maxCharCount
                                color: preparationVM.charCount > preparationVM.maxCharCount ? Theme.rose400 : Theme.zinc500
                                font.pixelSize: Typography.tiny
                            }
                        }
                    }
                }

                Item { implicitHeight: 16 }
            }
        }

        // Summary Sidebar
        DarkCard {
            Layout.preferredWidth: 320
            Layout.alignment: Qt.AlignTop
            padding: 24
            fillColor: Theme.alpha(Theme.zinc900, 0.7)

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 0

                MonoText {
                    caps: true
                    text: qsTr("Summary")
                    font.weight: Typography.semibold
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    AppText { Layout.fillWidth: true; text: qsTr("Track"); color: Theme.zinc400 }
                    AppText { text: page.lookup(page.directions, preparationVM.direction).title; font.weight: Typography.semibold }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    AppText { Layout.fillWidth: true; text: qsTr("Level"); color: Theme.zinc400 }
                    AppText { text: page.lookup(page.levels, preparationVM.level).title; font.weight: Typography.semibold }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 18
                    Layout.bottomMargin: 18
                    implicitHeight: 1
                    color: Theme.borderFaint
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Icon { Layout.alignment: Qt.AlignTop; name: "info"; size: 16; color: Theme.blue400 }
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("The analyzer checks for keywords and context. Anything not mentioned in your description will be suggested as study material.")
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                        lineHeight: Typography.leadingNormal
                        wrapMode: Text.WordWrap
                    }
                }

                Item { implicitHeight: 12 }

                Badge {
                    Layout.fillWidth: true
                    tone: "neutral"
                    text: qsTr("Local keyword analyzer · Zero network required")
                    fontSize: Typography.tiny
                }

                TactileButton {
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    text: preparationVM.isAnalyzing ? qsTr("Analyzing skills...") : qsTr("Generate study plan")
                    iconRight: preparationVM.isAnalyzing ? "" : "arrow-right"
                    busy: preparationVM.isAnalyzing
                    enabled: preparationVM.isValidInput && !preparationVM.isAnalyzing
                    implicitHeight: 46
                    glow: true
                    onClicked: preparationVM.analyze()
                }
            }
        }
    }

    // -------------------------------------------------------------
    // STATE 2: Loading Analysis Spinner
    // -------------------------------------------------------------
    ColumnLayout {
        visible: preparationVM.isAnalyzing
        Layout.fillWidth: true
        Layout.topMargin: 120
        spacing: 16

        Spinner {
            Layout.alignment: Qt.AlignHCenter
            size: 36
            color: Theme.blue400
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Analyzing your skills and mapping out a preparation roadmap…")
            color: Theme.zinc300
            font.pixelSize: Typography.sm
        }
    }

    // -------------------------------------------------------------
    // STATE 3: Results & Study Roadmap
    // -------------------------------------------------------------
    Flickable {
        id: resultsScroller
        visible: preparationVM.hasResult && !preparationVM.isAnalyzing
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: width
        contentHeight: resultsColumn.implicitHeight + 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ThemedScrollBar {}

        ColumnLayout {
            id: resultsColumn
            width: resultsScroller.width - 12
            spacing: 20

            // 1. Overview Banner
            DarkCard {
                Layout.fillWidth: true
                padding: 24

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Icon { name: "book-open"; size: 22; color: Theme.blue400 }
                        AppText {
                            Layout.fillWidth: true
                            text: qsTr("Roadmap Overview")
                            font.pixelSize: Typography.lg
                            font.weight: Typography.bold
                        }
                        Badge {
                            tone: preparationVM.coveredTopicsCount === preparationVM.totalTopicsCount ? "emerald" : "blue"
                            text: qsTr("%1 / %2 topics covered").arg(preparationVM.coveredTopicsCount).arg(preparationVM.totalTopicsCount)
                            fontWeight: Typography.bold
                        }
                    }

                    AppText {
                        Layout.fillWidth: true
                        text: preparationVM.summaryText
                        color: Theme.zinc300
                        font.pixelSize: Typography.sm
                        lineHeight: Typography.leadingRelaxed
                        wrapMode: Text.WordWrap
                    }
                }
            }

        // 2. Recommended Topics (Study Gaps)
        DarkCard {
            visible: preparationVM.recommendedTopics.length > 0
            Layout.fillWidth: true
            padding: 24

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Icon { name: "book-open"; size: 18; color: Theme.amber400 }
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("What to Study Next")
                        font.pixelSize: Typography.base
                        font.weight: Typography.bold
                    }
                    Badge {
                        tone: "amber"
                        text: qsTr("%1 recommendations").arg(preparationVM.recommendedTopics.length)
                    }
                }

                Repeater {
                    model: preparationVM.recommendedTopics
                    Rectangle {
                        id: topicBox
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: topicCol.implicitHeight + 24
                        radius: Theme.radiusMd
                        color: Theme.alpha(Theme.zinc950, 0.6)
                        border.width: 1
                        border.color: Theme.borderGlass

                        ColumnLayout {
                            id: topicCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 14
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                AppText {
                                    Layout.fillWidth: true
                                    text: topicBox.modelData.title
                                    font.weight: Typography.semibold
                                    font.pixelSize: Typography.sm
                                }
                                Badge {
                                    tone: topicBox.modelData.priority === "high" ? "rose" : (topicBox.modelData.priority === "medium" ? "amber" : "blue")
                                    text: topicBox.modelData.priorityText
                                    fontSize: Typography.tiny
                                }
                                Badge {
                                    tone: topicBox.modelData.status === "unclear" ? "amber" : "neutral"
                                    text: topicBox.modelData.statusText
                                    fontSize: Typography.tiny
                                }
                            }

                            AppText {
                                Layout.fillWidth: true
                                text: topicBox.modelData.reason
                                color: Theme.zinc400
                                font.pixelSize: Typography.xs
                                wrapMode: Text.WordWrap
                            }

                            // Materials list
                            Repeater {
                                model: topicBox.modelData.materials
                                AbstractButton {
                                    id: matLink
                                    required property var modelData
                                    Layout.fillWidth: true
                                    implicitHeight: 44
                                    hoverEnabled: true
                                    onClicked: Qt.openUrlExternally(matLink.modelData.url)
                                    HoverHandler { cursorShape: Qt.PointingHandCursor }

                                    background: Rectangle {
                                        radius: Theme.radiusSm
                                        color: matLink.hovered ? Theme.zinc900 : Theme.alpha(Theme.zinc900, 0.4)
                                        border.width: 1
                                        border.color: matLink.hovered ? Theme.alpha(Theme.blue500, 0.4) : Theme.borderFaint
                                    }

                                    contentItem: RowLayout {
                                        spacing: 10
                                        Item { implicitWidth: 4 }
                                        Icon { name: "external-link"; size: 14; color: matLink.hovered ? Theme.blue400 : Theme.zinc500 }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1
                                            AppText {
                                                Layout.fillWidth: true
                                                text: matLink.modelData.title
                                                font.pixelSize: Typography.xs
                                                font.weight: Typography.medium
                                                color: matLink.hovered ? Theme.blue300 : Theme.zinc200
                                                elide: Text.ElideRight
                                            }
                                            AppText {
                                                Layout.fillWidth: true
                                                text: matLink.modelData.url
                                                font.pixelSize: Typography.tiny
                                                color: Theme.textMuted
                                                elide: Text.ElideRight
                                            }
                                        }
                                        Item { implicitWidth: 4 }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. Covered Topics
        DarkCard {
            visible: preparationVM.coveredTopics.length > 0
            Layout.fillWidth: true
            padding: 20

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Icon { name: "circle-check"; size: 18; color: Theme.emerald400 }
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("Already Mentioned & Covered")
                        font.pixelSize: Typography.base
                        font.weight: Typography.bold
                    }
                    Badge {
                        tone: "emerald"
                        text: qsTr("%1 topics").arg(preparationVM.coveredTopics.length)
                    }
                }

                Repeater {
                    model: preparationVM.coveredTopics
                    RowLayout {
                        id: covRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8

                        Icon { name: "circle-check"; size: 14; color: Theme.emerald500 }
                        AppText {
                            text: covRow.modelData.title
                            font.weight: Typography.medium
                            font.pixelSize: Typography.sm
                            color: Theme.zinc300
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: "— " + covRow.modelData.reason
                            color: Theme.zinc500
                            font.pixelSize: Typography.xs
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        // 4. Disclaimer
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 8

            Icon { name: "info"; size: 14; color: Theme.textMuted }
            AppText {
                Layout.fillWidth: true
                text: qsTr("Note: This evaluation is generated locally from your free-form description and is not a conclusive assessment of your abilities.")
                color: Theme.textMuted
                font.pixelSize: Typography.tiny
                wrapMode: Text.WordWrap
            }
        }

        // 5. Actions Footer
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 12
            spacing: 14

            TactileButton {
                variant: "secondary"
                text: qsTr("Refine description")
                iconLeft: "rotate-ccw"
                implicitHeight: 44
                onClicked: preparationVM.refineDescription()
            }
            TactileButton {
                Layout.fillWidth: true
                text: qsTr("Start interview for this track")
                iconRight: "arrow-right"
                implicitHeight: 44
                glow: true
                onClicked: {
                    setupVM.direction = preparationVM.direction
                    setupVM.level = preparationVM.level
                    navVM.navigateToSetup()
                }
            }
        }

        Item { implicitHeight: 24 }
        }
    }
}
