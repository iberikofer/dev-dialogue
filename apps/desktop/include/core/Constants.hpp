#pragma once

#include <string_view>

namespace devdialogue::core::constants {

constexpr std::string_view AppName = "Dev Dialogue";
constexpr std::string_view AppVersion = "1.0.0";
constexpr std::string_view OrganizationName = "VNTU";
constexpr std::string_view OrganizationDomain = "vntu.edu.ua";

constexpr int DefaultWindowWidth = 1200;
constexpr int DefaultWindowHeight = 800;
constexpr int MinWindowWidth = 960;
constexpr int MinWindowHeight = 640;

// Network Timeouts (ms)
constexpr int ColdStartTimeoutMs = 90000;    // 90 s for Render cold start and healthcheck
constexpr int StandardTimeoutMs = 30000;     // 30 s for normal REST operations
constexpr int HealthCheckIntervalMs = 30000; // 30 s periodic heartbeat
constexpr int MaxGetRetries = 2;             // Retries allowed only for GET requests
constexpr int WakingUpThresholdMs = 3000;    // Trigger isWakingUp if request takes > 3s

// Secure Storage Keys
constexpr std::string_view StorageKeyAccessToken = "jwt_token";
constexpr std::string_view StorageKeyRefreshToken = "refresh_token";
constexpr std::string_view StorageKeyUser = "user_session";
constexpr std::string_view StorageKeyLanguage = "ui_language";

} // namespace devdialogue::core::constants
