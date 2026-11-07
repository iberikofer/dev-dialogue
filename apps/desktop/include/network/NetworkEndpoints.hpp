#pragma once

#include <QString>

namespace devdialogue::network::endpoints {

namespace api {
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
} // namespace api

namespace supabase {
inline const QString Token = "/auth/v1/token";
inline const QString PasswordToken = "/auth/v1/token?grant_type=password";
inline const QString RefreshToken = "/auth/v1/token?grant_type=refresh_token";
inline const QString Signup = "/auth/v1/signup";
inline const QString Logout = "/auth/v1/logout";
} // namespace supabase

// Export symbols into endpoints namespace as well
using namespace api;
using namespace supabase;

} // namespace devdialogue::network::endpoints

namespace devdialogue {
namespace endpoints = network::endpoints;
}
