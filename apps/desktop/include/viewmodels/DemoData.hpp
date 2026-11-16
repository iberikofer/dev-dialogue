#pragma once

#include <QString>
#include <QStringList>
#include <vector>

#include "models/ApiContracts.hpp"

// Local data shown when the backend cannot be reached.
//
// The demo has to stay consistent with itself: the history lists two sample
// interviews plus every interview finished offline in this run, the numbers
// on Home are counted from that list, and a report always belongs to the
// interview it is opened for. A report for an offline interview is built
// from its own transcript; with no evaluator available, each answer is
// scored by how detailed it is and the report says so.
//
// This is a presentation-level stand-in only. The embedded offline interview
// engine replaces it for real offline sessions.
namespace devdialogue::viewmodels::demo {

// --- History ---------------------------------------------------------------

// Remembers an interview finished without the server (kept in memory for the
// lifetime of the app). Recording the same id again replaces the entry.
void recordFinishedInterview(const models::InterviewDetail& interview);
// The recorded interview with this id, or nullptr.
const models::InterviewDetail* finishedInterview(const QString& interviewId);
// Forgets the recorded interviews (used by tests).
void clearFinishedInterviews();

// Sample interviews and recorded ones, newest first.
std::vector<models::InterviewSummary> interviews();
// Counted from interviews().
models::UserDashboardStats dashboardStats();

// --- Reports ---------------------------------------------------------------

// Report for an interview of the demo history. Sample interviews get their
// sample report in `uiLanguage`; recorded ones are scored from their
// transcript in the interview language. Unknown ids get an empty report.
models::InterviewFeedbackReport report(const QString& interviewId, const QString& uiLanguage);
// Report built from a transcript, scored by the detail of each answer.
models::InterviewFeedbackReport reportFromTranscript(const QString& direction,
                                                     const std::vector<models::InterviewMessage>& transcript,
                                                     const QString& language);

// --- Interview room and question bank ----------------------------------------

// `language` is "uk" or "en"; anything else falls back to English.
std::vector<models::QuestionBankItem> questions(const QString& language);

QString chatWelcome(const QString& direction, const QString& level, const QString& language);
// Follow-up questions of the track, then a closing line.
QStringList chatReplies(const QString& direction, const QString& language);

} // namespace devdialogue::viewmodels::demo
