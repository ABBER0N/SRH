#pragma once

#include "Domain/InputEvent.h"

#include <cstdint>
#include <variant>

namespace srh::engine::hub::protocol
{
    struct HelloMessage
    {
        std::uint64_t hubUid{ 0 };

        std::uint16_t firmwareMajor{ 0 };
        std::uint16_t firmwareMinor{ 0 };
        std::uint16_t firmwarePatch{ 0 };

        std::uint16_t hardwareRevision{ 0 };
    };

    struct HelloAckMessage
    {
        std::uint32_t heartbeatIntervalMs{
            1000
        };
    };

    struct HeartbeatMessage
    {
        std::uint64_t uptimeMs{
            0
        };
    };

    using HubMessage =
        std::variant<
        HelloMessage,
        HelloAckMessage,
        HeartbeatMessage,
        InputEvent
        >;
}