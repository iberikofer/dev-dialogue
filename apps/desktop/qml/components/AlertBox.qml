import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Inline message box for form errors, confirmations and hints.
Rectangle {
    id: root

    property string text
    property string tone: "error"    // error | success | info
    property string title

    readonly property var tones: ({
        error: { base: Theme.red500, deep: Theme.red950, fg: Theme.red200, icon: Theme.red400, name: "circle-alert" },
        success: { base: Theme.emerald500, deep: Theme.emerald950, fg: Theme.emerald300, icon: Theme.emerald400, name: "circle-check" },
        info: { base: Theme.blue500, deep: Theme.blue950, fg: Theme.zinc400, icon: Theme.blue400, name: "info" }
    })
    readonly property var toneColors: tones[tone] !== undefined ? tones[tone] : tones.error

    implicitHeight: row.implicitHeight + 28
    radius: Theme.radiusMd
    color: tone === "info" ? Theme.alpha(Theme.blue500, 0.05) : Theme.alpha(toneColors.deep, 0.3)
    border.width: 1
    border.color: Theme.alpha(toneColors.base, tone === "info" ? 0.2 : 0.3)

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 14
        spacing: 10

        Icon {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
            name: root.toneColors.name
            size: 16
            color: root.toneColors.icon
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            AppText {
                Layout.fillWidth: true
                visible: root.title !== ""
                text: root.title
                color: Theme.blue300
                font.pixelSize: Typography.xs
                font.weight: Typography.semibold
                wrapMode: Text.WordWrap
            }
            AppText {
                Layout.fillWidth: true
                text: root.text
                color: root.toneColors.fg
                font.pixelSize: root.tone === "info" ? Typography.tiny : Typography.xs
                lineHeight: Typography.leadingRelaxed
                wrapMode: Text.WordWrap
            }
        }
    }
}
