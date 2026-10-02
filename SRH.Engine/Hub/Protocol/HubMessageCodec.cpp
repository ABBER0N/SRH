#include "pch.h"

#include "Hub/Protocol/HubMessageCodec.h"

#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    void WriteUInt16Le(
        std::vector<std::uint8_t>& buffer,
        const std::size_t offset,
        const std::uint16_t value
    )
    {
        buffer[offset + 0] =
            static_cast<std::uint8_t>(
                value & 0xFFu
                );

        buffer[offset + 1] =
            static_cast<std::uint8_t>(
                (value >> 8) &
                0xFFu
                );
    }

    void WriteUInt32Le(
        std::vector<std::uint8_t>& buffer,
        const std::size_t offset,
        const std::uint32_t value
    )
    {
        buffer[offset + 0] =
            static_cast<std::uint8_t>(
                value & 0xFFu
                );

        buffer[offset + 1] =
            static_cast<std::uint8_t>(
                (value >> 8) &
                0xFFu
                );

        buffer[offset + 2] =
            static_cast<std::uint8_t>(
                (value >> 16) &
                0xFFu
                );

        buffer[offset + 3] =
            static_cast<std::uint8_t>(
                (value >> 24) &
                0xFFu
                );
    }

    void WriteUInt64Le(
        std::vector<std::uint8_t>& buffer,
        const std::size_t offset,
        const std::uint64_t value
    )
    {
        for (
            std::size_t index = 0;
            index < 8;
            ++index
            )
        {
            buffer[offset + index] =
                static_cast<std::uint8_t>(
                    (
                        value >>
                        (index * 8)
                        ) &
                    0xFFu
                    );
        }
    }

    void WriteInt32Le(
        std::vector<std::uint8_t>& buffer,
        const std::size_t offset,
        const std::int32_t value
    )
    {
        WriteUInt32Le(
            buffer,
            offset,
            static_cast<std::uint32_t>(
                value
                )
        );
    }

    std::uint16_t ReadUInt16Le(
        const std::uint8_t* data
    )
    {
        return
            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(
                    data[0]
                    ) |
                (
                    static_cast<std::uint16_t>(
                        data[1]
                        ) <<
                    8
                    )
                );
    }

    std::uint32_t ReadUInt32Le(
        const std::uint8_t* data
    )
    {
        return
            static_cast<std::uint32_t>(
                static_cast<std::uint32_t>(
                    data[0]
                    ) |
                (
                    static_cast<std::uint32_t>(
                        data[1]
                        ) <<
                    8
                    ) |
                (
                    static_cast<std::uint32_t>(
                        data[2]
                        ) <<
                    16
                    ) |
                (
                    static_cast<std::uint32_t>(
                        data[3]
                        ) <<
                    24
                    )
                );
    }

    std::uint64_t ReadUInt64Le(
        const std::uint8_t* data
    )
    {
        std::uint64_t value =
            0;

        for (
            std::size_t index = 0;
            index < 8;
            ++index
            )
        {
            value |=
                static_cast<std::uint64_t>(
                    data[index]
                    ) <<
                (index * 8);
        }

        return value;
    }

    std::int32_t ReadInt32Le(
        const std::uint8_t* data
    )
    {
        const std::uint32_t raw =
            ReadUInt32Le(
                data
            );

        std::int32_t value =
            0;

        static_assert(
            sizeof(value) ==
            sizeof(raw)
            );

        std::memcpy(
            &value,
            &raw,
            sizeof(value)
        );

        return value;
    }
}

namespace srh::engine::hub::protocol
{
    std::optional<HubPacket>
        HubMessageCodec::Encode(
            const HubMessage& message,
            const std::uint32_t sequence,
            const HubPacketFlags flags
        )
    {
        HubPacket packet;

        packet.protocolVersion =
            HubProtocolVersion;

        packet.sequence =
            sequence;

        packet.flags =
            flags;

        bool valid =
            true;

        std::visit(
            [&packet, &valid](
                const auto& typedMessage
                )
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
                    HelloMessage
                    >
                    )
                {
                    packet.messageType =
                        HubMessageType::
                        Hello;

                    packet.payload.resize(
                        HelloPayloadSize,
                        0
                    );

                    WriteUInt64Le(
                        packet.payload,
                        0,
                        typedMessage.hubUid
                    );

                    WriteUInt16Le(
                        packet.payload,
                        8,
                        typedMessage
                        .firmwareMajor
                    );

                    WriteUInt16Le(
                        packet.payload,
                        10,
                        typedMessage
                        .firmwareMinor
                    );

                    WriteUInt16Le(
                        packet.payload,
                        12,
                        typedMessage
                        .firmwarePatch
                    );

                    WriteUInt16Le(
                        packet.payload,
                        14,
                        typedMessage
                        .hardwareRevision
                    );
                }
                else if constexpr (
                    std::is_same_v<
                    MessageType,
                    HelloAckMessage
                    >
                    )
                {
                    packet.messageType =
                        HubMessageType::
                        HelloAck;

                    packet.payload.resize(
                        HelloAckPayloadSize,
                        0
                    );

                    WriteUInt32Le(
                        packet.payload,
                        0,
                        typedMessage
                        .heartbeatIntervalMs
                    );
                }
                else if constexpr (
                    std::is_same_v<
                    MessageType,
                    HeartbeatMessage
                    >
                    )
                {
                    packet.messageType =
                        HubMessageType::
                        Heartbeat;

                    packet.payload.resize(
                        HeartbeatPayloadSize,
                        0
                    );

                    WriteUInt64Le(
                        packet.payload,
                        0,
                        typedMessage.uptimeMs
                    );
                }
                else if constexpr (
                    std::is_same_v<
                    MessageType,
                    InputEvent
                    >
                    )
                {
                    if (
                        !HubMessageCodec::
                        IsValidInputEventType(
                            typedMessage.type
                        )
                        )
                    {
                        valid =
                            false;

                        return;
                    }

                    packet.messageType =
                        HubMessageType::
                        InputEvent;

                    packet.payload.resize(
                        InputEventPayloadSize,
                        0
                    );

                    WriteUInt16Le(
                        packet.payload,
                        0,
                        typedMessage.nodeId
                    );

                    WriteUInt16Le(
                        packet.payload,
                        2,
                        typedMessage.controlId
                    );

                    packet.payload[4] =
                        static_cast<
                        std::uint8_t
                        >(
                            typedMessage.type
                            );

                    //
                    // Bytes 5..7 remain reserved
                    // and are deliberately zero.
                    //

                    WriteInt32Le(
                        packet.payload,
                        8,
                        typedMessage.value
                    );

                    WriteUInt64Le(
                        packet.payload,
                        12,
                        typedMessage.timestamp
                    );
                }
                else
                {
                    valid =
                        false;
                }
            },
            message
        );

        if (!valid)
        {
            return std::nullopt;
        }

        return packet;
    }

    std::optional<HubMessage>
        HubMessageCodec::Decode(
            const HubPacket& packet
        )
    {
        if (
            packet.protocolVersion !=
            HubProtocolVersion
            )
        {
            return std::nullopt;
        }

        switch (
            packet.messageType
            )
        {
        case HubMessageType::Hello:
        {
            if (
                packet.payload.size() !=
                HelloPayloadSize
                )
            {
                return std::nullopt;
            }

            HelloMessage message;

            message.hubUid =
                ReadUInt64Le(
                    packet.payload.data() +
                    0
                );

            message.firmwareMajor =
                ReadUInt16Le(
                    packet.payload.data() +
                    8
                );

            message.firmwareMinor =
                ReadUInt16Le(
                    packet.payload.data() +
                    10
                );

            message.firmwarePatch =
                ReadUInt16Le(
                    packet.payload.data() +
                    12
                );

            message.hardwareRevision =
                ReadUInt16Le(
                    packet.payload.data() +
                    14
                );

            return HubMessage{
                message
            };
        }

        case HubMessageType::HelloAck:
        {
            if (
                packet.payload.size() !=
                HelloAckPayloadSize
                )
            {
                return std::nullopt;
            }

            HelloAckMessage message;

            message.heartbeatIntervalMs =
                ReadUInt32Le(
                    packet.payload.data()
                );

            return HubMessage{
                message
            };
        }

        case HubMessageType::Heartbeat:
        {
            if (
                packet.payload.size() !=
                HeartbeatPayloadSize
                )
            {
                return std::nullopt;
            }

            HeartbeatMessage message;

            message.uptimeMs =
                ReadUInt64Le(
                    packet.payload.data()
                );

            return HubMessage{
                message
            };
        }

        case HubMessageType::InputEvent:
        {
            if (
                packet.payload.size() !=
                InputEventPayloadSize
                )
            {
                return std::nullopt;
            }

            const auto type =
                static_cast<InputEventType>(
                    packet.payload[4]
                    );

            if (
                !IsValidInputEventType(
                    type
                )
                )
            {
                return std::nullopt;
            }

            //
            // Reserved bytes must currently be zero.
            //
            // This prevents accidentally accepting
            // an incompatible future payload layout
            // as protocol v1.
            //

            if (
                packet.payload[5] != 0 ||
                packet.payload[6] != 0 ||
                packet.payload[7] != 0
                )
            {
                return std::nullopt;
            }

            InputEvent event;

            event.nodeId =
                ReadUInt16Le(
                    packet.payload.data() +
                    0
                );

            event.controlId =
                ReadUInt16Le(
                    packet.payload.data() +
                    2
                );

            event.type =
                type;

            event.value =
                ReadInt32Le(
                    packet.payload.data() +
                    8
                );

            event.timestamp =
                ReadUInt64Le(
                    packet.payload.data() +
                    12
                );

            return HubMessage{
                event
            };
        }

        default:
            return std::nullopt;
        }
    }

    bool HubMessageCodec::
        IsValidInputEventType(
            const InputEventType type
        ) noexcept
    {
        switch (type)
        {
        case InputEventType::ButtonDown:
        case InputEventType::ButtonUp:
        case InputEventType::EncoderDelta:
        case InputEventType::AxisValue:
            return true;

        case InputEventType::Unknown:
        default:
            return false;
        }
    }
}