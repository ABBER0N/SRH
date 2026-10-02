#pragma once

#include <cstdint>

namespace srh::engine
{
    inline constexpr std::uint8_t
        HubProtocolVersion = 1;

    enum class HubMessageType :
        std::uint8_t
    {
        Unknown = 0,

        //
        // Hub -> PC / bidirectional session control
        //

        Hello = 1,
        HelloAck = 2,
        Heartbeat = 3,

        //
        // Hub state
        //

        HubState = 10,
        RequestState = 11,

        //
        // Nodes
        //

        NodeDiscovered = 20,
        NodeRemoved = 21,
        NodeState = 22,

        //
        // Input
        //

        InputEvent = 30,

        //
        // PC -> Hub
        //

        HubAction = 40,
        NodeCommand = 41,
        SetConfiguration = 42,

        //
        // Diagnostics
        //

        Error = 250
    };

    enum class HubPacketFlags :
        std::uint16_t
    {
        None = 0,

        //
        // Sender expects a response associated
        // with this sequence number.
        //

        ResponseRequired = 1u << 0,

        //
        // Packet is a response to a previous
        // request.
        //

        Response = 1u << 1,

        //
        // Payload represents an error response.
        //

        Error = 1u << 2
    };

    [[nodiscard]]
    constexpr HubPacketFlags operator|(
        const HubPacketFlags left,
        const HubPacketFlags right
        ) noexcept
    {
        return
            static_cast<HubPacketFlags>(
                static_cast<std::uint16_t>(
                    left
                    ) |
                static_cast<std::uint16_t>(
                    right
                    )
                );
    }

    [[nodiscard]]
    constexpr HubPacketFlags operator&(
        const HubPacketFlags left,
        const HubPacketFlags right
        ) noexcept
    {
        return
            static_cast<HubPacketFlags>(
                static_cast<std::uint16_t>(
                    left
                    ) &
                static_cast<std::uint16_t>(
                    right
                    )
                );
    }

    [[nodiscard]]
    constexpr bool HasFlag(
        const HubPacketFlags value,
        const HubPacketFlags flag
    ) noexcept
    {
        return
            (
                static_cast<std::uint16_t>(
                    value
                    ) &
                static_cast<std::uint16_t>(
                    flag
                    )
                ) != 0;
    }
}