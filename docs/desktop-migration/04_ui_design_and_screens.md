# Document 04: UI Design System & Screen Porting

This document specifies the QML implementation of the **Dev Dialogue** user interface, porting the Tailwind CSS dark-mode design system, Geist typography, and all application screens into GPU-accelerated Qt Quick components.

---

## 1. Design System Tokens (`qml/styles/Theme.qml`)

The visual identity follows the Anthropic and David Sanders high-craft UI principles: deep zinc backgrounds, 1px subtle hairline borders, atmospheric radial glows, and tactile button feedback.

```qml
pragma Singleton
import QtQuick

QtObject {
    // Background Palette (Tailwind Zinc)
    readonly property color backgroundPrimary: "#09090b"    // zinc-950
    readonly property color backgroundSecondary: "#18181b"  // zinc-900
    readonly property color backgroundCard: "#121215"       // zinc-925
    readonly property color backgroundElevated: "#27272a"   // zinc-800

    // Hairline Borders
    readonly property color borderSubtle: "#27272a"         // zinc-800
    readonly property color borderHighlight: "#3f3f46"      // zinc-700
    readonly property color borderGlass: Qt.rgba(1, 1, 1, 0.08)

    // Text Colors
    readonly property color textPrimary: "#f4f4f5"          // zinc-100
    readonly property color textSecondary: "#a1a1aa"        // zinc-400
    readonly property color textMuted: "#71717a"            // zinc-500

    // Accents & Gradients
    readonly property color accentRose: "#f43f5e"           // rose-500
    readonly property color accentIndigo: "#6366f1"         // indigo-500
    readonly property color accentTeal: "#14b8a6"           // teal-500
    readonly property color accentEmerald: "#10b981"        // emerald-500
    readonly property color accentAmber: "#f59e0b"          // amber-500

    // Geometry & Radii
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 24

    // Typography Fonts
    readonly property string fontSans: "Geist"
    readonly property string fontMono: "Geist Mono"
}
```

---

## 2. Core Reusable Component: Tactile Button (`TactileButton.qml`)

Following Emil Kowalski's motion heuristics, this button provides spring-physics scale feedback, subtle hover highlights, and gradient fills:

```qml
import QtQuick
import QtQuick.Controls
import DevDialogue.App

Button {
    id: control
    property color bgGradientStart: Theme.accentIndigo
    property color bgGradientEnd: "#4f46e5"
    property int buttonRadius: Theme.radiusMd

    implicitHeight: 40
    implicitWidth: Math.max(120, contentItem.implicitWidth + 32)

    contentItem: Text {
        text: control.text
        font.family: Theme.fontSans
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
        radius: control.buttonRadius
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: control.down ? Qt.darker(control.bgGradientStart, 1.15) : control.bgGradientStart }
            GradientStop { position: 1.0; color: control.down ? Qt.darker(control.bgGradientEnd, 1.15) : control.bgGradientEnd }
        }
        border.color: Qt.rgba(1, 1, 1, 0.15)
        border.width: 1

        scale: control.down ? 0.97 : (control.hovered ? 1.02 : 1.0)
        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutBack }
        }
    }
}
```

---

## 3. Screen 1: Interactive Chat Room (`ChatRoomView.qml`)

The chat room replicates `apps/web/src/components/ChatRoom.tsx`, offering an AI dialogue interface with auto-scrolling virtualized delegates, question progress metrics, and timer updates:

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DevDialogue.App

Item {
    id: chatView

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        // Top Session Bar
        Rectangle {
            Layout.fillWidth: true
            height: 56
            radius: Theme.radiusMd
            color: Theme.backgroundSecondary
            border.color: Theme.borderGlass
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: qsTr("Interview Session")
                    font.family: Theme.fontSans
                    font.weight: Font.Bold
                    font.pixelSize: 15
                    color: Theme.textPrimary
                }

                Rectangle {
                    radius: 6
                    color: Qt.rgba(0.95, 0.25, 0.37, 0.15)
                    border.color: Qt.rgba(0.95, 0.25, 0.37, 0.3)
                    implicitWidth: trackText.implicitWidth + 12
                    implicitHeight: 22

                    Text {
                        id: trackText
                        anchors.centerIn: parent
                        text: chatVM.currentDirection.toUpperCase()
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        color: Theme.accentRose
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: qsTr("Answers: ") + chatVM.candidateAnswersCount
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                TactileButton {
                    text: qsTr("Complete Interview")
                    bgGradientStart: Theme.accentEmerald
                    bgGradientEnd: Theme.accentTeal
                    enabled: chatVM.candidateAnswersCount > 0 && !chatVM.isBusy
                    onClicked: chatVM.completeInterview()
                }
            }
        }

        // Virtualized Scrollable Message Area
        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 16
            model: chatVM.messagesModel

            delegate: MessageBubble {
                width: messageList.width
                senderRole: model.role
                content: model.content
                timestamp: model.timestamp
            }

            onCountChanged: {
                Qt.callLater(() => messageList.positionViewAtEnd());
            }
        }

        // Input Box Bar
        Rectangle {
            Layout.fillWidth: true
            height: 72
            radius: Theme.radiusLg
            color: Theme.backgroundSecondary
            border.color: Theme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        id: inputArea
                        placeholderText: qsTr("Type your answer... (Enter to send, Shift+Enter for newline)")
                        placeholderTextColor: Theme.textMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 14
                        color: Theme.textPrimary
                        wrapMode: Text.Wrap
                        background: null

                        Keys.onReturnPressed: (event) => {
                            if (!(event.modifiers & Qt.ShiftModifier)) {
                                event.accepted = true;
                                if (text.trim().length > 0 && !chatVM.isBusy) {
                                    chatVM.sendAnswer(text.trim());
                                    text = "";
                                }
                            }
                        }
                    }
                }

                TactileButton {
                    text: qsTr("Send")
                    implicitWidth: 80
                    enabled: inputArea.text.trim().length > 0 && !chatVM.isBusy
                    onClicked: {
                        chatVM.sendAnswer(inputArea.text.trim());
                        inputArea.text = "";
                    }
                }
            }
        }
    }
}
```

---

## 4. Screen 2: Performance Scorecard & Report (`ReportView.qml`)

Replicating `PerformanceReport.tsx`, this screen renders:
1. **Overall Score Badge:** Animated circular or numeric dial (>=85: Emerald, 70–84: Amber, <70: Rose).
2. **Executive Summary:** Synthesized AI feedback narrative.
3. **Strengths & Weaknesses Grids:** Two-column card view with status indicators.
4. **Curated Recommendations:** Recommended study topics with clickable hyperlinks opening in the user's default browser via `Qt.openUrlExternally(url)`.
5. **Question Breakdown:** Expandable per-question review showing score out of 100 and AI evaluator commentary.

---

## 5. Screen 3: Candidate Dashboard (`DashboardView.qml`)

Port of `DashboardView.tsx`:
1. **Welcome Header:** Personalized candidate salutation and "Start New Interview" quick CTA.
2. **KPI Statistics Grid (3 Cards):**
   - **Total Interviews Completed** (Numeric counter + icon).
   - **Average Overall Score** (Percentage badge).
   - **Daily Limit Tracker** (e.g., 2 / 10 interviews completed today with circular progress ring).
3. **Session History Table:** Virtualized table showing Track, Level, Date, Status badge (Active/Completed), Score, and a button to review or resume.

---

## 6. Screen 4: Interview Setup (`SetupView.qml`)

Port of `InterviewSetup.tsx`:
1. **Track Selector Cards:** Grid of 5 selectable tracks (Frontend, Backend, DevOps, QA, Product Management) with SVG icons and active border glow.
2. **Level Selector:** Segmented toggle (Trainee, Junior, Middle).
3. **Language Selector:** Segmented toggle (Українська / English).
4. **Action Button:** "Start Interview Session" transitioning immediately into `ChatRoomView`.

---

## 7. Screen 5: Admin Question Bank Management (`AdminView.qml`)

Port of `AdminPanel.tsx`:
- Gated behind `authVM.isAdmin && authVM.isAdminViewActive`.
- Filter bar by track (All, Frontend, Backend, etc.).
- Paginated table showing Topic, Question text snippet, Level badge, Active/Inactive switch.
- "Add Question" dialog modal with form validation.

