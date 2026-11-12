import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Scrollable page shell shared by the full-page views: a centred content
// column with a maximum width, the themed scrollbar, the footer pinned to
// the bottom of the page, and the fade-in-up entrance of the web client
// whenever the page becomes the current one.
Flickable {
    id: page

    default property alias content: body.data
    // Decorations painted behind the content, spanning the full page width.
    property alias backdrop: backdropLayer.data
    property int maxContentWidth: Theme.contentWidthDefault
    property int horizontalPadding: 16
    property int topPadding: 32
    property int bottomPadding: 32
    property int contentSpacing: 0
    property bool showFooter: true
    // Set by the owning view (StackLayout.isCurrentItem).
    property bool active: true
    readonly property real bodyWidth: body.width

    function scrollToTop() {
        contentY = 0
    }

    contentWidth: width
    contentHeight: Math.max(height, bodyHolder.implicitHeight + (showFooter ? footer.height : 0))
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick
    ScrollBar.vertical: ThemedScrollBar {}

    onActiveChanged: {
        if (active) {
            scrollToTop()
            entrance.restart()
        }
    }

    Item {
        id: backdropLayer
        width: page.width
        height: page.contentHeight
        clip: true
    }

    Item {
        id: bodyHolder
        width: page.width
        implicitHeight: body.implicitHeight + page.topPadding + page.bottomPadding
        height: Math.max(implicitHeight, page.height - (page.showFooter ? footer.height : 0))

        ColumnLayout {
            id: body
            width: Math.min(parent.width - page.horizontalPadding * 2, page.maxContentWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            y: page.topPadding + entranceOffset
            spacing: page.contentSpacing
            property real entranceOffset: 0
        }
    }

    AppFooter {
        id: footer
        visible: page.showFooter
        width: page.width
        y: bodyHolder.height
    }

    ParallelAnimation {
        id: entrance
        NumberAnimation {
            target: body; property: "opacity"; from: 0; to: 1
            duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
        }
        NumberAnimation {
            target: body; property: "entranceOffset"; from: 8; to: 0
            duration: Theme.durationEnter; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.easeSpring
        }
    }
}
