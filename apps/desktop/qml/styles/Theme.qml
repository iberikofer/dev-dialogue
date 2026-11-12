pragma Singleton
import QtQuick

QtObject {
    id: themeRoot

    // Background Palette (Tailwind Zinc)
    readonly property color backgroundPrimary: "#09090b"    // zinc-950
    readonly property color backgroundSecondary: "#18181b"  // zinc-900
    readonly property color backgroundCard: "#121215"       // zinc-925
    readonly property color backgroundElevated: "#27272a"   // zinc-800

    // Hairline Borders
    readonly property color borderSubtle: "#27272a"         // zinc-800
    readonly property color borderHighlight: "#3f3f46"      // zinc-700
    readonly property color borderGlass: Qt.rgba(1, 1, 1, 0.08)

    // Text Colors
    readonly property color textPrimary: "#f4f4f5"          // zinc-100
    readonly property color textSecondary: "#a1a1aa"        // zinc-400
    readonly property color textMuted: "#71717a"            // zinc-500

    // Accents & Gradients
    readonly property color accentRose: "#f43f5e"           // rose-500
    readonly property color accentIndigo: "#6366f1"         // indigo-500
    readonly property color accentIndigoDeep: "#4f46e5"     // indigo-600
    readonly property color accentTeal: "#14b8a6"           // teal-500
    readonly property color accentEmerald: "#10b981"        // emerald-500
    readonly property color accentAmber: "#f59e0b"          // amber-500

    // Geometry & Radii
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 24

    // Motion
    readonly property int durationFast: 120
    readonly property int durationNormal: 180
    readonly property int durationSlow: 320

    // Typography Fonts. The Geist family ships with the app as a variable
    // TTF; the native families stay as a fallback when the resource fails to
    // load (e.g. a stripped down deployment bundle). The sources are
    // configurable so the QML test suite can load the same files from disk.
    property string fontSansSource:
        "qrc:/DevDialogue/App/resources/fonts/Geist-VF.ttf"
    property string fontMonoSource:
        "qrc:/DevDialogue/App/resources/fonts/GeistMono-VF.ttf"

    readonly property FontLoader geistLoader: FontLoader {
        source: themeRoot.fontSansSource
    }

    readonly property FontLoader geistMonoLoader: FontLoader {
        source: themeRoot.fontMonoSource
    }

    readonly property string fontSans: geistLoader.status === FontLoader.Ready
                                       ? geistLoader.font.family
                                       : "Segoe UI"
    readonly property string fontMono: geistMonoLoader.status === FontLoader.Ready
                                        ? geistMonoLoader.font.family
                                        : "Cascadia Code"
}
