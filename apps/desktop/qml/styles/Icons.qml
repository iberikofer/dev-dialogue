pragma Singleton
import QtQuick

// Resolves icon names to the bundled SVG files (resources/icons).
// The set mirrors the lucide icons used by the web client, plus the few
// custom glyphs defined inline in the web sources.
QtObject {
    readonly property string basePath: Qt.resolvedUrl("../../resources/icons/")

    function source(name) {
        return name ? basePath + name + ".svg" : ""
    }
}
