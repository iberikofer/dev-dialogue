pragma Singleton
import QtQuick

// Type scale of the web client (Tailwind font sizes in px).
QtObject {
    readonly property int micro: 10     // text-[10px]
    readonly property int tiny: 11      // text-[11px]
    readonly property int xs: 12        // text-xs
    readonly property int sm: 14        // text-sm
    readonly property int base: 16      // text-base
    readonly property int lg: 18        // text-lg
    readonly property int xl: 20        // text-xl
    readonly property int xl2: 24       // text-2xl
    readonly property int xl3: 30       // text-3xl
    readonly property int xl4: 36       // text-4xl
    readonly property int xl5: 48       // text-5xl
    readonly property int xl6: 60       // text-6xl

    readonly property int regular: Font.Normal
    readonly property int medium: Font.Medium
    readonly property int semibold: Font.DemiBold
    readonly property int bold: Font.Bold
    readonly property int extrabold: Font.ExtraBold
    readonly property int black: Font.Black

    // Line heights (Tailwind leading-*)
    readonly property real leadingTight: 1.08
    readonly property real leadingSnug: 1.3
    readonly property real leadingNormal: 1.5
    readonly property real leadingRelaxed: 1.625

    // Letter spacing in em, converted to pixels for a given font size.
    readonly property real trackingBody: -0.011     // body { letter-spacing }
    readonly property real trackingTight: -0.025    // tracking-tight
    readonly property real trackingTighter: -0.035  // hero headline
    readonly property real trackingWider: 0.05      // tracking-wider

    function tracking(pixelSize, em) {
        return pixelSize * em
    }
}
