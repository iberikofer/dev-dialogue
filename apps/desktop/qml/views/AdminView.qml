pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Question bank administration. Reachable from the sidebar only for
// administrators with the admin view switched on.
AppPage {
    id: page

    active: StackLayout.isCurrentItem

    readonly property bool hasAccess: authVM.isAdmin && authVM.isAdminViewActive
    readonly property int rowHeight: 60
    readonly property var columns: [0.13, 0.10, 0.20, 0.35, 0.14, 0.08]
    readonly property var trackFilters: [
        { value: "", label: qsTr("All tracks") },
        { value: "frontend", label: "Frontend" },
        { value: "backend", label: "Backend" },
        { value: "devops", label: "DevOps" },
        { value: "qa", label: "QA" },
        { value: "pm", label: "PM" }
    ]

    function columnWidth(index, tableWidth) {
        return Math.floor((tableWidth - 48) * columns[index])
    }
    function openAddDialog() {
        addDialog.show()
    }
    function reload() {
        if (active && hasAccess)
            adminVM.refresh()
    }

    onActiveChanged: reload()
    onHasAccessChanged: reload()
    Connections {
        target: backendVM
        function onStatusChanged() {
            if (backendVM.isOffline && adminVM.loading)
                adminVM.refresh()
        }
    }

    title: qsTr("Question bank")
    subtitle: qsTr("%1 questions  ·  questions the interviewer draws from").arg(adminVM.totalCount)
    badgeText: adminVM.isDemoData ? qsTr("Demo data") : ""

    actions: [
        TactileButton {
            visible: page.hasAccess
            text: qsTr("Add question")
            iconLeft: "plus"
            implicitHeight: 40
            glow: true
            onClicked: page.openAddDialog()
        }
    ]

    // Only shown if the role changes while the page is open.
    AppText {
        visible: !page.hasAccess
        Layout.fillWidth: true
        text: qsTr("The question bank is available to administrators in admin view.")
        color: Theme.zinc400
    }

    // ---------------------------------------------------------- Filter
    RowLayout {
        visible: page.hasAccess
        Layout.fillWidth: true
        spacing: 8

        Repeater {
            model: page.trackFilters
            AbstractButton {
                id: chip
                required property var modelData
                readonly property bool selected: adminVM.directionFilter === modelData.value

                implicitHeight: 32
                implicitWidth: chipText.implicitWidth + 28
                hoverEnabled: true
                onClicked: adminVM.directionFilter = modelData.value
                HoverHandler { cursorShape: Qt.PointingHandCursor }

                background: Rectangle {
                    radius: height / 2
                    color: chip.selected ? Theme.alpha(Theme.blue600, 0.25) : (chip.hovered ? Theme.zinc800 : "transparent")
                    border.width: 1
                    border.color: chip.selected ? Theme.alpha(Theme.blue400, 0.5) : (chip.visualFocus ? Theme.focusRing : Theme.borderStrong)
                    Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                }
                contentItem: Item {
                    AppText {
                        id: chipText
                        anchors.centerIn: parent
                        text: chip.modelData.label
                        color: chip.selected ? Theme.blue200 : Theme.zinc300
                        font.pixelSize: Typography.xs
                        font.weight: Typography.medium
                    }
                }
            }
        }
        Item { Layout.fillWidth: true }
    }

    AlertBox {
        visible: page.hasAccess && adminVM.errorMessage !== ""
        Layout.fillWidth: true
        text: adminVM.errorMessage
    }

    // ----------------------------------------------------------- Table
    Rectangle {
        id: table
        visible: page.hasAccess
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

            // Column headers
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 42
                color: Theme.alpha(Theme.zinc950, 0.6)
                Row {
                    x: 24
                    height: parent.height
                    Repeater {
                        model: [qsTr("Track"), qsTr("Level"), qsTr("Topic"), qsTr("Question"), qsTr("Active"), ""]
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

            // Loading
            ColumnLayout {
                visible: adminVM.loading
                Layout.fillWidth: true
                Layout.margins: 24
                spacing: 24
                Repeater {
                    model: 4
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 24
                        Skeleton { Layout.preferredWidth: 72; Layout.preferredHeight: 16; radius: 4 }
                        Skeleton { Layout.preferredWidth: 56; Layout.preferredHeight: 14; radius: 4 }
                        Skeleton { Layout.preferredWidth: 160; Layout.preferredHeight: 14; radius: 4 }
                        Skeleton { Layout.fillWidth: true; Layout.preferredHeight: 14; radius: 4 }
                    }
                }
                Item { Layout.fillHeight: true }
            }

            // Empty
            ColumnLayout {
                visible: !adminVM.loading && adminVM.questionCount === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12
                Item { Layout.fillHeight: true }
                AppText {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("No questions for this track yet")
                    font.weight: Typography.semibold
                }
                TactileButton {
                    Layout.alignment: Qt.AlignHCenter
                    variant: "secondary"
                    text: qsTr("Add the first one")
                    iconLeft: "plus"
                    onClicked: page.openAddDialog()
                }
                Item { Layout.fillHeight: true }
            }

            ListView {
                id: rows
                visible: !adminVM.loading && adminVM.questionCount > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: adminVM.questions
                ScrollBar.vertical: ThemedScrollBar {}

                delegate: Rectangle {
                    id: row
                    required property int index
                    required property string questionId
                    required property string direction
                    required property string level
                    required property string topic
                    required property string questionText
                    required property bool isActive

                    width: rows.width
                    height: page.rowHeight
                    color: rowHover.hovered ? Theme.alpha(Theme.zinc800, 0.35) : "transparent"
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

                        Item {
                            width: page.columnWidth(0, table.width)
                            height: parent.height
                            Badge {
                                anchors.verticalCenter: parent.verticalCenter
                                mono: true
                                caps: true
                                text: row.direction
                                fontSize: Typography.tiny
                                fontWeight: Typography.semibold
                            }
                        }
                        AppText {
                            width: page.columnWidth(1, table.width)
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.level
                            color: Theme.zinc300
                            font.pixelSize: Typography.xs
                            font.capitalization: Font.Capitalize
                        }
                        AppText {
                            width: page.columnWidth(2, table.width) - 16
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.topic
                            font.weight: Typography.medium
                            elide: Text.ElideRight
                        }
                        Item { width: 16; height: 1 }
                        AppText {
                            width: page.columnWidth(3, table.width) - 16
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: row.questionText
                            color: Theme.zinc300
                            font.pixelSize: Typography.xs
                            lineHeight: Typography.leadingSnug
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                        Item { width: 16; height: 1 }
                        Item {
                            width: page.columnWidth(4, table.width)
                            height: parent.height
                            ToggleSwitch {
                                anchors.verticalCenter: parent.verticalCenter
                                checked: row.isActive
                                onToggled: adminVM.setQuestionActive(row.questionId, checked)
                                ToolTip.visible: hovered
                                ToolTip.delay: 600
                                ToolTip.text: row.isActive ? qsTr("Used in interviews") : qsTr("Not used in interviews")
                            }
                        }
                        Item {
                            width: page.columnWidth(5, table.width)
                            height: parent.height
                            AbstractButton {
                                id: removeButton
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                implicitWidth: 30
                                implicitHeight: 30
                                hoverEnabled: true
                                opacity: rowHover.hovered || hovered ? 1 : 0.4
                                onClicked: adminVM.deleteQuestion(row.questionId)
                                HoverHandler { cursorShape: Qt.PointingHandCursor }
                                ToolTip.visible: hovered
                                ToolTip.delay: 400
                                ToolTip.text: qsTr("Delete question")
                                background: Rectangle {
                                    radius: Theme.radiusSm
                                    color: removeButton.hovered ? Theme.alpha(Theme.red500, 0.12) : "transparent"
                                }
                                contentItem: Item {
                                    Icon {
                                        anchors.centerIn: parent
                                        name: "trash-2"
                                        size: 16
                                        color: removeButton.hovered ? Theme.red400 : Theme.zinc400
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Pagination
            Rectangle {
                visible: !adminVM.loading && adminVM.totalPages > 1
                Layout.fillWidth: true
                implicitHeight: 52
                color: "transparent"
                Rectangle { width: parent.width; height: 1; color: Theme.borderGlass }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 16
                    spacing: 8
                    MonoText {
                        Layout.fillWidth: true
                        text: qsTr("Page %1 of %2").arg(adminVM.page).arg(adminVM.totalPages)
                    }
                    TactileButton {
                        variant: "secondary"
                        text: qsTr("Previous")
                        fontSize: Typography.xs
                        implicitHeight: 30
                        leftPadding: 12
                        rightPadding: 12
                        buttonRadius: Theme.radiusSm
                        enabled: adminVM.page > 1
                        onClicked: adminVM.previousPage()
                    }
                    TactileButton {
                        variant: "secondary"
                        text: qsTr("Next")
                        fontSize: Typography.xs
                        implicitHeight: 30
                        leftPadding: 12
                        rightPadding: 12
                        buttonRadius: Theme.radiusSm
                        enabled: adminVM.page < adminVM.totalPages
                        onClicked: adminVM.nextPage()
                    }
                }
            }
        }
    }

    // --------------------------------------------------- Add question
    ModalOverlay {
        id: addDialog

        property bool attempted: false
        readonly property bool topicValid: topicField.text.trim().length >= 3
        readonly property bool questionValid: questionField.text.trim().length >= 10

        function show() {
            attempted = false
            topicField.text = ""
            questionField.text = ""
            trackSelect.currentIndex = 0
            levelSelect.currentIndex = 1
            open = true
            topicField.forceActiveFocus()
        }
        function submit() {
            attempted = true
            if (!topicValid || !questionValid || adminVM.isSubmitting)
                return
            adminVM.createQuestion(trackSelect.currentValue, levelSelect.currentValue,
                                   topicField.text.trim(), questionField.text.trim())
        }

        dialogWidth: 512
        dialogPadding: 24
        dialogRadius: Theme.radiusXl
        dialogColor: Theme.zinc900
        dialogBorder: Theme.borderGlass
        backdropOpacity: 0.7
        onCloseRequested: open = false

        Connections {
            target: adminVM
            function onQuestionCreated() { addDialog.open = false }
        }

        component FieldLabel: AppText {
            Layout.fillWidth: true
            Layout.bottomMargin: 4
            color: Theme.zinc400
            font.pixelSize: Typography.xs
            font.weight: Typography.semibold
        }
        component FieldError: AppText {
            Layout.fillWidth: true
            Layout.topMargin: 4
            color: Theme.red400
            font.pixelSize: Typography.tiny
            wrapMode: Text.WordWrap
        }

        AppText {
            Layout.fillWidth: true
            Layout.bottomMargin: 16
            text: qsTr("New question")
            color: Theme.textOnAccent
            font.pixelSize: Typography.lg
            font.weight: Typography.bold
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                spacing: 0
                FieldLabel { text: qsTr("Track") }
                SelectBox {
                    id: trackSelect
                    Layout.fillWidth: true
                    model: [
                        { value: "frontend", label: "Frontend" },
                        { value: "backend", label: "Backend" },
                        { value: "devops", label: "DevOps" },
                        { value: "qa", label: "QA" },
                        { value: "pm", label: qsTr("Project Manager") }
                    ]
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 100
                spacing: 0
                FieldLabel { text: qsTr("Level") }
                SelectBox {
                    id: levelSelect
                    Layout.fillWidth: true
                    model: [
                        { value: "trainee", label: "Trainee" },
                        { value: "junior", label: "Junior" },
                        { value: "middle", label: "Middle" }
                    ]
                }
            }
        }

        FieldLabel {
            Layout.topMargin: 16
            text: qsTr("Topic")
        }
        DarkTextField {
            id: topicField
            Layout.fillWidth: true
            appearance: "solid"
            placeholderText: qsTr("e.g. React Lifecycle or TCP/UDP")
            invalid: addDialog.attempted && !addDialog.topicValid
            onAccepted: questionField.area.forceActiveFocus()
        }
        FieldError {
            visible: addDialog.attempted && !addDialog.topicValid
            text: qsTr("Enter a topic of at least 3 characters.")
        }

        FieldLabel {
            Layout.topMargin: 16
            text: qsTr("Question")
        }
        DarkTextArea {
            id: questionField
            Layout.fillWidth: true
            appearance: "solid"
            placeholderText: qsTr("Enter technical question prompt...")
            invalid: addDialog.attempted && !addDialog.questionValid
        }
        FieldError {
            visible: addDialog.attempted && !addDialog.questionValid
            text: qsTr("Enter the question text, at least 10 characters.")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 24
            spacing: 12

            Item { Layout.fillWidth: true }
            TactileButton {
                id: cancelButton
                variant: "ghost"
                text: qsTr("Cancel")
                fontSize: Typography.xs
                implicitHeight: 34
                leftPadding: 16
                rightPadding: 16
                onClicked: addDialog.open = false
                background: Rectangle {
                    radius: Theme.radiusMd
                    color: cancelButton.hovered ? Theme.zinc800 : "transparent"
                    border.width: 1
                    border.color: Theme.zinc700
                }
            }
            TactileButton {
                text: adminVM.isSubmitting ? qsTr("Saving...") : qsTr("Save")
                fontSize: Typography.xs
                implicitHeight: 34
                leftPadding: 20
                rightPadding: 20
                enabled: !adminVM.isSubmitting
                onClicked: addDialog.submit()
            }
        }
    }
}
