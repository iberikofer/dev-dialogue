import QtQuick
import QtTest
import DevDialogue.App

TestCase {
    id: testCase

    name: "Labels"
    when: windowShown
    visible: true
    width: 400
    height: 200

    function test_directionLabels() {
        compare(Labels.directionLabel("frontend"), "Frontend")
        compare(Labels.directionLabel("backend"), "Backend")
        compare(Labels.directionLabel("devops"), "DevOps")
        compare(Labels.directionLabel("qa"), "QA")
        compare(Labels.directionLabel("pm"), "Product Management")
        compare(Labels.directionShortLabel("pm"), "PM")
        compare(Labels.directionLabel("unknown"), "unknown")
    }

    function test_levelAndStageLabels() {
        compare(Labels.levelLabel("trainee"), "Trainee")
        compare(Labels.levelLabel("junior"), "Junior")
        compare(Labels.levelLabel("middle"), "Middle")
        compare(Labels.stageLabel("hard_skills"), "Hard skills")
        compare(Labels.stageLabel("soft_skills"), "Soft skills")
        compare(Labels.stageLabel("english"), "English")
    }

    function test_statusAndRoleLabels() {
        compare(Labels.statusLabel("active"), "Active")
        compare(Labels.statusLabel("completed"), "Completed")
        compare(Labels.statusLabel("abandoned"), "Abandoned")
        compare(Labels.roleLabel("user"), "Candidate")
        compare(Labels.roleLabel("admin"), "Administrator")
    }

    function test_scoreThresholds() {
        // The score thresholds are part of the design contract:
        // >= 85 emerald, 70-84 amber, below 70 rose.
        compare(Labels.scoreLabel(100), "Excellent")
        compare(Labels.scoreLabel(85), "Excellent")
        compare(Labels.scoreLabel(84), "Good")
        compare(Labels.scoreLabel(70), "Good")
        compare(Labels.scoreLabel(69), "Needs practice")
        compare(Labels.scoreLabel(0), "Needs practice")

        compare(Labels.scoreColor(90), Theme.accentEmerald)
        compare(Labels.scoreColor(85), Theme.accentEmerald)
        compare(Labels.scoreColor(84), Theme.accentAmber)
        compare(Labels.scoreColor(70), Theme.accentAmber)
        compare(Labels.scoreColor(69), Theme.accentRose)
        compare(Labels.scoreColor(10), Theme.accentRose)
    }

    function test_statusColors() {
        compare(Labels.statusColor("active"), Theme.accentAmber)
        compare(Labels.statusColor("completed"), Theme.accentEmerald)
        compare(Labels.statusColor("abandoned"), Theme.textMuted)
    }

    function test_directionIconsResolve() {
        verify(Labels.directionIcon("frontend").indexOf("frontend.svg") >= 0)
        verify(Labels.directionIcon("pm").indexOf("pm.svg") >= 0)
        verify(Labels.directionIcon("unknown").indexOf("dashboard.svg") >= 0)
    }

    function test_dateFormattingFallsBackToTheRawValue() {
        compare(Labels.formatDate("not-a-date"), "not-a-date")
        compare(Labels.formatDateTime("not-a-date"), "not-a-date")
        verify(Labels.formatDate("2026-11-04T12:00:00Z").indexOf("2026") >= 0)
    }
}
