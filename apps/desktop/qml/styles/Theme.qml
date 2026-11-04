pragma Singleton
import QtQuick

QtObject {
    // Dark Zinc Palette (Tailwind Zinc)
    readonly property color backgroundPrimary: "#09090b"    // zinc-950
    readonly property color backgroundSecondary: "#18181b"  // zinc-900
    readonly property color backgroundCard: "#121215"       // zinc-925
    readonly property color backgroundElevated: "#27272a"   // zinc-800

    // Borders
    readonly property color borderSubtle: "#27272a"         // zinc-800
    readonly property color borderHighlight: "#3f3f46"      // zinc-700
    readonly property color borderGlass: Qt.rgba(1, 1, 1, 0.08)

    // Typography Colors
    readonly property color textPrimary: "#f4f4f5"          // zinc-100
    readonly property color textSecondary: "#a1a1aa"        // zinc-400
    readonly property color textMuted: "#71717a"            // zinc-500

    // Accent Colors
    readonly property color accentRose: "#f43f5e"           // rose-500
    readonly property color accentIndigo: "#6366f1"         // indigo-500
    readonly property color accentTeal: "#14b8a6"           // teal-500
    readonly property color accentEmerald: "#10b981"        // emerald-500
    readonly property color accentAmber: "#f59e0b"          // amber-500

    // Radii
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 24

    // Font Defaults
    readonly property string fontSans: "Segoe UI, Inter, sans-serif"
    readonly property string fontMono: "Cascadia Code, Consolas, monospace"
}

