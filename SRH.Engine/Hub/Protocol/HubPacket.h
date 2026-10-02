#pragma once

#include "Domain/HubProtocolTypes.h"

#include <cstdint>
#include <vector>

namespace srh::engine::hub::protocol
{
    struct HubPacket
    {
        std::uint8_t protocolVersion{
            HubProtocolVersion
        };

        HubMessageType messageType{
            HubMessageType::Unknown
        };

        HubPacketFlags flags{
            HubPacketFlags::None
        };

        std::uint32_t sequence{
            0
        };

        std::vector<std::uint8_t>
            payload;
    };
}