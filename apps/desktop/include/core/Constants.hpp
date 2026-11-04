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

constexpr int NetworkTimeoutMs = 15000;
constexpr int HealthCheckIntervalMs = 30000;

constexpr std::string_view StorageKeyToken = "jwt_token";
constexpr std::string_view StorageKeyUser = "user_session";
constexpr std::string_view StorageKeyLanguage = "ui_language";

} // namespace devdialogue::core::constants
