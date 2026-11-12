pragma Singleton
import QtQuick

// The theme and icon singletons of this module are resolved through the
// implicit module import, so no self import is declared here: a self import
// inside a singleton makes the QML test runner instantiate it recursively.
// qmllint disable missing-property
QtObject {
    function directionLabel(id) {
        switch (id) {
        case "frontend": return qsTr("Frontend")
        case "backend": return qsTr("Backend")
        case "devops": return qsTr("DevOps")
        case "qa": return qsTr("QA")
        case "pm": return qsTr("Product Management")
        default: return id
        }
    }

    function directionShortLabel(id) {
        switch (id) {
        case "pm": return qsTr("PM")
        default: return directionLabel(id)
        }
    }

    function directionDescription(id) {
        switch (id) {
        case "frontend": return qsTr("React, TypeScript, browser performance and UI architecture.")
        case "backend": return qsTr("APIs, databases, distributed systems and service design.")
        case "devops": return qsTr("CI/CD, containers, Kubernetes and observability.")
        case "qa": return qsTr("Test strategy, automation, API checks and release quality.")
        case "pm": return qsTr("Discovery, prioritisation, metrics and product trade-offs.")
        default: return ""
        }
    }

    function directionIcon(id) {
        switch (id) {
        case "frontend": return Icons.frontend
        case "backend": return Icons.backend
        case "devops": return Icons.devops
        case "qa": return Icons.qa
        case "pm": return Icons.pm
        default: return Icons.dashboard
        }
    }

    function levelLabel(id) {
        switch (id) {
        case "trainee": return qsTr("Trainee")
        case "junior": return qsTr("Junior")
        case "middle": return qsTr("Middle")
        default: return id
        }
    }

    function levelDescription(id) {
        switch (id) {
        case "trainee": return qsTr("Fundamentals, motivation and learning potential.")
        case "junior": return qsTr("Core concepts and one to two years of practice.")
        case "middle": return qsTr("Independent delivery, architecture and trade-offs.")
        default: return ""
        }
    }

    function stageLabel(id) {
        switch (id) {
        case "hard_skills": return qsTr("Hard skills")
        case "soft_skills": return qsTr("Soft skills")
        case "english": return qsTr("English")
        default: return id
        }
    }

    function statusLabel(id) {
        switch (id) {
        case "active": return qsTr("Active")
        case "completed": return qsTr("Completed")
        case "abandoned": return qsTr("Abandoned")
        default: return id
        }
    }

    function statusColor(id) {
        switch (id) {
        case "active": return Theme.accentAmber
        case "completed": return Theme.accentEmerald
        case "abandoned": return Theme.textMuted
        default: return Theme.textSecondary
        }
    }

    function languageLabel(id) {
        switch (id) {
        case "uk": return qsTr("Ukrainian")
        case "en": return qsTr("English")
        default: return id
        }
    }

    function roleLabel(role) {
        switch (role) {
        case "user": return qsTr("Candidate")
        case "admin": return qsTr("Administrator")
        default: return role
        }
    }

    function scoreColor(score) {
        if (score >= 85) return Theme.accentEmerald
        if (score >= 70) return Theme.accentAmber
        return Theme.accentRose
    }

    function scoreLabel(score) {
        if (score >= 85) return qsTr("Excellent")
        if (score >= 70) return qsTr("Good")
        return qsTr("Needs practice")
    }

    function formatDate(iso) {
        const date = new Date(iso)
        if (isNaN(date.getTime())) return iso
        return Qt.formatDate(date, "dd MMM yyyy")
    }

    function formatTime(iso) {
        const date = new Date(iso)
        if (isNaN(date.getTime())) return ""
        return Qt.formatTime(date, "HH:mm")
    }

    function formatDateTime(iso) {
        const date = new Date(iso)
        if (isNaN(date.getTime())) return iso
        return Qt.formatDateTime(date, "dd MMM yyyy, HH:mm")
    }
}
