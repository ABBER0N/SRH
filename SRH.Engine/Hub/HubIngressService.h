#pragma once

#include "Hub/Protocol/HubMessage.h"
#include "Hub/Protocol/HubProtocol.h"
#include "Public/SrhEngine.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace srh::engine::hub
{
    struct HubIngressResult
    {
        //
        // Number of valid framed packets produced
        // by HubStreamDecoder during this Push().
        //

        std::size_t packetCount{
            0
        };

        //
        // Number of semantic Hub messages accepted
        // and processed by HubIngressService.
        //

        std::size_t messageCount{
            0
        };

        //
        // Framed packets which could not be decoded
        // or were invalid for the current direction
        // or session state.
        //

        std::size_t rejectedMessageCount{
            0
        };

        //
        // Processing results produced specifically
        // by incoming InputEvent messages.
        //

        std::vector<InputProcessingResult>
            inputResults;
    };

    class HubIngressService
    {
    public:
        explicit HubIngressService(
            SrhEngine& engine
        );

        [[nodiscard]]
        HubIngressResult Push(
            const std::uint8_t* data,
            std::size_t size
        );

        [[nodiscard]]
        HubIngressResult Push(
            const std::vector<std::uint8_t>& data
        );

        //
        // Called when the physical Hub transport
        // disconnects or is restarted.
        //

        void Reset();

        [[nodiscard]]
        bool IsSessionEstablished() const
            noexcept;

        [[nodiscard]]
        std::optional<std::uint64_t>
            LastHeartbeatUptimeMs() const
            noexcept;

        [[nodiscard]]
        std::uint64_t DroppedByteCount() const
            noexcept;

        [[nodiscard]]
        std::uint64_t InvalidPacketCount() const
            noexcept;

    private:
        [[nodiscard]]
        bool HandleMessage(
            const protocol::HubMessage& message,
            HubIngressResult& result
        );

        void HandleHello(
            const protocol::HelloMessage& message
        );

        [[nodiscard]]
        bool HandleHeartbeat(
            const protocol::HeartbeatMessage& message
        );

        [[nodiscard]]
        bool HandleInputEvent(
            const InputEvent& event,
            HubIngressResult& result
        );

        SrhEngine&
            m_engine;

        mutable std::mutex
            m_mutex;

        protocol::HubStreamDecoder
            m_decoder;

        bool m_sessionEstablished{
            false
        };

        bool m_hasHeartbeat{
            false
        };

        std::uint64_t
            m_lastHeartbeatUptimeMs{
                0
        };
    };
}