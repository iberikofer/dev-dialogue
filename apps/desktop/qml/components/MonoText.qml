import QtQuick
import DevDialogue.App

// Monospace text (Geist Mono) for badges, counters, timestamps and labels.
Text {
    // Section labels on the web use uppercase with wide tracking.
    property bool caps: false

    color: Theme.textSecondary
    font.family: Theme.fontMono
    font.pixelSize: Typography.xs
    font.capitalization: caps ? Font.AllUppercase : Font.MixedCase
    font.letterSpacing: caps ? 0.6 : 0
    textFormat: Text.PlainText
}
