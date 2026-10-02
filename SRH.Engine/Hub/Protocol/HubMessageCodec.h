#pragma once

#include "Hub/Protocol/HubMessage.h"
#include "Hub/Protocol/HubPacket.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace srh::engine::hub::protocol
{
    inline constexpr std::size_t
        HelloPayloadSize = 16;

    inline constexpr std::size_t
        HelloAckPayloadSize = 4;

    inline constexpr std::size_t
        HeartbeatPayloadSize = 8;

    inline constexpr std::size_t
        InputEventPayloadSize = 20;

    class HubMessageCodec
    {
    public:
        [[nodiscard]]
        static std::optional<HubPacket>
            Encode(
                const HubMessage& message,
                std::uint32_t sequence,
                HubPacketFlags flags =
                HubPacketFlags::None
            );

        [[nodiscard]]
        static std::optional<HubMessage>
            Decode(
                const HubPacket& packet
            );

    private:
        [[nodiscard]]
        static bool IsValidInputEventType(
            InputEventType type
        ) noexcept;
    };
}