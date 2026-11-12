#pragma once

#include <QString>

#include <nlohmann/json.hpp>

#include "models/ApiContracts.hpp"

/// QML-facing enum helpers. QML passes the lowercase identifiers used by the
/// web client ("frontend", "junior", "hard_skills", ...), so these functions
/// convert in both directions without throwing on unknown input.
namespace devdialogue::core::enums {

inline QString toString(models::InterviewDirection value) {
  return QString::fromStdString(nlohmann::json(value).get<std::string>());
}

inline QString toString(models::InterviewLevel value) {
  return QString::fromStdString(nlohmann::json(value).get<std::string>());
}

inline QString toString(models::InterviewLanguage value) {
  return QString::fromStdString(nlohmann::json(value).get<std::string>());
}

inline QString toString(models::InterviewStage value) {
  return QString::fromStdString(nlohmann::json(value).get<std::string>());
}

inline QString toString(models::InterviewStatus value) {
  return QString::fromStdString(nlohmann::json(value).get<std::string>());
}

inline models::InterviewDirection directionFrom(const QString &value) {
  if (value == QLatin1String("backend"))
    return models::InterviewDirection::Backend;
  if (value == QLatin1String("devops"))
    return models::InterviewDirection::DevOps;
  if (value == QLatin1String("qa"))
    return models::InterviewDirection::Qa;
  if (value == QLatin1String("pm"))
    return models::InterviewDirection::Pm;
  return models::InterviewDirection::Frontend;
}

inline models::InterviewLevel levelFrom(const QString &value) {
  if (value == QLatin1String("trainee"))
    return models::InterviewLevel::Trainee;
  if (value == QLatin1String("middle"))
    return models::InterviewLevel::Middle;
  return models::InterviewLevel::Junior;
}

inline models::InterviewLanguage languageFrom(const QString &value) {
  return value == QLatin1String("en") ? models::InterviewLanguage::En
                                      : models::InterviewLanguage::Uk;
}

inline models::InterviewStage stageFrom(const QString &value) {
  if (value == QLatin1String("soft_skills"))
    return models::InterviewStage::SoftSkills;
  if (value == QLatin1String("english"))
    return models::InterviewStage::English;
  return models::InterviewStage::HardSkills;
}

} // namespace devdialogue::core::enums
