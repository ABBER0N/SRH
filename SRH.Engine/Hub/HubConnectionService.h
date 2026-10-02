#pragma once

#include "Hub/HubIngressService.h"
#include "Hub/Protocol/HubMessage.h"
#include "Hub/Transport/HubSerialTransport.h"
#include "Public/SrhEngine.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace srh::engine::hub
{
    class HubConnectionService
    {
    public:
        explicit HubConnectionService(
            SrhEngine& engine
        );

        ~HubConnectionService();

        HubConnectionService(
            const HubConnectionService&
        ) = delete;

        HubConnectionService& operator=(
            const HubConnectionService&
            ) = delete;

        HubConnectionService(
            HubConnectionService&&
        ) = delete;

        HubConnectionService& operator=(
            HubConnectionService&&
            ) = delete;

        [[nodiscard]]
        bool Connect(
            std::string_view portName,
            std::uint32_t baudRate =
            HubSerialTransport::
            DefaultBaudRate
        );

        void Disconnect()
            noexcept;

        [[nodiscard]]
        bool IsTransportConnected() const
            noexcept;

        [[nodiscard]]
        bool IsSessionEstablished() const
            noexcept;

        [[nodiscard]]
        std::uint32_t LastError() const
            noexcept;

        [[nodiscard]]
        std::string PortName() const;

        //
        // Encodes:
        //
        // HubMessage
        //   -> HubPacket
        //   -> SRHP frame
        //
        // and writes the complete frame to the
        // active transport.
        //

        [[nodiscard]]
        bool SendHubMessage(
            const protocol::HubMessage& message,
            HubPacketFlags flags =
            HubPacketFlags::None
        );

        [[nodiscard]]
        std::optional<std::uint64_t>
            LastHeartbeatUptimeMs() const
            noexcept;

    private:
        void WorkerMain()
            noexcept;

        [[nodiscard]]
        std::uint32_t AllocateSequence()
            noexcept;

        SrhEngine&
            m_engine;

        HubIngressService
            m_ingress;

        HubSerialTransport
            m_transport;

        mutable std::mutex
            m_stateMutex;

        //
        // Protects synchronous serial I/O and
        // transport open/close operations.
        //

        mutable std::mutex
            m_ioMutex;

        std::thread
            m_worker;

        std::atomic_bool
            m_stopRequested{
                false
        };

        std::atomic<std::uint32_t>
            m_nextSequence{
                1
        };

        bool m_transportConnected{
            false
        };

        std::uint32_t m_lastError{
            0
        };

        std::string
            m_portName;
    };
}