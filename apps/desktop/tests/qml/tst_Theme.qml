import QtQuick
import QtTest
import DevDialogue.App

TestCase {
    id: testCase

    name: "Theme"
    when: windowShown
    visible: true
    width: 320
    height: 200

    function test_paletteTokensAreStable() {
        compare(Theme.backgroundPrimary, "#09090b")
        compare(Theme.backgroundSecondary, "#18181b")
        compare(Theme.textPrimary, "#f4f4f5")
        compare(Theme.accentIndigo, "#6366f1")
        compare(Theme.accentRose, "#f43f5e")
        compare(Theme.accentEmerald, "#10b981")
        compare(Theme.accentAmber, "#f59e0b")
    }

    function test_geometryTokens() {
        compare(Theme.radiusSm, 8)
        compare(Theme.radiusMd, 12)
        compare(Theme.radiusLg, 16)
        compare(Theme.radiusXl, 24)
    }

    function test_geistFontIsLoaded() {
        // The compiled resources are not available in the standalone test
        // runner, so the very same font files are loaded from disk.
        Theme.fontSansSource =
            Qt.resolvedUrl("../../resources/fonts/Geist-VF.ttf")
        Theme.fontMonoSource =
            Qt.resolvedUrl("../../resources/fonts/GeistMono-VF.ttf")

        wait(200)

        verify(Theme.fontSans.indexOf("Geist") >= 0,
               "expected the Geist family, got " + Theme.fontSans)
        verify(Theme.fontMono.indexOf("Geist") >= 0,
               "expected the Geist Mono family, got " + Theme.fontMono)
    }
}
