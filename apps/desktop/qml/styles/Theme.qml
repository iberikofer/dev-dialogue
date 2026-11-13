pragma Singleton
import QtQuick

// Design tokens ported from the web client (apps/web/tailwind.config.ts and
// apps/web/src/app/globals.css). The web UI is the visual source of truth.
QtObject {
    id: theme

    // ------------------------------------------------------------------
    // Raw palette. zinc-900 / zinc-850 follow the overrides in the web
    // tailwind.config.ts, the rest are stock Tailwind values.
    // ------------------------------------------------------------------
    readonly property color zinc950: "#09090b"
    readonly property color zinc900: "#121215"
    readonly property color zinc850: "#18181b"
    readonly property color zinc800: "#27272a"
    readonly property color zinc700: "#3f3f46"
    readonly property color zinc600: "#52525b"
    readonly property color zinc500: "#71717a"
    readonly property color zinc400: "#a1a1aa"
    readonly property color zinc300: "#d4d4d8"
    readonly property color zinc200: "#e4e4e7"
    readonly property color zinc100: "#f4f4f5"

    readonly property color blue100: "#dbeafe"
    readonly property color blue200: "#bfdbfe"
    readonly property color blue300: "#93c5fd"
    readonly property color blue400: "#60a5fa"
    readonly property color blue500: "#3b82f6"
    readonly property color blue600: "#2563eb"
    readonly property color blue900: "#1e3a8a"
    readonly property color blue950: "#172554"

    readonly property color emerald300: "#6ee7b7"
    readonly property color emerald400: "#34d399"
    readonly property color emerald500: "#10b981"
    readonly property color emerald600: "#059669"
    readonly property color emerald950: "#022c22"
    readonly property color teal500: "#14b8a6"
    readonly property color teal600: "#0d9488"

    readonly property color amber200: "#fde68a"
    readonly property color amber300: "#fcd34d"
    readonly property color amber400: "#fbbf24"
    readonly property color amber500: "#f59e0b"
    readonly property color amber950: "#451a03"

    readonly property color rose100: "#ffe4e6"
    readonly property color rose200: "#fecdd3"
    readonly property color rose300: "#fda4af"
    readonly property color rose400: "#fb7185"
    readonly property color rose500: "#f43f5e"
    readonly property color rose950: "#4c0519"
    readonly property color pink500: "#ec4899"

    readonly property color red200: "#fecaca"
    readonly property color red300: "#fca5a5"
    readonly property color red400: "#f87171"
    readonly property color red500: "#ef4444"
    readonly property color red950: "#450a0a"

    readonly property color indigo400: "#818cf8"
    readonly property color indigo500: "#6366f1"
    readonly property color indigo600: "#4f46e5"
    readonly property color sky300: "#7dd3fc"

    // ------------------------------------------------------------------
    // Semantic tokens
    // ------------------------------------------------------------------
    readonly property color backgroundPrimary: zinc950
    readonly property color backgroundSecondary: zinc850
    readonly property color backgroundCard: zinc900
    readonly property color backgroundElevated: zinc800

    // Translucent surfaces used all over the web UI (bg-zinc-900/40 etc.)
    readonly property color surfaceCard: alpha(zinc900, 0.4)
    readonly property color surfaceCardHover: alpha(zinc900, 0.7)
    readonly property color surfaceInput: alpha(zinc900, 0.6)
    readonly property color surfaceControl: alpha(zinc900, 0.8)
    readonly property color surfaceOverlay: Qt.rgba(0, 0, 0, 0.8)

    readonly property color borderSubtle: zinc800
    readonly property color borderHighlight: zinc700
    readonly property color borderGlass: Qt.rgba(1, 1, 1, 0.08)
    readonly property color borderFaint: Qt.rgba(1, 1, 1, 0.06)
    readonly property color borderStrong: Qt.rgba(1, 1, 1, 0.12)
    readonly property color borderHover: Qt.rgba(1, 1, 1, 0.22)

    readonly property color textPrimary: zinc100
    readonly property color textBody: zinc300
    readonly property color textSecondary: zinc400
    readonly property color textMuted: zinc500
    readonly property color textOnAccent: "#ffffff"

    // Primary action colour of the web client (blue-600 / blue-500 on hover).
    readonly property color primary: blue600
    readonly property color primaryHover: blue500
    readonly property color primaryBorder: alpha(blue400, 0.3)
    readonly property color focusRing: alpha(blue500, 0.5)

    readonly property color accentBlue: blue500
    readonly property color accentRose: rose500
    readonly property color accentIndigo: indigo500
    readonly property color accentTeal: teal500
    readonly property color accentEmerald: emerald500
    readonly property color accentAmber: amber500

    readonly property color success: emerald500
    readonly property color warning: amber500
    readonly property color danger: red500

    // ------------------------------------------------------------------
    // Geometry
    // ------------------------------------------------------------------
    readonly property int radiusXs: 6      // rounded-md
    readonly property int radiusSm: 8      // rounded-lg
    readonly property int radiusMd: 12     // rounded-xl
    readonly property int radiusLg: 16     // rounded-2xl
    readonly property int radiusXl: 24     // rounded-3xl

    readonly property int sidebarExpanded: 232
    readonly property int sidebarCollapsed: 72
    readonly property int windowGap: 12
    readonly property int pageGutter: 32
    readonly property int contentWidthWide: 1280    // max-w-7xl
    readonly property int contentWidthDefault: 1152 // max-w-6xl
    readonly property int contentWidthMedium: 1024  // max-w-5xl
    readonly property int contentWidthNarrow: 896   // max-w-4xl

    // ------------------------------------------------------------------
    // Motion (globals.css: --ease-spring, .btn-tactile, .card-interactive)
    // ------------------------------------------------------------------
    readonly property int durationFast: 160
    readonly property int durationCard: 240
    readonly property int durationEnter: 350
    readonly property var easeSpring: [0.16, 1.0, 0.3, 1.0, 1.0, 1.0]
    readonly property var easeSnappy: [0.2, 0.0, 0.0, 1.0, 1.0, 1.0]

    // ------------------------------------------------------------------
    // Typography. Geist ships with the application; the system UI font is
    // only used if the bundled files fail to load.
    // ------------------------------------------------------------------
    readonly property FontLoader sansLoader: FontLoader {
        source: Qt.resolvedUrl("../../resources/fonts/Geist-Variable.ttf")
    }
    readonly property FontLoader monoLoader: FontLoader {
        source: Qt.resolvedUrl("../../resources/fonts/GeistMono-Variable.ttf")
    }
    readonly property string fontSans: sansLoader.status === FontLoader.Ready ? sansLoader.name : "Segoe UI"
    readonly property string fontMono: monoLoader.status === FontLoader.Ready ? monoLoader.name : "Consolas"

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------
    function alpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }

    // Report score colouring, same thresholds as the web PerformanceReport.
    function scoreColor(score) {
        if (score >= 80)
            return emerald400
        if (score >= 60)
            return blue400
        return amber400
    }
}
