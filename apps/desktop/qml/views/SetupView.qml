import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: setupView

    readonly property var directionOptions: [
        { id: "frontend" },
        { id: "backend" },
        { id: "devops" },
        { id: "qa" },
        { id: "pm" }
    ]

    readonly property var levelOptions: [
        { label: qsTr("Trainee"), value: "trainee" },
        { label: qsTr("Junior"), value: "junior" },
        { label: qsTr("Middle"), value: "middle" }
    ]

    readonly property var languageOptions: [
        { label: qsTr("Українська"), value: "uk" },
        { label: qsTr("English"), value: "en" }
    ]

    onVisibleChanged: {
        if (visible)
            setupVM.clearError()
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
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("Set up your interview")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.h1
                        font.weight: Typography.weightBold
                        color: Theme.textPrimary
                    }

                    Text {
                        text: qsTr("Choose a track, seniority level and language. You can change these before every session.")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.body
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                TactileButton {
                    text: qsTr("Back to dashboard")
                    outlined: true
                    implicitWidth: 170
                    onClicked: navVM.navigateToDashboard()
                }
            }

            ErrorBanner {
                Layout.fillWidth: true
                errorMessage: setupVM.errorMessage
                onDismissed: setupVM.clearError()
            }

            // -----------------------------------------------------------
            // Track selector
            // -----------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: qsTr("1. Choose your track")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.h3
                    font.weight: Typography.weightSemibold
                    color: Theme.textPrimary
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 14
                    rowSpacing: 14

                    Repeater {
                        model: setupView.directionOptions

                        delegate: Rectangle {
                            id: directionCard
                            required property var modelData

                            readonly property bool selected: setupVM.direction === modelData.id

                            Layout.fillWidth: true
                            Layout.preferredHeight: 118
                            radius: Theme.radiusLg
                            color: directionCard.selected
                                   ? Qt.rgba(0.39, 0.4, 0.95, 0.12)
                                   : Theme.backgroundCard
                            border.width: 1
                            border.color: directionCard.selected
                                          ? Theme.accentIndigo
                                          : (directionHover.hovered ? Theme.borderHighlight
                                                                    : Theme.borderSubtle)

                            Behavior on border.color {
                                ColorAnimation { duration: Theme.durationFast }
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    Rectangle {
                                        Layout.preferredWidth: 34
                                        Layout.preferredHeight: 34
                                        radius: Theme.radiusSm
                                        color: directionCard.selected
                                               ? Qt.rgba(0.39, 0.4, 0.95, 0.2)
                                               : Theme.backgroundElevated
                                        border.width: 1
                                        border.color: directionCard.selected
                                                      ? Qt.rgba(0.39, 0.4, 0.95, 0.45)
                                                      : Theme.borderSubtle

                                        Image {
                                            anchors.centerIn: parent
                                            source: Labels.directionIcon(directionCard.modelData.id)
                                            sourceSize.width: 18
                                            sourceSize.height: 18
                                            width: 18
                                            height: 18
                                            fillMode: Image.PreserveAspectFit
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: Labels.directionLabel(directionCard.modelData.id)
                                        font.family: Theme.fontSans
                                        font.pixelSize: Typography.h3
                                        font.weight: Typography.weightSemibold
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        visible: directionCard.selected
                                        Layout.preferredWidth: 20
                                        Layout.preferredHeight: 20
                                        Layout.alignment: Qt.AlignVCenter
                                        radius: 10
                                        color: Theme.accentIndigo

                                        Text {
                                            anchors.centerIn: parent
                                            text: "✓"
                                            font.family: Theme.fontSans
                                            font.pixelSize: 12
                                            font.weight: Typography.weightBold
                                            color: "#ffffff"
                                        }
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: Labels.directionDescription(directionCard.modelData.id)
                                    font.family: Theme.fontSans
                                    font.pixelSize: Typography.small
                                    lineHeight: Typography.lineNormal
                                    color: Theme.textSecondary
                                    wrapMode: Text.Wrap
                                }
                            }

                            HoverHandler {
                                id: directionHover
                                cursorShape: Qt.PointingHandCursor
                            }

                            TapHandler {
                                onTapped: setupVM.direction = directionCard.modelData.id
                            }

                            Accessible.role: Accessible.RadioButton
                            Accessible.name: Labels.directionLabel(directionCard.modelData.id)
                            Accessible.checked: directionCard.selected
                        }
                    }
                }
            }

            // -----------------------------------------------------------
            // Level + language
            // -----------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                DarkCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 128
                    contentPadding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            text: qsTr("2. Seniority level")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.body
                            font.weight: Typography.weightSemibold
                            color: Theme.textPrimary
                        }

                        SegmentedControl {
                            id: levelControl
                            items: setupView.levelOptions
                            currentValue: setupVM.level
                            onValueSelected: (value) => setupVM.level = value
                        }

                        Text {
                            Layout.fillWidth: true
                            text: Labels.levelDescription(setupVM.level)
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.small
                            color: Theme.textMuted
                            wrapMode: Text.Wrap
                        }
                    }
                }

                DarkCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 128
                    contentPadding: 18

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        Text {
                            text: qsTr("3. Interview language")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.body
                            font.weight: Typography.weightSemibold
                            color: Theme.textPrimary
                        }

                        SegmentedControl {
                            id: languageControl
                            items: setupView.languageOptions
                            currentValue: setupVM.language
                            onValueSelected: (value) => setupVM.language = value
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Questions and feedback will be generated in this language.")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.small
                            color: Theme.textMuted
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            // -----------------------------------------------------------
            // Summary + action
            // -----------------------------------------------------------
            DarkCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                contentPadding: 18

                RowLayout {
                    anchors.fill: parent
                    spacing: 16

                    Image {
                        source: Labels.directionIcon(setupVM.direction)
                        sourceSize.width: 24
                        sourceSize.height: 24
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        fillMode: Image.PreserveAspectFit
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: qsTr("%1 · %2 · %3")
                                  .arg(Labels.directionLabel(setupVM.direction))
                                  .arg(Labels.levelLabel(setupVM.level))
                                  .arg(Labels.languageLabel(setupVM.language))
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.body
                            font.weight: Typography.weightSemibold
                            color: Theme.textPrimary
                        }

                        Text {
                            text: qsTr("The AI interviewer will ask roughly 4–5 questions and then produce a detailed scorecard.")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.small
                            color: Theme.textMuted
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Spinner {
                        visible: setupVM.isBusy
                        diameter: 20
                    }

                    TactileButton {
                        implicitWidth: 220
                        implicitHeight: 44
                        text: setupVM.isBusy ? qsTr("Preparing session…")
                                             : qsTr("Start interview session")
                        iconSource: Icons.sparkles
                        enabled: !setupVM.isBusy
                        onClicked: setupVM.startInterview()
                    }
                }
            }
        }
    }
}
