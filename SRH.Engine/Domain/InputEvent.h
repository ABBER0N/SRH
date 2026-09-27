#pragma once

#include "Domain/InputTypes.h"
#include <cstdint>

namespace srh::engine
{
    struct InputEvent
    {
        NodeId nodeId{ 0 };
        ControlId controlId{ 0 };

        InputEventType type{
            InputEventType::Unknown
        };

        std::int32_t value{ 0 };

        std::uint64_t timestamp{ 0 };
    };
}