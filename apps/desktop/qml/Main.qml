import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DevDialogue.App

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 1200
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    title: qsTr("Dev Dialogue — AI Interview Simulator (C++ Desktop)")
    color: Theme.backgroundPrimary

    Navbar {
        id: navbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 10
    }

    StackLayout {
        id: viewStack
        anchors.top: navbar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: navVM.currentViewIndex

        LandingView {}

        // Placeholder for Dashboard
        Rectangle {
            color: Theme.backgroundPrimary
            Text {
                anchors.centerIn: parent
                text: "Dashboard View (Milestone 4)"
                color: Theme.textSecondary
                font.pixelSize: 18
                font.family: Theme.fontSans
            }
        }

        // Placeholder for Setup
        Rectangle {
            color: Theme.backgroundPrimary
            Text {
                anchors.centerIn: parent
                text: "Interview Setup View (Milestone 5)"
                color: Theme.textSecondary
                font.pixelSize: 18
                font.family: Theme.fontSans
            }
        }

        // Placeholder for Chat
        Rectangle {
            color: Theme.backgroundPrimary
            Text {
                anchors.centerIn: parent
                text: "Chat Room View (Milestone 5)"
                color: Theme.textSecondary
                font.pixelSize: 18
                font.family: Theme.fontSans
            }
        }

        // Placeholder for Report
        Rectangle {
            color: Theme.backgroundPrimary
            Text {
                anchors.centerIn: parent
                text: "Scorecard Report View (Milestone 6)"
                color: Theme.textSecondary
                font.pixelSize: 18
                font.family: Theme.fontSans
            }
        }

        // Placeholder for Admin
        Rectangle {
            color: Theme.backgroundPrimary
            Text {
                anchors.centerIn: parent
                text: "Admin Question Bank View (Milestone 7)"
                color: Theme.textSecondary
                font.pixelSize: 18
                font.family: Theme.fontSans
            }
        }
    }
}

