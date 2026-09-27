#pragma once

#include "Domain/DeviceState.h"
#include <cstdint>
#include <vector>

namespace srh::engine
{
    using ControlId = std::uint16_t;

    enum class InputEventType : std::uint8_t
    {
        Unknown = 0,

        ButtonDown,
        ButtonUp,

        EncoderDelta,

        AxisValue
    };

    struct InputControlState
    {
        NodeId nodeId{ 0 };
        ControlId controlId{ 0 };

        InputEventType type{
            InputEventType::Unknown
        };

        std::int32_t value{ 0 };

        std::uint64_t timestamp{ 0 };
    };

    struct InputSnapshot
    {
        std::vector<InputControlState>
            controls;
    };
}