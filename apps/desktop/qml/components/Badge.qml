import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Small label chip: status badges, track tags, pill headers.
// `tone` picks one of the tinted colour sets used on the web
// (bg-<tone>-500/10 + border-<tone>-500/20..30 + text-<tone>-300/400).
Rectangle {
    id: badge

    property string text
    property string tone: "blue"       // blue | emerald | amber | rose | red | neutral
    property bool mono: false
    property bool caps: false
    property bool pill: false
    property string iconName
    property real iconSize: 14
    property bool dot: false
    property int fontSize: Typography.xs
    property int fontWeight: Typography.medium
    property int horizontalPadding: 8
    property int verticalPadding: 2
    property real fillOpacity: 0.1
    property real strokeOpacity: 0.2

    readonly property var tones: ({
        blue: { base: Theme.blue500, fg: Theme.blue400 },
        emerald: { base: Theme.emerald500, fg: Theme.emerald400 },
        amber: { base: Theme.amber500, fg: Theme.amber400 },
        rose: { base: Theme.rose500, fg: Theme.rose300 },
        red: { base: Theme.red500, fg: Theme.red300 },
        neutral: { base: Theme.zinc800, fg: Theme.zinc300 }
    })
    readonly property var toneColors: tones[tone] !== undefined ? tones[tone] : tones.blue
    property color foreground: toneColors.fg

    implicitWidth: row.implicitWidth + horizontalPadding * 2
    implicitHeight: row.implicitHeight + verticalPadding * 2
    radius: pill ? height / 2 : Theme.radiusXs
    color: tone === "neutral" ? Theme.alpha(Theme.zinc800, 0.8) : Theme.alpha(toneColors.base, fillOpacity)
    border.width: 1
    border.color: tone === "neutral" ? Theme.borderFaint : Theme.alpha(toneColors.base, strokeOpacity)

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 6

        Rectangle {
            visible: badge.dot
            implicitWidth: 6
            implicitHeight: 6
            radius: 3
            color: badge.foreground
        }
        Icon {
            visible: badge.iconName !== ""
            name: badge.iconName
            size: badge.iconSize
            color: badge.foreground
        }
        Text {
            text: badge.text
            color: badge.foreground
            font.family: badge.mono ? Theme.fontMono : Theme.fontSans
            font.pixelSize: badge.fontSize
            font.weight: badge.fontWeight
            font.capitalization: badge.caps ? Font.AllUppercase : Font.MixedCase
            textFormat: Text.PlainText
        }
    }
}
