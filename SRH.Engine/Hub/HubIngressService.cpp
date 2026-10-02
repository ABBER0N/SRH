#include "pch.h"

#include "Hub/HubIngressService.h"

#include "Hub/Protocol/HubMessageCodec.h"

#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace
{
    std::string MakeFirmwareVersion(
        const std::uint16_t major,
        const std::uint16_t minor,
        const std::uint16_t patch
    )
    {
        return
            std::to_string(
                major
            ) +
            "." +
            std::to_string(
                minor
            ) +
            "." +
            std::to_string(
                patch
            );
    }
}

namespace srh::engine::hub
{
    HubIngressService::
        HubIngressService(
            SrhEngine& engine
        )
        : m_engine(
            engine
        )
    {
    }

    HubIngressResult
        HubIngressService::Push(
            const std::uint8_t* data,
            const std::size_t size
        )
    {
        HubIngressResult result;

        std::vector<protocol::HubPacket>
            packets;

        //
        // Decoder state is protected independently
        // from message processing.
        //
        // Engine callbacks may execute while an
        // InputEvent is being submitted, so no
        // ingress mutex is held while calling
        // SrhEngine.
        //

        {
            std::scoped_lock lock(
                m_mutex
            );

            packets =
                m_decoder.Push(
                    data,
                    size
                );
        }

        result.packetCount =
            packets.size();

        for (
            const auto& packet :
            packets
            )
        {
            const auto message =
                protocol::
                HubMessageCodec::
                Decode(
                    packet
                );

            if (!message.has_value())
            {
                ++result
                    .rejectedMessageCount;

                continue;
            }

            if (
                !HandleMessage(
                    *message,
                    result
                )
                )
            {
                ++result
                    .rejectedMessageCount;

                continue;
            }

            ++result
                .messageCount;
        }

        return result;
    }

    HubIngressResult
        HubIngressService::Push(
            const std::vector<std::uint8_t>& data
        )
    {
        return
            Push(
                data.data(),
                data.size()
            );
    }

    void HubIngressService::Reset()
    {
        {
            std::scoped_lock lock(
                m_mutex
            );

            m_decoder.Reset();

            m_sessionEstablished =
                false;

            m_hasHeartbeat =
                false;

            m_lastHeartbeatUptimeMs =
                0;
        }

        HubState state =
            m_engine.GetHubState();

        state.connected =
            false;

        state.health =
            DeviceHealth::Unknown;

        state.statusMessage =
            "Disconnected";

        m_engine.SetHubState(
            std::move(
                state
            )
        );
    }

    bool HubIngressService::
        IsSessionEstablished() const
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        return
            m_sessionEstablished;
    }

    std::optional<std::uint64_t>
        HubIngressService::
        LastHeartbeatUptimeMs() const
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        if (!m_hasHeartbeat)
        {
            return
                std::nullopt;
        }

        return
            m_lastHeartbeatUptimeMs;
    }

    std::uint64_t
        HubIngressService::
        DroppedByteCount() const
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        return
            m_decoder
            .DroppedByteCount();
    }

    std::uint64_t
        HubIngressService::
        InvalidPacketCount() const
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        return
            m_decoder
            .InvalidPacketCount();
    }

    bool HubIngressService::
        HandleMessage(
            const protocol::HubMessage& message,
            HubIngressResult& result
        )
    {
        return
            std::visit(
                [this, &result](
                    const auto& typedMessage
                    ) -> bool
                {
                    using MessageType =
                        std::decay_t<
                        decltype(
                            typedMessage
                            )
                        >;

                    if constexpr (
                        std::is_same_v<
                        MessageType,
                        protocol::
                        HelloMessage
                        >
                        )
                    {
                        HandleHello(
                            typedMessage
                        );

                        return true;
                    }
                    else if constexpr (
                        std::is_same_v<
                        MessageType,
                        protocol::
                        HelloAckMessage
                        >
                        )
                    {
                        //
                        // HelloAck is PC -> Hub.
                        //
                        // Receiving one from the Hub
                        // is a direction violation.
                        //

                        return false;
                    }
                    else if constexpr (
                        std::is_same_v<
                        MessageType,
                        protocol::
                        HeartbeatMessage
                        >
                        )
                    {
                        return
                            HandleHeartbeat(
                                typedMessage
                            );
                    }
                    else if constexpr (
                        std::is_same_v<
                        MessageType,
                        InputEvent
                        >
                        )
                    {
                        return
                            HandleInputEvent(
                                typedMessage,
                                result
                            );
                    }
                    else
                    {
                        return false;
                    }
                },
                message
            );
    }

    void HubIngressService::
        HandleHello(
            const protocol::
            HelloMessage& message
        )
    {
        {
            std::scoped_lock lock(
                m_mutex
            );

            m_sessionEstablished =
                true;

            m_hasHeartbeat =
                false;

            m_lastHeartbeatUptimeMs =
                0;
        }

        HubState state =
            m_engine.GetHubState();

        state.physicalUid =
            message.hubUid;

        state.connected =
            true;

        if (state.name.empty())
        {
            state.name =
                "SRH Hub";
        }

        state.firmwareVersion =
            MakeFirmwareVersion(
                message.firmwareMajor,
                message.firmwareMinor,
                message.firmwarePatch
            );

        state.protocolVersion =
            std::to_string(
                HubProtocolVersion
            );

        state.health =
            DeviceHealth::Healthy;

        state.statusMessage =
            "Connected";

        m_engine.SetHubState(
            std::move(
                state
            )
        );
    }

    bool HubIngressService::
        HandleHeartbeat(
            const protocol::
            HeartbeatMessage& message
        )
    {
        std::scoped_lock lock(
            m_mutex
        );

        if (!m_sessionEstablished)
        {
            return false;
        }

        m_lastHeartbeatUptimeMs =
            message.uptimeMs;

        m_hasHeartbeat =
            true;

        return true;
    }

    bool HubIngressService::
        HandleInputEvent(
            const InputEvent& event,
            HubIngressResult& result
        )
    {
        {
            std::scoped_lock lock(
                m_mutex
            );

            //
            // The Hub must identify itself with
            // Hello before it is allowed to inject
            // physical input into the Engine.
            //

            if (!m_sessionEstablished)
            {
                return false;
            }
        }

        result.inputResults.push_back(
            m_engine.SubmitInputEvent(
                event
            )
        );

        return true;
    }
}