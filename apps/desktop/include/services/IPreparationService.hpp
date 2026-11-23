#pragma once

#include <QString>
#include <functional>
#include "models/PreparationModels.hpp"

namespace devdialogue::services {

class IPreparationService {
public:
    virtual ~IPreparationService() = default;

    virtual void analyzeProfile(
        const QString& direction,
        const QString& level,
        const QString& userDescription,
        const QString& language,
        std::function<void(const models::PreparationPlanResult& result, const QString& error)> callback
    ) = 0;
};

} // namespace devdialogue::services
