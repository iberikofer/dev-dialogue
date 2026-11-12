import QtQuick
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: adminView

    property string newDirection: "frontend"
    property string newLevel: "junior"
    property string newStage: "hard_skills"
    property string deleteTargetId: ""
    property string deleteTargetLabel: ""

    readonly property var directionItems: [
        { label: qsTr("All"), value: "" },
        { label: qsTr("Frontend"), value: "frontend" },
        { label: qsTr("Backend"), value: "backend" },
        { label: qsTr("DevOps"), value: "devops" },
        { label: qsTr("QA"), value: "qa" },
        { label: qsTr("PM"), value: "pm" }
    ]

    readonly property var levelItems: [
        { label: qsTr("All"), value: "" },
        { label: qsTr("Trainee"), value: "trainee" },
        { label: qsTr("Junior"), value: "junior" },
        { label: qsTr("Middle"), value: "middle" }
    ]

    readonly property var stageItems: [
        { label: qsTr("All"), value: "" },
        { label: qsTr("Hard skills"), value: "hard_skills" },
        { label: qsTr("Soft skills"), value: "soft_skills" },
        { label: qsTr("English"), value: "english" }
    ]

    readonly property var statusItems: [
        { label: qsTr("All"), value: "all" },
        { label: qsTr("Active"), value: "active" },
        { label: qsTr("Inactive"), value: "inactive" }
    ]

    readonly property var formDirectionItems: [
        { label: qsTr("Frontend"), value: "frontend" },
        { label: qsTr("Backend"), value: "backend" },
        { label: qsTr("DevOps"), value: "devops" },
        { label: qsTr("QA"), value: "qa" },
        { label: qsTr("PM"), value: "pm" }
    ]

    readonly property var formLevelItems: [
        { label: qsTr("Trainee"), value: "trainee" },
        { label: qsTr("Junior"), value: "junior" },
        { label: qsTr("Middle"), value: "middle" }
    ]

    readonly property var formStageItems: [
        { label: qsTr("Hard skills"), value: "hard_skills" },
        { label: qsTr("Soft skills"), value: "soft_skills" },
        { label: qsTr("English"), value: "english" }
    ]

    function resetForm() {
        newDirection = "frontend"
        newLevel = "junior"
        newStage = "hard_skills"
        newTopic.text = ""
        newQuestion.text = ""
        adminVM.clearError()
    }

    onVisibleChanged: {
        if (visible)
            adminVM.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        // ---------------------------------------------------------------
        // Header
        // ---------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: qsTr("Question bank")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.h1
                    font.weight: Typography.weightBold
                    color: Theme.textPrimary
                }

                Text {
                    text: qsTr("Curate the questions the AI interviewer can draw from. Changes apply to new sessions.")
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.body
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Spinner {
                visible: adminVM.isBusy
                diameter: 20
            }

            TactileButton {
                text: qsTr("Refresh")
                outlined: true
                implicitWidth: 110
                iconSource: Icons.refresh
                enabled: !adminVM.isBusy
                onClicked: adminVM.refresh()
            }

            TactileButton {
                text: qsTr("Add question")
                implicitWidth: 158
                iconSource: Icons.plus
                onClicked: {
                    resetForm()
                    addModal.visible = true
                }
            }
        }

        ErrorBanner {
            Layout.fillWidth: true
            errorMessage: adminVM.errorMessage
            onDismissed: adminVM.clearError()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: successRow.implicitHeight + 20
            visible: adminVM.successMessage.length > 0
            radius: Theme.radiusMd
            color: Qt.rgba(0.06, 0.72, 0.51, 0.1)
            border.width: 1
            border.color: Qt.rgba(0.06, 0.72, 0.51, 0.32)

            RowLayout {
                id: successRow
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 10
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: adminVM.successMessage
                    font.family: Theme.fontSans
                    font.pixelSize: Typography.bodySmall
                    color: Theme.accentEmerald
                }

                TactileButton {
                    text: "✕"
                    outlined: true
                    implicitWidth: 30
                    implicitHeight: 30
                    buttonRadius: Theme.radiusSm
                    textColor: Theme.accentEmerald
                    onClicked: adminVM.clearSuccess()
                }
            }
        }

        // ---------------------------------------------------------------
        // Filters
        // ---------------------------------------------------------------
        DarkCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 196
            contentPadding: 18

            GridLayout {
                anchors.fill: parent
                columns: 2
                columnSpacing: 18
                rowSpacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 54
                        text: qsTr("Track")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }

                    SegmentedControl {
                        id: directionControl
                        items: adminView.directionItems
                        currentValue: adminVM.direction
                        onValueSelected: (value) => adminVM.applyFilters(
                                              value, levelControl.currentValue,
                                              stageControl.currentValue,
                                              statusControl.currentValue)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 54
                        text: qsTr("Level")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }

                    SegmentedControl {
                        id: levelControl
                        items: adminView.levelItems
                        currentValue: adminVM.level
                        onValueSelected: (value) => adminVM.applyFilters(
                                              directionControl.currentValue, value,
                                              stageControl.currentValue,
                                              statusControl.currentValue)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 54
                        text: qsTr("Stage")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }

                    SegmentedControl {
                        id: stageControl
                        items: adminView.stageItems
                        currentValue: adminVM.stage
                        onValueSelected: (value) => adminVM.applyFilters(
                                              directionControl.currentValue,
                                              levelControl.currentValue, value,
                                              statusControl.currentValue)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 54
                        text: qsTr("Status")
                        font.family: Theme.fontSans
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }

                    SegmentedControl {
                        id: statusControl
                        items: adminView.statusItems
                        currentValue: adminVM.activeFilter
                        onValueSelected: (value) => adminVM.applyFilters(
                                              directionControl.currentValue,
                                              levelControl.currentValue,
                                              stageControl.currentValue, value)
                    }
                }

                Item { Layout.fillWidth: true }

                TactileButton {
                    text: qsTr("Clear filters")
                    outlined: true
                    implicitWidth: 120
                    implicitHeight: 34
                    buttonRadius: Theme.radiusSm
                    onClicked: adminVM.clearFilters()
                }
            }
        }

        // ---------------------------------------------------------------
        // Question table
        // ---------------------------------------------------------------
        DarkCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentPadding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    Layout.topMargin: 12
                    Layout.bottomMargin: 10
                    spacing: 12

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Question")
                        font.family: Theme.fontMono
                        font.pixelSize: Typography.tiny
                        color: Theme.textMuted
                    }

                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("Level")
                        font.family: Theme.fontMono
                        font.pixelSize: Typography.tiny
                        color: Theme.textMuted
                    }

                    Text {
                        Layout.preferredWidth: 110
                        text: qsTr("Status")
                        font.family: Theme.fontMono
                        font.pixelSize: Typography.tiny
                        color: Theme.textMuted
                    }

                    Text {
                        Layout.preferredWidth: 80
                        horizontalAlignment: Text.AlignRight
                        text: qsTr("Actions")
                        font.family: Theme.fontMono
                        font.pixelSize: Typography.tiny
                        color: Theme.textMuted
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: questionList
                        anchors.fill: parent
                        clip: true
                        model: adminVM.questionsModel
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Item {
                            width: ListView.view ? ListView.view.width : 0
                            height: 80

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 18
                                anchors.rightMargin: 14
                                spacing: 12

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    RowLayout {
                                        spacing: 8

                                        Badge {
                                            text: Labels.directionShortLabel(model.direction)
                                            toneColor: Theme.accentIndigo
                                        }

                                        Badge {
                                            text: Labels.stageLabel(model.stage)
                                            toneColor: Theme.accentTeal
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: model.topic
                                            font.family: Theme.fontMono
                                            font.pixelSize: Typography.tiny
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: model.questionText
                                        font.family: Theme.fontSans
                                        font.pixelSize: Typography.bodySmall
                                        color: Theme.textPrimary
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 90
                                    Layout.preferredHeight: 24

                                    Badge {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: Labels.levelLabel(model.level)
                                        toneColor: Theme.accentAmber
                                    }
                                }

                                RowLayout {
                                    Layout.preferredWidth: 110
                                    spacing: 8

                                    PillSwitch {
                                        checked: model.isActive
                                        onToggled: adminVM.setQuestionActive(model.questionId,
                                                                             !model.isActive)
                                    }

                                    Text {
                                        text: model.isActive ? qsTr("Active")
                                                             : qsTr("Inactive")
                                        font.family: Theme.fontSans
                                        font.pixelSize: Typography.tiny
                                        color: model.isActive ? Theme.accentEmerald
                                                              : Theme.textMuted
                                    }
                                }

                                TactileButton {
                                    Layout.preferredWidth: 80
                                    implicitHeight: 30
                                    buttonRadius: Theme.radiusSm
                                    outlined: true
                                    textColor: Theme.accentRose
                                    text: qsTr("Delete")
                                    onClicked: {
                                        adminView.deleteTargetId = model.questionId
                                        adminView.deleteTargetLabel = model.questionText
                                        deleteModal.visible = true
                                    }
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 18
                                height: 1
                                color: Theme.borderSubtle
                                opacity: 0.6
                            }
                        }
                    }

                    EmptyState {
                        anchors.fill: parent
                        visible: adminVM.questionsModel.count === 0 && !adminVM.isBusy
                        iconSource: Icons.dashboard
                        title: qsTr("No questions match these filters")
                        description: qsTr("Adjust the filters or add a new question to grow the bank.")
                        actionText: qsTr("Clear filters")
                        onActionClicked: adminVM.clearFilters()
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 12
                        visible: adminVM.questionsModel.count === 0 && adminVM.isBusy

                        Spinner {
                            anchors.horizontalCenter: parent.horizontalCenter
                            diameter: 26
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Loading questions…")
                            font.family: Theme.fontSans
                            font.pixelSize: Typography.small
                            color: Theme.textMuted
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 18
                    Layout.rightMargin: 14
                    Layout.topMargin: 10
                    Layout.bottomMargin: 12
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%1 questions · page %2 of %3")
                              .arg(adminVM.totalCount)
                              .arg(adminVM.page)
                              .arg(Math.max(adminVM.totalPages, 1))
                        font.family: Theme.fontMono
                        font.pixelSize: Typography.small
                        color: Theme.textMuted
                    }

                    TactileButton {
                        text: qsTr("Previous")
                        outlined: true
                        implicitWidth: 104
                        implicitHeight: 32
                        buttonRadius: Theme.radiusSm
                        enabled: adminVM.page > 1 && !adminVM.isBusy
                        onClicked: adminVM.previousPage()
                    }

                    TactileButton {
                        text: qsTr("Next")
                        outlined: true
                        implicitWidth: 84
                        implicitHeight: 32
                        buttonRadius: Theme.radiusSm
                        enabled: adminVM.page < adminVM.totalPages && !adminVM.isBusy
                        onClicked: adminVM.nextPage()
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------
    // Add question modal
    // -------------------------------------------------------------------
    AppModal {
        id: addModal
        anchors.fill: parent
        z: 100
        title: qsTr("Add a question")
        modalWidth: 640
        contentSpacing: 14
        onClosed: {
            visible = false
            adminView.resetForm()
        }

        Text {
            width: parent.width
            text: qsTr("The question becomes available to the mock interviewer and the backend question bank.")
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            lineHeight: Typography.lineNormal
            color: Theme.textSecondary
            wrapMode: Text.Wrap
        }

        Text {
            text: qsTr("Track")
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            font.weight: Typography.weightMedium
            color: Theme.textSecondary
        }

        SegmentedControl {
            items: adminView.formDirectionItems
            currentValue: adminView.newDirection
            onValueSelected: (value) => adminView.newDirection = value
        }

        Text {
            text: qsTr("Level")
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            font.weight: Typography.weightMedium
            color: Theme.textSecondary
        }

        SegmentedControl {
            items: adminView.formLevelItems
            currentValue: adminView.newLevel
            onValueSelected: (value) => adminView.newLevel = value
        }

        Text {
            text: qsTr("Stage")
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            font.weight: Typography.weightMedium
            color: Theme.textSecondary
        }

        SegmentedControl {
            items: adminView.formStageItems
            currentValue: adminView.newStage
            onValueSelected: (value) => adminView.newStage = value
        }

        Text {
            text: qsTr("Topic")
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            font.weight: Typography.weightMedium
            color: Theme.textSecondary
        }

        DarkTextField {
            id: newTopic
            width: parent.width
            placeholderText: qsTr("e.g. JavaScript, Kubernetes, Product discovery")
        }

        Text {
            text: qsTr("Question text")
            font.family: Theme.fontSans
            font.pixelSize: Typography.small
            font.weight: Typography.weightMedium
            color: Theme.textSecondary
        }

        DarkTextArea {
            id: newQuestion
            width: parent.width
            placeholderText: qsTr("Write the full question the interviewer should ask…")
        }

        ErrorBanner {
            width: parent.width
            errorMessage: adminVM.errorMessage
            onDismissed: adminVM.clearError()
        }

        RowLayout {
            width: parent.width
            spacing: 10

            Item { Layout.fillWidth: true }

            TactileButton {
                text: qsTr("Cancel")
                outlined: true
                implicitWidth: 104
                onClicked: addModal.closed()
            }

            TactileButton {
                text: qsTr("Add question")
                implicitWidth: 150
                enabled: newTopic.text.trim().length > 0
                         && newQuestion.text.trim().length > 0
                         && !adminVM.isBusy
                onClicked: {
                    adminVM.createQuestion(adminView.newDirection,
                                           adminView.newStage,
                                           adminView.newLevel,
                                           newTopic.text.trim(),
                                           newQuestion.text.trim())
                    addModal.visible = false
                    adminView.resetForm()
                }
            }
        }
    }

    // -------------------------------------------------------------------
    // Delete confirmation modal
    // -------------------------------------------------------------------
    AppModal {
        id: deleteModal
        anchors.fill: parent
        z: 101
        title: qsTr("Delete question?")
        modalWidth: 520
        contentSpacing: 16
        onClosed: visible = false

        Text {
            width: parent.width
            text: qsTr("This removes the question from the bank. Active interviews keep their current transcript.")
            font.family: Theme.fontSans
            font.pixelSize: Typography.bodySmall
            lineHeight: Typography.lineNormal
            color: Theme.textSecondary
            wrapMode: Text.Wrap
        }

        Rectangle {
            width: parent.width
            height: previewText.implicitHeight + 24
            radius: Theme.radiusSm
            color: Theme.backgroundPrimary
            border.width: 1
            border.color: Theme.borderSubtle

            Text {
                id: previewText
                anchors.fill: parent
                anchors.margins: 12
                text: adminView.deleteTargetLabel
                font.family: Theme.fontSans
                font.pixelSize: Typography.small
                color: Theme.textPrimary
                wrapMode: Text.Wrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }

        RowLayout {
            width: parent.width
            spacing: 10

            Item { Layout.fillWidth: true }

            TactileButton {
                text: qsTr("Cancel")
                outlined: true
                implicitWidth: 104
                onClicked: deleteModal.closed()
            }

            TactileButton {
                text: qsTr("Delete")
                implicitWidth: 110
                bgGradientStart: Theme.accentRose
                bgGradientEnd: Qt.darker(Theme.accentRose, 1.2)
                onClicked: {
                    adminVM.deleteQuestion(adminView.deleteTargetId)
                    deleteModal.visible = false
                }
            }
        }
    }
}
