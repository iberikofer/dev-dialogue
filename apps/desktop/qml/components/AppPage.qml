import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import DevDialogue.App

// Container for one screen of the app: a header with the title, optional
// badge and actions, and a body that fills the rest of the window. With
// `scrollable` the body scrolls; otherwise children size themselves to the
// available height (tables and chats manage their own scrolling).
Item {
    id: page

    property string title
    property string subtitle
    property string badgeText          // e.g. "Demo data"
    property alias actions: actionsRow.data
    property alias headerExtra: extraRow.data
    default property alias content: body.data
    property bool scrollable: false
    property int bodySpacing: 20
    property int bodyPadding: 28
    // Set by the owner (StackLayout.isCurrentItem) to replay the entrance.
    property bool active: true
    readonly property real bodyWidth: body.width

    function scrollToTop() {
        if (scroller.visible)
            scroller.contentY = 0
    }

    onActiveChanged: {
        if (active) {
            scrollToTop()
            entrance.restart()
        }
    }

    // ------------------------------------------------------------- Header
    Item {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 84

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: page.bodyPadding
            anchors.rightMargin: page.bodyPadding
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    AppText {
                        Layout.maximumWidth: header.width * 0.6
                        text: page.title
                        font.pixelSize: Typography.xl2
                        font.weight: Typography.bold
                        font.letterSpacing: Typography.tracking(Typography.xl2, Typography.trackingTight)
                        elide: Text.ElideRight
                    }
                    Badge {
                        visible: page.badgeText !== ""
                        tone: "amber"
                        pill: true
                        dot: true
                        text: page.badgeText
                        fontSize: Typography.tiny
                    }
                    RowLayout {
                        id: extraRow
                        spacing: 8
                    }
                }
                AppText {
                    Layout.fillWidth: true
                    visible: page.subtitle !== ""
                    text: page.subtitle
                    color: Theme.zinc400
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                id: actionsRow
                spacing: 10
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.borderFaint
        }
    }

    // --------------------------------------------------------------- Body
    Item {
        id: bodyArea
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Flickable {
            id: scroller
            anchors.fill: parent
            visible: page.scrollable
            contentWidth: width
            contentHeight: page.scrollable ? body.implicitHeight + page.bodyPadding * 2 : height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ThemedScrollBar {}
        }

        ColumnLayout {
            id: body
            parent: page.scrollable ? scroller.contentItem : bodyArea
            x: page.bodyPadding
            y: page.bodyPadding + entranceOffset
            width: (page.scrollable ? scroller.width : bodyArea.width) - page.bodyPadding * 2
            height: page.scrollable ? implicitHeight : bodyArea.height - page.bodyPadding * 2
            spacing: page.bodySpacing
            property real entranceOffset: 0
        }
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
