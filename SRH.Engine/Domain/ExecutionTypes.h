#pragma once

#include "Domain/Action.h"

#include <cstdint>
#include <vector>

namespace srh::engine
{
    enum class ActionExecutionStatus :
        std::uint8_t
    {
        Executed = 0,
        Unsupported,
        Failed
    };

    struct ActionExecutionResult
    {
        Action action;

        ActionExecutionStatus status{
            ActionExecutionStatus::Unsupported
        };
    };

    struct InputProcessingResult
    {
        bool accepted{ false };

        std::vector<ActionExecutionResult>
            executions;

        [[nodiscard]]
        bool AllExecuted() const
        {
            if (!accepted)
            {
                return false;
            }

            for (
                const auto& execution :
                executions
                )
            {
                if (
                    execution.status !=
                    ActionExecutionStatus::Executed
                    )
                {
                    return false;
                }
            }

            return true;
        }
    };
}