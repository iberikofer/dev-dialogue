#pragma once

#include <QString>

namespace devdialogue::network::endpoints {

// API Endpoints
inline const QString HealthPing = "/api/health/ping";
inline const QString Health = "/health";
inline const QString AuthMe = "/api/auth/me";
inline const QString Options = "/api/options";

inline const QString Interviews = "/api/interviews";
inline const QString InterviewStats = "/api/interviews/stats";
inline QString interviewById(const QString& id) { return "/api/interviews/" + id; }
inline QString interviewMessage(const QString& id) { return "/api/interviews/" + id + "/message"; }
inline QString interviewComplete(const QString& id) { return "/api/interviews/" + id + "/complete"; }

inline const QString AdminQuestions = "/api/admin/questions";
inline QString adminQuestionById(const QString& id) { return "/api/admin/questions/" + id; }

// Supabase Auth Endpoints
inline const QString SupabaseToken = "/auth/v1/token";
inline const QString SupabaseSignup = "/auth/v1/signup";
inline const QString SupabaseLogout = "/auth/v1/logout";

} // namespace devdialogue::network::endpoints
