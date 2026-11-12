pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import DevDialogue.App

// Landing page (web app/page.tsx): hero, interactive interview preview,
// metrics strip, career tracks, architecture highlights and the final CTA.
PageScroll {
    id: page

    active: StackLayout.isCurrentItem
    maxContentWidth: Theme.contentWidthWide
    horizontalPadding: width >= 1024 ? 32 : 24
    topPadding: 56
    bottomPadding: 96

    readonly property bool wide: width >= 900

    // Sample dialogue shown in the terminal preview, one entry per track.
    readonly property var previewTracks: [
        {
            id: "frontend", name: "Frontend", role: "React / Next.js", level: "Junior+", score: 94,
            question: qsTr("How does Reconciliation work in React, and why should you avoid using array index as a key for dynamic lists?"),
            answer: qsTr("React relies on Virtual DOM diffing. If you use an array index as a key during element deletion or sorting, component state (input focus, local state) will attach to the wrong DOM node."),
            highlight: qsTr("Deep understanding of fiber architecture and reconciliation algorithm.")
        },
        {
            id: "backend", name: "Backend", role: "C# .NET 9 / EF Core", level: "Junior/Middle", score: 91,
            question: qsTr("Explain the difference between AsNoTracking() and standard queries in Entity Framework Core. When is this critical?"),
            answer: qsTr("AsNoTracking disables the Change Tracker within the DbContext. This saves memory and significantly speeds up read-only queries, but prevents subsequent SaveChanges without manual entity attachment."),
            highlight: qsTr("Accurate evaluation of memory footprints and query optimization scenarios.")
        },
        {
            id: "devops", name: "DevOps", role: "Docker / CI/CD", level: "Junior", score: 96,
            question: qsTr("Why are Multi-stage builds recommended in Dockerfiles for production releases?"),
            answer: qsTr("To decouple the heavy build/compiler SDK from the runtime environment. Only the compiled binaries make it into the final image, shrinking container footprint from 800MB to 90MB while eliminating build-tool security vulnerabilities."),
            highlight: qsTr("Excellent reasoning regarding container security and attack surface reduction.")
        }
    ]
    property string selectedTrackId: "frontend"
    readonly property var activeTrack: {
        for (let i = 0; i < previewTracks.length; ++i)
            if (previewTracks[i].id === selectedTrackId)
                return previewTracks[i]
        return previewTracks[0]
    }

    backdrop: [
        DotGrid { anchors.fill: parent },
        GlowBlob {
            x: parent.width * 0.15 - 140
            y: -190
            width: 830
            height: 660
            color: Theme.blue600
            strength: 0.2
        },
        GlowBlob {
            x: parent.width * 0.9 - 640
            y: -130
            width: 780
            height: 640
            color: Theme.indigo600
            strength: 0.13
        }
    ]

    // ---------------------------------------------------------------- Hero
    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: badgeRow.implicitWidth + 32
        implicitHeight: 26
        radius: 13
        color: Theme.alpha(Theme.blue500, 0.1)
        border.width: 1
        border.color: Theme.alpha(Theme.blue500, badgeHover.hovered ? 0.5 : 0.3)
        HoverHandler { id: badgeHover }

        RowLayout {
            id: badgeRow
            anchors.centerIn: parent
            spacing: 8

            PulseDot { size: 8; color: Theme.blue400; pingColor: Theme.blue400 }
            AppText {
                text: qsTr("VNTU Software Engineering Olympiad 2026")
                color: Theme.blue200
                font.pixelSize: Typography.xs
                font.weight: Typography.medium
            }
            AppText { text: "•"; color: Theme.alpha(Theme.blue400, 0.5); font.pixelSize: Typography.xs }
            MonoText {
                text: qsTr("Live AI Simulator")
                color: Theme.blue300
                font.pixelSize: Typography.tiny
            }
        }
    }

    AppText {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 32
        Layout.fillWidth: true
        Layout.maximumWidth: 768
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Technical Interview Simulator with Adaptive")
        font.pixelSize: page.wide ? Typography.xl6 : Typography.xl4
        font.weight: Typography.extrabold
        font.letterSpacing: Typography.tracking((page.wide ? Typography.xl6 : Typography.xl4), Typography.trackingTighter)
        lineHeight: Typography.leadingTight
        wrapMode: Text.WordWrap
    }
    GradientText {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("AI Interviewer")
        font.pixelSize: page.wide ? Typography.xl6 : Typography.xl4
        font.weight: Typography.extrabold
        font.letterSpacing: Typography.tracking((page.wide ? Typography.xl6 : Typography.xl4), Typography.trackingTighter)
    }

    AppText {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 24
        Layout.fillWidth: true
        Layout.maximumWidth: 672
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("A comprehensive platform for realistic technical interview preparation. AI asks dynamic follow-up questions, probes deep conceptual understanding, and generates in-depth engineering scorecards.")
        color: Theme.alpha(Theme.zinc300, 0.9)
        font.pixelSize: page.wide ? Typography.lg : Typography.base
        lineHeight: Typography.leadingRelaxed
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 36
        spacing: 14

        TactileButton {
            text: qsTr("Start Interview")
            iconLeft: "sparkles"
            iconRight: "arrow-right"
            implicitHeight: 50
            leftPadding: 24
            rightPadding: 24
            spacing: 10
            glow: true
            onClicked: navVM.navigateToSetup()
        }
        TactileButton {
            variant: "secondary"
            text: qsTr("View Dashboard")
            iconRight: "chevron-right"
            fontWeight: Typography.medium
            implicitHeight: 50
            leftPadding: 20
            rightPadding: 20
            onClicked: navVM.navigateToDashboard()
        }
    }

    // ------------------------------------------------- Interview preview
    Item {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 64
        Layout.fillWidth: true
        Layout.maximumWidth: Theme.contentWidthNarrow
        implicitHeight: terminal.implicitHeight

        SoftGlow {
            anchors.fill: terminal
            color: "#000000"
            radius: Theme.radiusLg
            blur: 40
            offsetY: 20
            strength: 0.7
        }

        Rectangle {
            id: terminal
            width: parent.width
            implicitHeight: terminalColumn.implicitHeight
            radius: Theme.radiusLg
            color: Theme.alpha(Theme.zinc950, 0.9)
            border.width: 1
            border.color: terminalHover.hovered ? Theme.alpha(Theme.blue500, 0.3) : Qt.rgba(1, 1, 1, 0.1)
            Behavior on border.color { ColorAnimation { duration: 300 } }
            HoverHandler { id: terminalHover }

            ColumnLayout {
                id: terminalColumn
                width: parent.width
                spacing: 0

                // Window bar with the track tabs
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 53

                    // Tinted bar: rounded on top only, so the fill is a taller
                    // rounded rectangle clipped to the bar height.
                    Item {
                        anchors.fill: parent
                        anchors.margins: 1
                        anchors.bottomMargin: 0
                        clip: true
                        Rectangle {
                            width: parent.width
                            height: parent.height + radius
                            radius: Theme.radiusLg - 1
                            color: Theme.alpha(Theme.zinc900, 0.5)
                        }
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.borderGlass
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 8

                        Rectangle { implicitWidth: 12; implicitHeight: 12; radius: 6; color: Theme.alpha(Theme.rose500, 0.7) }
                        Rectangle { implicitWidth: 12; implicitHeight: 12; radius: 6; color: Theme.alpha(Theme.amber500, 0.7) }
                        Rectangle { implicitWidth: 12; implicitHeight: 12; radius: 6; color: Theme.alpha(Theme.emerald500, 0.7) }
                        MonoText {
                            Layout.leftMargin: 8
                            text: "simulation-session.vntu"
                            color: Theme.zinc300
                        }
                        Item { Layout.fillWidth: true }
                        SegmentedControl {
                            compact: true
                            model: page.previewTracks.map((track) => ({ value: track.id, label: track.name }))
                            currentValue: page.activeTrack.id
                            onActivated: (value) => page.selectedTrackId = value
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 24
                    spacing: 20

                    // Interviewer question
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            implicitWidth: 32
                            implicitHeight: 32
                            radius: Theme.radiusSm
                            color: Theme.alpha(Theme.rose500, 0.1)
                            border.width: 1
                            border.color: Theme.alpha(Theme.rose500, 0.3)
                            Icon { anchors.centerIn: parent; name: "bot"; size: 16; color: Theme.rose300 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            RowLayout {
                                spacing: 8
                                AppText {
                                    text: qsTr("AI Interviewer")
                                    color: Theme.zinc200
                                    font.pixelSize: Typography.xs
                                    font.weight: Typography.semibold
                                }
                                Badge {
                                    tone: "rose"
                                    mono: true
                                    text: page.activeTrack.role + " • " + page.activeTrack.level
                                    fontSize: Typography.micro
                                    fontWeight: Typography.regular
                                    radius: 4
                                    fillOpacity: 0.15
                                    strokeOpacity: 0.3
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: questionText.implicitHeight + 32
                                radius: Theme.radiusMd
                                color: Theme.surfaceInput
                                border.width: 1
                                border.color: Theme.borderGlass
                                AppText {
                                    id: questionText
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    text: page.activeTrack.question
                                    lineHeight: Typography.leadingRelaxed
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Candidate answer
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            implicitWidth: 32
                            implicitHeight: 32
                            radius: Theme.radiusSm
                            color: Theme.zinc900
                            border.width: 1
                            border.color: Theme.borderGlass
                            Icon { anchors.centerIn: parent; name: "user"; size: 16; color: Theme.blue400 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            RowLayout {
                                spacing: 8
                                AppText {
                                    text: qsTr("Candidate (Answer)")
                                    color: Theme.zinc300
                                    font.pixelSize: Typography.xs
                                    font.weight: Typography.semibold
                                }
                                MonoText {
                                    text: qsTr("voice / text")
                                    color: Theme.textMuted
                                    font.pixelSize: Typography.micro
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: answerText.implicitHeight + 32
                                radius: Theme.radiusMd
                                color: Theme.surfaceCard
                                border.width: 1
                                border.color: Theme.borderFaint
                                AppText {
                                    id: answerText
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    text: page.activeTrack.answer
                                    color: Theme.zinc200
                                    lineHeight: Typography.leadingRelaxed
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Instant analysis strip
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: analysisRow.implicitHeight + 32
                        radius: Theme.radiusMd
                        border.width: 1
                        border.color: Theme.alpha(Theme.rose500, 0.3)
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.rose950, 0.3)) }
                            GradientStop { position: 0.5; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.zinc900, 0.5)) }
                            GradientStop { position: 1.0; color: Qt.tint(Theme.zinc950, Theme.alpha(Theme.emerald950, 0.2)) }
                        }

                        RowLayout {
                            id: analysisRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: 16
                            spacing: 12

                            Rectangle {
                                implicitWidth: 40
                                implicitHeight: 40
                                radius: Theme.radiusSm
                                color: Theme.alpha(Theme.rose500, 0.2)
                                border.width: 1
                                border.color: Theme.alpha(Theme.rose500, 0.4)
                                MonoText {
                                    anchors.centerIn: parent
                                    text: page.activeTrack.score
                                    color: Theme.rose300
                                    font.pixelSize: Typography.sm
                                    font.weight: Typography.bold
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                RowLayout {
                                    spacing: 6
                                    Icon { name: "circle-check"; size: 14; color: Theme.rose400 }
                                    AppText {
                                        text: qsTr("Instant Knowledge Depth Analysis")
                                        color: Theme.rose200
                                        font.pixelSize: Typography.xs
                                        font.weight: Typography.medium
                                    }
                                }
                                AppText {
                                    Layout.fillWidth: true
                                    text: page.activeTrack.highlight
                                    color: Theme.alpha(Theme.zinc300, 0.8)
                                    font.pixelSize: Typography.xs
                                    wrapMode: Text.WordWrap
                                }
                            }
                            TactileButton {
                                text: qsTr("Take Full Test")
                                iconRight: "arrow-right"
                                iconSize: 14
                                fontSize: Typography.xs
                                buttonRadius: Theme.radiusSm
                                implicitHeight: 30
                                leftPadding: 14
                                rightPadding: 14
                                spacing: 6
                                onClicked: navVM.navigateToSetup()
                            }
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------------------ Metrics strip
    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 64
        implicitHeight: metricsRow.implicitHeight + 64
        radius: Theme.radiusMd
        color: Theme.alpha(Theme.zinc900, 0.2)

        Rectangle { anchors.top: parent.top; x: 8; width: parent.width - 16; height: 1; color: Theme.borderGlass }
        Rectangle { anchors.bottom: parent.bottom; x: 8; width: parent.width - 16; height: 1; color: Theme.borderGlass }

        RowLayout {
            id: metricsRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 16

            Repeater {
                model: [
                    { value: "5", label: qsTr("Specialized Tracks"), color: Theme.zinc100 },
                    { value: qsTr("3 Levels"), label: "Trainee • Junior • Middle", color: Theme.zinc100 },
                    { value: "< 1.8s", label: qsTr("AI Inference Latency"), color: Theme.blue400 },
                    { value: "100%", label: qsTr("Type-Safe API Contracts"), color: Theme.emerald400 }
                ]

                ColumnLayout {
                    id: metric
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: (metricsRow.width - metricsRow.spacing * 3) / 4
                    spacing: 4

                    MonoText {
                        Layout.alignment: Qt.AlignHCenter
                        text: metric.modelData.value
                        color: metric.modelData.color
                        font.pixelSize: Typography.xl3
                        font.weight: Typography.bold
                        font.letterSpacing: Typography.tracking(Typography.xl3, Typography.trackingTight)
                    }
                    AppText {
                        Layout.alignment: Qt.AlignHCenter
                        text: metric.modelData.label
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                        font.weight: Typography.medium
                    }
                }
            }
        }
    }

    // ------------------------------------------------------ Career tracks
    AppText {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 80
        Layout.fillWidth: true
        Layout.maximumWidth: 672
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Tailored for Real Market Requirements")
        font.pixelSize: Typography.xl3
        font.weight: Typography.bold
        font.letterSpacing: Typography.tracking(Typography.xl3, Typography.trackingTight)
        wrapMode: Text.WordWrap
    }
    AppText {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 8
        Layout.fillWidth: true
        Layout.maximumWidth: 672
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Every track provides a calibrated set of questions spanning core fundamentals to real-world system architecture.")
        color: Theme.zinc400
        wrapMode: Text.WordWrap
    }

    GridLayout {
        id: tracksGrid
        Layout.fillWidth: true
        Layout.topMargin: 48
        columns: page.wide ? 3 : 1
        columnSpacing: 20
        rowSpacing: 20

        Repeater {
            model: [
                {
                    icon: "code-xml", tone: Theme.blue500, toneText: Theme.blue400, toneHover: Theme.blue300,
                    title: qsTr("Frontend Engineering"),
                    desc: qsTr("React, Next.js App Router, Virtual DOM reconciliation, CSS Architecture, SSR/SSG, Web Performance, Zustand, TypeScript."),
                    tags: ["Junior", "Middle"]
                },
                {
                    icon: "database", tone: Theme.blue500, toneText: Theme.blue400, toneHover: Theme.blue300,
                    title: qsTr("Backend & Databases"),
                    desc: qsTr("C# .NET 9, Node.js, PostgreSQL, EF Core, REST API design, JWT auth, database indexes, transactions, caching."),
                    tags: ["Trainee", "Junior", "Middle"]
                },
                {
                    icon: "server", tone: Theme.emerald500, toneText: Theme.emerald400, toneHover: Theme.emerald300,
                    title: qsTr("DevOps & Infrastructure"),
                    desc: qsTr("Docker containerization, Docker Compose, CI/CD pipelines, Render, Vercel, environment variables, and telemetry."),
                    tags: ["Junior", "Docker"]
                }
            ]

            DarkCard {
                id: trackCard
                required property var modelData

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: (tracksGrid.width - tracksGrid.columnSpacing * (tracksGrid.columns - 1)) / tracksGrid.columns
                interactive: true
                padding: 24
                hoverFillColor: Theme.surfaceCardHover
                hoverStrokeColor: Theme.alpha(modelData.tone, 0.4)

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 0

                    Rectangle {
                        implicitWidth: 40
                        implicitHeight: 40
                        radius: Theme.radiusMd
                        color: Theme.alpha(trackCard.modelData.tone, 0.1)
                        border.width: 1
                        border.color: Theme.alpha(trackCard.modelData.tone, 0.3)
                        scale: trackCard.hovered ? 1.05 : 1.0
                        Behavior on scale { NumberAnimation { duration: 200 } }
                        Icon { anchors.centerIn: parent; name: trackCard.modelData.icon; size: 20; color: trackCard.modelData.toneText }
                    }
                    AppText {
                        Layout.topMargin: 16
                        Layout.fillWidth: true
                        text: trackCard.modelData.title
                        color: trackCard.hovered ? trackCard.modelData.toneHover : Theme.zinc100
                        font.pixelSize: Typography.base
                        font.weight: Typography.semibold
                        wrapMode: Text.WordWrap
                        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
                    }
                    AppText {
                        Layout.topMargin: 8
                        Layout.fillWidth: true
                        text: trackCard.modelData.desc
                        color: Theme.zinc400
                        font.pixelSize: Typography.xs
                        lineHeight: Typography.leadingRelaxed
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Layout.topMargin: 16
                        spacing: 8
                        Repeater {
                            model: trackCard.modelData.tags
                            Badge {
                                required property string modelData
                                tone: "neutral"
                                mono: true
                                text: modelData
                                foreground: Theme.zinc400
                                fontSize: Typography.tiny
                                fontWeight: Typography.regular
                                radius: 4
                            }
                        }
                    }
                }
            }
        }
    }

    // ------------------------------------------- Architecture highlights
    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 80
        implicitHeight: archGrid.implicitHeight + 64
        radius: Theme.radiusLg
        color: Theme.alpha(Theme.zinc950, 0.8)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.1)

        GridLayout {
            id: archGrid
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 32
            columns: page.wide ? 3 : 1
            columnSpacing: 24
            rowSpacing: 24

            Repeater {
                model: [
                    {
                        icon: "layers", color: Theme.blue400,
                        title: qsTr("Turborepo Monorepo"),
                        desc: qsTr("Shared types and DTOs in @dev-dialogue/api-contracts eliminate contract drifts between backend and frontend.")
                    },
                    {
                        icon: "zap", color: Theme.amber400,
                        title: qsTr("Autonomous Mock Fallback"),
                        desc: qsTr("100% operational offline without cloud API keys: in-memory simulation runs smoothly with zero external cost.")
                    },
                    {
                        icon: "shield", color: Theme.blue400,
                        title: qsTr("Role-Based Access"),
                        desc: qsTr("Rigid role separation (Admin / Candidate) with an instant navbar switch for frictionless jury and evaluation tests.")
                    }
                ]

                RowLayout {
                    id: archItem
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredWidth: (archGrid.width - archGrid.columnSpacing * (archGrid.columns - 1)) / archGrid.columns
                    Layout.alignment: Qt.AlignTop
                    spacing: 16

                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        implicitWidth: 36
                        implicitHeight: 36
                        radius: Theme.radiusSm
                        color: Theme.zinc800
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.1)
                        Icon { anchors.centerIn: parent; name: archItem.modelData.icon; size: 16; color: archItem.modelData.color }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        AppText {
                            Layout.fillWidth: true
                            text: archItem.modelData.title
                            font.weight: Typography.semibold
                            wrapMode: Text.WordWrap
                        }
                        AppText {
                            Layout.fillWidth: true
                            text: archItem.modelData.desc
                            color: Theme.zinc400
                            font.pixelSize: Typography.xs
                            lineHeight: Typography.leadingRelaxed
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------- Final CTA
    TactileButton {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 64
        text: qsTr("Start Practice Interview")
        iconLeft: "sparkles"
        iconRight: "arrow-right"
        fontWeight: Typography.bold
        implicitHeight: 54
        leftPadding: 32
        rightPadding: 32
        spacing: 10
        glow: true
        onClicked: navVM.navigateToSetup()
    }
}
