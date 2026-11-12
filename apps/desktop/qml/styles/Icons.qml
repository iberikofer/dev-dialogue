pragma Singleton
import QtQuick

QtObject {
    readonly property string base: "qrc:/DevDialogue/App/resources/icons/"

    // Tracks
    readonly property string frontend: base + "frontend.svg"
    readonly property string backend: base + "backend.svg"
    readonly property string devops: base + "devops.svg"
    readonly property string qa: base + "qa.svg"
    readonly property string pm: base + "pm.svg"

    // Interface
    readonly property string bot: base + "bot.svg"
    readonly property string user: base + "user.svg"
    readonly property string sparkles: base + "sparkles.svg"
    readonly property string shield: base + "shield.svg"
    readonly property string warning: base + "warning.svg"
    readonly property string refresh: base + "refresh.svg"
    readonly property string dashboard: base + "dashboard.svg"
    readonly property string chart: base + "chart.svg"
    readonly property string plus: base + "plus.svg"
    readonly property string logout: base + "logout.svg"
}
