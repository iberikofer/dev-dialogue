pragma Singleton
import QtQuick

QtObject {
    // Display
    readonly property int display: 40
    readonly property int h1: 30
    readonly property int h2: 22
    readonly property int h3: 17

    // Body
    readonly property int bodyLarge: 15
    readonly property int body: 14
    readonly property int bodySmall: 13
    readonly property int small: 12
    readonly property int tiny: 11

    // Weights
    readonly property int weightRegular: Font.Normal
    readonly property int weightMedium: Font.Medium
    readonly property int weightSemibold: Font.DemiBold
    readonly property int weightBold: Font.Bold

    // Line heights (as pixel multipliers for Text.lineHeight)
    readonly property real lineTight: 1.2
    readonly property real lineNormal: 1.45
}
