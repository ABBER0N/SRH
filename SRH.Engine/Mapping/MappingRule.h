#pragma once

#include "Domain/Action.h"
#include "Domain/InputEvent.h"

#include <cstdint>

namespace srh::engine
{
    using MappingRuleId =
        std::uint64_t;

    enum class InputValueCondition :
        std::uint8_t
    {
        Any = 0,
        Equal,
        Positive,
        Negative
    };

    enum class ActionValueMode :
        std::uint8_t
    {
        Fixed = 0,
        FromInput
    };

    struct MappingInput
    {
        NodeId nodeId{ 0 };
        ControlId controlId{ 0 };

        InputEventType eventType{
            InputEventType::Unknown
        };

        InputValueCondition valueCondition{
            InputValueCondition::Any
        };

        std::int32_t compareValue{ 0 };
    };

    struct MappingRule
    {
        MappingRuleId id{ 0 };

        bool enabled{ true };

        MappingInput input;

        Action action;

        ActionValueMode valueMode{
            ActionValueMode::Fixed
        };

        float valueScale{ 1.0f };
        float valueOffset{ 0.0f };
    };
}