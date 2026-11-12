#pragma once

#include <QString>
#include <QStringList>
#include <vector>

#include "models/ApiContracts.hpp"

// Local sample data shown when the backend cannot be reached. It mirrors the
// fallback content of the web client (DashboardView.tsx, ChatRoom.tsx,
// PerformanceReport.tsx, AdminPanel.tsx) so both clients demo identically.
//
// This is a presentation-level stand-in only. The embedded offline interview
// engine replaces it for real offline sessions.
namespace devdialogue::viewmodels::demo {

models::UserDashboardStats dashboardStats();
std::vector<models::InterviewSummary> interviews();

// `language` is "uk" or "en"; anything else falls back to English.
models::InterviewFeedbackReport report(const QString& language);
std::vector<models::QuestionBankItem> questions(const QString& language);

QString chatWelcome(const QString& direction, const QString& level, const QString& language);
QStringList chatReplies(const QString& language);

} // namespace devdialogue::viewmodels::demo
