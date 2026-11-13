import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Button with the tactile feedback of the web client (.btn-tactile):
// lifts 1px on hover, compresses to 97% on press, 160ms spring easing.
//
// Variants follow the button styles used across the web UI:
//   primary   - solid blue call to action
//   secondary - bordered dark button
//   ghost     - text-only button with a hover wash
//   success   - emerald > teal gradient ("Finish interview")
//   rose      - solid rose (chat send button)
//   soft      - tinted blue button that fills on hover ("Link")
//   danger    - tinted red button ("Unlink")
Button {
    id: control

    property string variant: "primary"
    property string iconLeft
    property string iconRight
    property real iconSize: 16
    property bool busy: false
    property bool glow: false
    property int buttonRadius: Theme.radiusMd
    property int fontSize: Typography.sm
    property int fontWeight: Typography.semibold
    // Optional gradient override (API from the migration spec). When both
    // are set they replace the variant fill.
    property color bgGradientStart: "transparent"
    property color bgGradientEnd: "transparent"
    readonly property bool hasCustomGradient: bgGradientStart.a > 0 && bgGradientEnd.a > 0

    readonly property var palette_: ({
        primary: {
            bg: Theme.primary, bgHover: Theme.primaryHover,
            border: Theme.primaryBorder, borderHover: Theme.primaryBorder,
            fg: Theme.textOnAccent, fgHover: Theme.textOnAccent, glow: Theme.primary
        },
        secondary: {
            bg: Theme.surfaceControl, bgHover: Theme.zinc800,
            border: Theme.borderStrong, borderHover: Theme.borderHover,
            fg: Theme.zinc200, fgHover: Theme.textOnAccent, glow: "#000000"
        },
        ghost: {
            bg: "transparent", bgHover: Qt.rgba(1, 1, 1, 0.06),
            border: "transparent", borderHover: "transparent",
            fg: Theme.zinc300, fgHover: Theme.textOnAccent, glow: "#000000"
        },
        success: {
            bg: Theme.emerald600, bgHover: Theme.emerald500,
            border: "transparent", borderHover: "transparent",
            fg: Theme.textOnAccent, fgHover: Theme.textOnAccent, glow: Theme.emerald600
        },
        rose: {
            bg: Theme.rose500, bgHover: Theme.rose400,
            border: "transparent", borderHover: "transparent",
            fg: Theme.textOnAccent, fgHover: Theme.textOnAccent, glow: Theme.rose500
        },
        soft: {
            bg: Theme.alpha(Theme.blue600, 0.2), bgHover: Theme.blue600,
            border: Theme.alpha(Theme.blue500, 0.3), borderHover: Theme.alpha(Theme.blue500, 0.3),
            fg: Theme.blue300, fgHover: Theme.textOnAccent, glow: Theme.primary
        },
        danger: {
            bg: Theme.alpha(Theme.red500, 0.1), bgHover: Theme.alpha(Theme.red500, 0.2),
            border: Theme.alpha(Theme.red500, 0.25), borderHover: Theme.alpha(Theme.red500, 0.4),
            fg: Theme.red300, fgHover: Theme.red300, glow: Theme.red500
        }
    })
    readonly property var colors: palette_[variant] !== undefined ? palette_[variant] : palette_.primary
    readonly property color foreground: hovered && enabled ? colors.fgHover : colors.fg

    implicitHeight: 40
    implicitWidth: Math.max(implicitHeight, implicitContentWidth + leftPadding + rightPadding)
    leftPadding: 20
    rightPadding: 20
    topPadding: 0
    bottomPadding: 0
    spacing: 8
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.5

    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    transform: [
        Translate {
            y: control.hovered && control.enabled && !control.down ? -1 : 0
            Behavior on y {
                NumberAnimation { duration: Theme.durationFast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
            }
        },
        Scale {
            origin.x: control.width / 2
            origin.y: control.height / 2
            xScale: control.down ? 0.97 : 1.0
            yScale: xScale
            Behavior on xScale {
                NumberAnimation { duration: Theme.durationFast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring }
            }
        }
    ]

    HoverHandler {
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: control.spacing
            visible: !control.busy

            Icon {
                visible: control.iconLeft !== ""
                name: control.iconLeft
                size: control.iconSize
                color: control.foreground
            }
            AppText {
                visible: control.text !== ""
                text: control.text
                color: control.foreground
                font.pixelSize: control.fontSize
                font.weight: control.fontWeight
                Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            }
            Icon {
                visible: control.iconRight !== ""
                name: control.iconRight
                size: control.iconSize
                color: control.foreground
            }
        }

        Spinner {
            anchors.centerIn: parent
            visible: control.busy
            color: control.foreground
            size: 16
        }
    }

    background: Item {
        implicitHeight: 40

        SoftGlow {
            anchors.fill: parent
            visible: control.glow && control.enabled
            color: control.colors.glow
            radius: control.buttonRadius
            blur: 18
            offsetY: 8
            strength: 0.3
        }

        Rectangle {
            id: fill
            anchors.fill: parent
            radius: control.buttonRadius
            color: control.hovered && control.enabled ? control.colors.bgHover : control.colors.bg
            border.width: 1
            border.color: control.hovered && control.enabled ? control.colors.borderHover : control.colors.border
            visible: !control.hasCustomGradient && control.variant !== "success"
            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
        }

        // Gradient fills: the success variant and the spec's custom gradient.
        Rectangle {
            anchors.fill: parent
            radius: control.buttonRadius
            visible: control.hasCustomGradient || control.variant === "success"
            border.width: control.hasCustomGradient ? 1 : 0
            border.color: Qt.rgba(1, 1, 1, 0.15)
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: control.hasCustomGradient
                        ? (control.down ? Qt.darker(control.bgGradientStart, 1.15) : control.bgGradientStart)
                        : (control.hovered && control.enabled ? Theme.emerald500 : Theme.emerald600)
                }
                GradientStop {
                    position: 1.0
                    color: control.hasCustomGradient
                        ? (control.down ? Qt.darker(control.bgGradientEnd, 1.15) : control.bgGradientEnd)
                        : (control.hovered && control.enabled ? Theme.teal500 : Theme.teal600)
                }
            }
        }

        // Keyboard focus ring (globals.css :focus-visible)
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: control.buttonRadius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.focusRing
            visible: control.visualFocus
        }
    }
}
