pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Question bank administration (web AdminPanel.tsx). Only reachable for
// administrators with the admin view switched on; everyone else sees the
// access notice.
PageScroll {
    id: page

    active: StackLayout.isCurrentItem
    maxContentWidth: Theme.contentWidthDefault

    readonly property bool hasAccess: authVM.isAdmin && authVM.isAdminViewActive
    readonly property bool wide: width >= 720
    readonly property int rowHeight: 64
    readonly property int maxVisibleRows: 8
    readonly property var columns: [0.13, 0.10, 0.20, 0.34, 0.14, 0.09]
    readonly property var trackFilters: [
        { value: "", label: qsTr("All Tracks") },
        { value: "frontend", label: "FRONTEND" },
        { value: "backend", label: "BACKEND" },
        { value: "devops", label: "DEVOPS" },
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

    // ------------------------------------------------- Access restricted
    ColumnLayout {
        visible: !page.hasAccess
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 48
        Layout.fillWidth: true
        Layout.maximumWidth: 672
        spacing: 0

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 56
            implicitHeight: 56
            radius: Theme.radiusLg
            color: Theme.alpha(Theme.amber500, 0.1)
            border.width: 1
            border.color: Theme.alpha(Theme.amber500, 0.2)
            Icon { anchors.centerIn: parent; name: "shield-alert"; size: 28; color: Theme.amber400 }
        }
        AppText {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            text: qsTr("Access Restricted")
            color: Theme.textOnAccent
            font.pixelSize: Typography.xl2
            font.weight: Typography.bold
        }
        AppText {
            Layout.topMargin: 8
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Administrator role with active admin view mode is required to access the Question Bank.")
            color: Theme.zinc400
            wrapMode: Text.WordWrap
        }
    }

    // ------------------------------------------------------------- Header
    RowLayout {
        visible: page.hasAccess
        Layout.fillWidth: true
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Badge {
                pill: true
                iconName: "shield-alert"
                text: qsTr("Administrator Panel")
                fontWeight: Typography.semibold
                horizontalPadding: 12
                verticalPadding: 5
            }
            AppText {
                Layout.fillWidth: true
                text: qsTr("Manage Question Bank, difficulty parameters, and session telemetry.")
                color: Theme.textOnAccent
                font.pixelSize: page.wide ? Typography.xl3 : Typography.xl2
                font.weight: Typography.bold
                font.letterSpacing: Typography.tracking(Typography.xl3, Typography.trackingTight)
                lineHeight: Typography.leadingSnug
                wrapMode: Text.WordWrap
            }
        }

        TactileButton {
            text: qsTr("Add Question")
            iconLeft: "plus"
            implicitHeight: 42
            glow: true
            onClicked: page.openAddDialog()
        }
    }
    Rectangle {
        visible: page.hasAccess
        Layout.fillWidth: true
        Layout.topMargin: 24
        Layout.bottomMargin: 32
        implicitHeight: 1
        color: Theme.borderGlass
    }

    // ------------------------------------------------------ Track filter
    RowLayout {
        visible: page.hasAccess
        Layout.fillWidth: true
        Layout.bottomMargin: 24
        spacing: 12

        Icon { name: "filter"; size: 16; color: Theme.zinc400 }
        MonoText {
            text: qsTr("Track Filter:")
            caps: true
            font.weight: Typography.semibold
        }
        Flow {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: page.trackFilters

                AbstractButton {
                    id: chip
                    required property var modelData
                    readonly property bool selected: adminVM.directionFilter === modelData.value

                    implicitHeight: 26
                    implicitWidth: chipText.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: adminVM.directionFilter = modelData.value
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    scale: down ? 0.97 : 1.0
                    Behavior on scale { NumberAnimation { duration: Theme.durationFast } }

                    background: Rectangle {
                        radius: Theme.radiusSm
                        color: chip.selected ? Theme.primary : (chip.hovered ? Theme.zinc700 : Theme.zinc800)
                        border.width: chip.visualFocus ? 2 : 0
                        border.color: Theme.focusRing
                        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                    }
                    contentItem: Item {
                        AppText {
                            id: chipText
                            anchors.centerIn: parent
                            text: chip.modelData.label
                            color: chip.selected ? Theme.textOnAccent : Theme.zinc300
                            font.pixelSize: Typography.xs
                            font.weight: Typography.medium
                        }
                    }
                }
            }
        }
    }

    AlertBox {
        visible: page.hasAccess && adminVM.errorMessage !== ""
        Layout.fillWidth: true
        Layout.bottomMargin: 16
        text: adminVM.errorMessage
    }

    // ---------------------------------------------------- Questions table
    Rectangle {
        id: table
        visible: page.hasAccess
        Layout.fillWidth: true
        implicitHeight: tableColumn.implicitHeight
        radius: Theme.radiusLg
        color: Theme.surfaceInput
        border.width: 1
        border.color: Theme.borderGlass

        ColumnLayout {
            id: tableColumn
            width: parent.width
            spacing: 0

            // Loading
            ColumnLayout {
                visible: adminVM.loading
                Layout.fillWidth: true
                Layout.margins: 24
                spacing: 20
                Repeater {
                    model: 3
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 24
                        Skeleton { Layout.preferredWidth: 84; Layout.preferredHeight: 18; radius: 4 }
                        Skeleton { Layout.preferredWidth: 56; Layout.preferredHeight: 14; radius: 4 }
                        Skeleton { Layout.preferredWidth: 160; Layout.preferredHeight: 16; radius: 4 }
                        Skeleton { Layout.fillWidth: true; Layout.preferredHeight: 14; radius: 4 }
                    }
                }
            }

            // Empty
            AppText {
                visible: !adminVM.loading && adminVM.questionCount === 0
                Layout.fillWidth: true
                Layout.margins: 48
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No questions found for the selected filter.")
                color: Theme.zinc400
                wrapMode: Text.WordWrap
            }

            // Column headers
            Item {
                visible: !adminVM.loading && adminVM.questionCount > 0
                Layout.fillWidth: true
                implicitHeight: 41

                Item {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.bottomMargin: 0
                    clip: true
                    Rectangle {
                        width: parent.width
                        height: parent.height + radius
                        radius: Theme.radiusLg - 1
                        color: Theme.alpha(Theme.zinc950, 0.6)
                    }
                }
                Row {
                    x: 24
                    height: parent.height
                    Repeater {
                        model: [qsTr("Track"), qsTr("Level"), qsTr("Topic"), qsTr("Question Text"), qsTr("Status"), qsTr("Actions")]
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

            // Rows (virtualised)
            ListView {
                id: rows
                visible: !adminVM.loading && adminVM.questionCount > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(count, page.maxVisibleRows) * page.rowHeight
                Layout.bottomMargin: 1
                clip: true
                interactive: count > page.maxVisibleRows
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
                    color: rowHover.hovered ? Theme.alpha(Theme.zinc800, 0.4) : "transparent"
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

                        Item {
                            width: page.columnWidth(0, table.width)
                            height: parent.height
                            Badge {
                                anchors.verticalCenter: parent.verticalCenter
                                mono: true
                                caps: true
                                text: row.direction
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
                            elide: Text.ElideRight
                        }
                        AppText {
                            width: page.columnWidth(2, table.width) - 16
                            height: parent.height
                            rightPadding: 16
                            verticalAlignment: Text.AlignVCenter
                            text: row.topic
                            color: Theme.textOnAccent
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
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                ToggleSwitch {
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: row.isActive
                                    onToggled: adminVM.setQuestionActive(row.questionId, checked)
                                }
                                AppText {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: row.isActive ? qsTr("Active") : qsTr("Inactive")
                                    color: row.isActive ? Theme.emerald400 : Theme.textMuted
                                    font.pixelSize: Typography.xs
                                }
                            }
                        }
                        Item {
                            width: page.columnWidth(5, table.width)
                            height: parent.height

                            AbstractButton {
                                id: removeButton
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                implicitWidth: 28
                                implicitHeight: 28
                                hoverEnabled: true
                                onClicked: adminVM.deleteQuestion(row.questionId)
                                HoverHandler { cursorShape: Qt.PointingHandCursor }
                                scale: down ? 0.94 : 1.0
                                ToolTip.visible: hovered
                                ToolTip.delay: 600
                                ToolTip.text: qsTr("Delete")

                                background: Rectangle {
                                    radius: Theme.radiusSm
                                    color: removeButton.hovered ? Theme.alpha(Theme.red500, 0.1) : "transparent"
                                }
                                contentItem: Item {
                                    Icon {
                                        anchors.centerIn: parent
                                        name: "trash-2"
                                        size: 16
                                        color: removeButton.hovered ? Theme.red400 : Theme.textMuted
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Pagination
            RowLayout {
                visible: !adminVM.loading && adminVM.totalPages > 1
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 16
                Layout.preferredHeight: 52
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.leftMargin: -24
                    Layout.rightMargin: -16
                    implicitHeight: 1
                    color: Theme.borderGlass
                    visible: false
                }
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
            text: qsTr("Add New Question to Bank")
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
            text: qsTr("Question Text")
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
