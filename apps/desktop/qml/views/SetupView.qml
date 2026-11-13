pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// New interview: choose track, level and language on the left; the summary
// panel on the right always shows the current choice and starts the session.
AppPage {
    id: page

    active: StackLayout.isCurrentItem
    title: qsTr("New interview")
    subtitle: qsTr("Pick a track, a level and the interview language.")

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
    readonly property var languages: [
        { id: "uk", short: "UA", title: "Українська" },
        { id: "en", short: "EN", title: "English" }
    ]

    function lookup(list, id) {
        for (let i = 0; i < list.length; ++i)
            if (list[i].id === id)
                return list[i]
        return list[0]
    }

    component SectionLabel: MonoText {
        Layout.fillWidth: true
        caps: true
        font.weight: Typography.semibold
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 24

        // ------------------------------------------------------ Choices
        Flickable {
            id: choices
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: choiceColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ThemedScrollBar {}

            ColumnLayout {
                id: choiceColumn
                width: choices.width - 12
                spacing: 12

                AlertBox {
                    visible: setupVM.errorMessage !== ""
                    Layout.fillWidth: true
                    Layout.bottomMargin: 8
                    text: setupVM.errorMessage
                }

                SectionLabel { text: qsTr("Track") }
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
                            selected: setupVM.direction === modelData.id
                            onClicked: setupVM.direction = modelData.id

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

                SectionLabel { Layout.topMargin: 16; text: qsTr("Level") }
                RowLayout {
                    id: levelRow
                    Layout.fillWidth: true
                    spacing: 12
                    Repeater {
                        model: page.levels
                        SelectCard {
                            id: levelCard
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            selected: setupVM.level === modelData.id
                            onClicked: setupVM.level = modelData.id
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
                            }
                        }
                    }
                }

                SectionLabel { Layout.topMargin: 16; text: qsTr("Interview language") }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 420
                    spacing: 12
                    Repeater {
                        model: page.languages
                        SelectCard {
                            id: languageCard
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            selected: setupVM.language === modelData.id
                            onClicked: setupVM.language = modelData.id
                            contentItem: RowLayout {
                                spacing: 12
                                MonoText {
                                    text: languageCard.modelData.short
                                    color: languageCard.selected ? Theme.blue300 : Theme.zinc500
                                    font.pixelSize: Typography.base
                                    font.weight: Typography.bold
                                }
                                AppText {
                                    Layout.fillWidth: true
                                    text: languageCard.modelData.title
                                    font.weight: Typography.semibold
                                }
                            }
                        }
                    }
                }
                Item { implicitHeight: 8 }
            }
        }

        // ------------------------------------------------------ Summary
        DarkCard {
            Layout.preferredWidth: 300
            Layout.alignment: Qt.AlignTop
            padding: 24
            fillColor: Theme.alpha(Theme.zinc900, 0.7)

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 0

                MonoText {
                    caps: true
                    text: qsTr("Your session")
                    font.weight: Typography.semibold
                }

                Repeater {
                    model: [
                        { label: qsTr("Track"), value: page.lookup(page.directions, setupVM.direction).title },
                        { label: qsTr("Level"), value: page.lookup(page.levels, setupVM.level).title },
                        { label: qsTr("Language"), value: page.lookup(page.languages, setupVM.language).title }
                    ]
                    RowLayout {
                        id: summaryRow
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        AppText {
                            Layout.fillWidth: true
                            text: summaryRow.modelData.label
                            color: Theme.zinc400
                        }
                        AppText {
                            text: summaryRow.modelData.value
                            font.weight: Typography.semibold
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Layout.bottomMargin: 20
                    implicitHeight: 1
                    color: Theme.borderFaint
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Icon { Layout.alignment: Qt.AlignTop; name: "info"; size: 16; color: Theme.blue400 }
                    AppText {
                        Layout.fillWidth: true
                        text: qsTr("5–7 questions. Finish whenever you like and get a scorecard for the answers given.")
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                        lineHeight: Typography.leadingNormal
                        wrapMode: Text.WordWrap
                    }
                }

                TactileButton {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    text: setupVM.isBusy ? qsTr("Starting...") : qsTr("Start interview")
                    iconRight: setupVM.isBusy ? "" : "arrow-right"
                    busy: setupVM.isBusy
                    enabled: !setupVM.isBusy
                    implicitHeight: 46
                    glow: true
                    onClicked: setupVM.start()
                }
            }
        }
    }
}
