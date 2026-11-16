pragma Singleton
import QtQuick

// Thin UI-side view of the interface language.
//
// All user-facing strings in the QML layer go through qsTr() with English
// source text. The translation catalogs and the manager that installs them
// are owned by the i18n module; this singleton only reports which language
// is actually on screen and asks the engine to switch.
QtObject {
    // A catalog translates this marker into its own language code, so the UI
    // can tell which language is really displayed (not merely requested).
    //: Language code of this translation catalog, e.g. "uk". Do not translate literally.
    readonly property string language: qsTr("en", "ui-language-code")
    readonly property bool isUkrainian: language === "uk"

    function setLanguage(code) {
        Qt.uiLanguage = code
    }

    function toggle() {
        setLanguage(isUkrainian ? "en" : "uk")
    }
}
