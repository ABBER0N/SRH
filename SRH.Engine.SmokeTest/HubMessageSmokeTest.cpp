#include "HubMessageSmokeTest.h"

#include "Hub/Protocol/HubMessageCodec.h"
#include "Hub/Protocol/HubProtocol.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

namespace
{
    using namespace
        srh::engine;

    using namespace
        srh::engine::hub::protocol;

    bool TestHelloMessage()
    {
        HelloMessage original;

        original.hubUid =
            0x1122334455667788ull;

        original.firmwareMajor =
            1;

        original.firmwareMinor =
            2;

        original.firmwarePatch =
            3;

        original.hardwareRevision =
            4;

        const HubMessage message =
            original;

        const auto packet =
            HubMessageCodec::Encode(
                message,
                100
            );

        if (!packet.has_value())
        {
            return false;
        }

        if (
            packet->messageType !=
            HubMessageType::Hello ||
            packet->sequence !=
            100 ||
            packet->payload.size() !=
            HelloPayloadSize
            )
        {
            return false;
        }

        const auto decoded =
            HubMessageCodec::Decode(
                *packet
            );

        if (!decoded.has_value())
        {
            return false;
        }

        const auto* result =
            std::get_if<
            HelloMessage
            >(
                &*decoded
            );

        if (result == nullptr)
        {
            return false;
        }

        return
            result->hubUid ==
            original.hubUid &&
            result->firmwareMajor ==
            original.firmwareMajor &&
            result->firmwareMinor ==
            original.firmwareMinor &&
            result->firmwarePatch ==
            original.firmwarePatch &&
            result->hardwareRevision ==
            original.hardwareRevision;
    }

    bool TestHelloAckMessage()
    {
        HelloAckMessage original;

        original.heartbeatIntervalMs =
            750;

        const auto packet =
            HubMessageCodec::Encode(
                HubMessage{
                    original
                },
                101,
                HubPacketFlags::
                Response
            );

        if (!packet.has_value())
        {
            return false;
        }

        if (
            packet->messageType !=
            HubMessageType::HelloAck ||
            packet->flags !=
            HubPacketFlags::Response ||
            packet->payload.size() !=
            HelloAckPayloadSize
            )
        {
            return false;
        }

        const auto decoded =
            HubMessageCodec::Decode(
                *packet
            );

        if (!decoded.has_value())
        {
            return false;
        }

        const auto* result =
            std::get_if<
            HelloAckMessage
            >(
                &*decoded
            );

        return
            result != nullptr &&
            result->heartbeatIntervalMs ==
            original.heartbeatIntervalMs;
    }

    bool TestHeartbeatMessage()
    {
        HeartbeatMessage original;

        original.uptimeMs =
            123456789ull;

        const auto packet =
            HubMessageCodec::Encode(
                HubMessage{
                    original
                },
                102
            );

        if (!packet.has_value())
        {
            return false;
        }

        const auto decoded =
            HubMessageCodec::Decode(
                *packet
            );

        if (!decoded.has_value())
        {
            return false;
        }

        const auto* result =
            std::get_if<
            HeartbeatMessage
            >(
                &*decoded
            );

        return
            result != nullptr &&
            result->uptimeMs ==
            original.uptimeMs;
    }

    bool TestInputEventMessage()
    {
        InputEvent original;

        original.nodeId =
            0x1234;

        original.controlId =
            0x5678;

        original.type =
            InputEventType::
            AxisValue;

        original.value =
            -123456789;

        original.timestamp =
            0x0102030405060708ull;

        const auto packet =
            HubMessageCodec::Encode(
                HubMessage{
                    original
                },
                103
            );

        if (!packet.has_value())
        {
            return false;
        }

        if (
            packet->messageType !=
            HubMessageType::InputEvent ||
            packet->payload.size() !=
            InputEventPayloadSize
            )
        {
            return false;
        }

        //
        // Verify the actual v1 wire layout,
        // not merely encode/decode symmetry.
        //

        const auto& payload =
            packet->payload;

        if (
            payload[0] != 0x34 ||
            payload[1] != 0x12 ||
            payload[2] != 0x78 ||
            payload[3] != 0x56
            )
        {
            return false;
        }

        if (
            payload[4] !=
            static_cast<std::uint8_t>(
                InputEventType::
                AxisValue
                )
            )
        {
            return false;
        }

        if (
            payload[5] != 0 ||
            payload[6] != 0 ||
            payload[7] != 0
            )
        {
            return false;
        }

        if (
            payload[12] != 0x08 ||
            payload[13] != 0x07 ||
            payload[14] != 0x06 ||
            payload[15] != 0x05 ||
            payload[16] != 0x04 ||
            payload[17] != 0x03 ||
            payload[18] != 0x02 ||
            payload[19] != 0x01
            )
        {
            return false;
        }

        const auto decoded =
            HubMessageCodec::Decode(
                *packet
            );

        if (!decoded.has_value())
        {
            return false;
        }

        const auto* result =
            std::get_if<InputEvent>(
                &*decoded
            );

        if (result == nullptr)
        {
            return false;
        }

        return
            result->nodeId ==
            original.nodeId &&
            result->controlId ==
            original.controlId &&
            result->type ==
            original.type &&
            result->value ==
            original.value &&
            result->timestamp ==
            original.timestamp;
    }

    bool TestAllInputEventTypes()
    {
        const InputEventType types[]
        {
            InputEventType::ButtonDown,
            InputEventType::ButtonUp,
            InputEventType::EncoderDelta,
            InputEventType::AxisValue
        };

        std::uint32_t sequence =
            200;

        for (
            const auto type :
            types
            )
        {
            InputEvent original;

            original.nodeId =
                7;

            original.controlId =
                9;

            original.type =
                type;

            original.value =
                -42;

            original.timestamp =
                123456;

            const auto packet =
                HubMessageCodec::Encode(
                    HubMessage{
                        original
                    },
                    sequence++
                );

            if (!packet.has_value())
            {
                return false;
            }

            const auto decoded =
                HubMessageCodec::Decode(
                    *packet
                );

            if (!decoded.has_value())
            {
                return false;
            }

            const auto* result =
                std::get_if<InputEvent>(
                    &*decoded
                );

            if (
                result == nullptr ||
                result->type !=
                type
                )
            {
                return false;
            }
        }

        return true;
    }

    bool TestUnknownInputEventRejected()
    {
        InputEvent event;

        event.nodeId =
            1;

        event.controlId =
            1;

        event.type =
            InputEventType::
            Unknown;

        event.value =
            1;

        event.timestamp =
            1;

        const auto packet =
            HubMessageCodec::Encode(
                HubMessage{
                    event
                },
                300
            );

        return
            !packet.has_value();
    }

    bool TestMalformedPayloadRejected()
    {
        HubPacket packet;

        packet.messageType =
            HubMessageType::
            InputEvent;

        packet.sequence =
            301;

        packet.payload.resize(
            InputEventPayloadSize - 1,
            0
        );

        return
            !HubMessageCodec::Decode(
                packet
            )
            .has_value();
    }

    bool TestReservedBytesRejected()
    {
        InputEvent event;

        event.nodeId =
            1;

        event.controlId =
            2;

        event.type =
            InputEventType::
            ButtonDown;

        event.value =
            1;

        event.timestamp =
            999;

        auto packet =
            HubMessageCodec::Encode(
                HubMessage{
                    event
                },
                302
            );

        if (!packet.has_value())
        {
            return false;
        }

        packet->payload[5] =
            1;

        return
            !HubMessageCodec::Decode(
                *packet
            )
            .has_value();
    }

    bool TestFullFramingRoundTrip()
    {
        InputEvent original;

        original.nodeId =
            4;

        original.controlId =
            12;

        original.type =
            InputEventType::
            EncoderDelta;

        original.value =
            -3;

        original.timestamp =
            987654321ull;

        const auto packet =
            HubMessageCodec::Encode(
                HubMessage{
                    original
                },
                0x12345678u
            );

        if (!packet.has_value())
        {
            return false;
        }

        const auto bytes =
            HubProtocol::Encode(
                *packet
            );

        if (bytes.empty())
        {
            return false;
        }

        HubStreamDecoder
            streamDecoder;

        std::vector<HubPacket>
            decodedPackets;

        //
        // Simulate arbitrary USB CDC reads:
        // 1 byte, then 2, then 3, then the rest.
        //

        std::size_t offset =
            0;

        const std::size_t chunks[]
        {
            1,
            2,
            3
        };

        for (
            const std::size_t chunk :
        chunks
            )
        {
            const auto packets =
                streamDecoder.Push(
                    bytes.data() +
                    offset,
                    chunk
                );

            decodedPackets.insert(
                decodedPackets.end(),
                packets.begin(),
                packets.end()
            );

            offset +=
                chunk;
        }

        const auto finalPackets =
            streamDecoder.Push(
                bytes.data() +
                offset,
                bytes.size() -
                offset
            );

        decodedPackets.insert(
            decodedPackets.end(),
            finalPackets.begin(),
            finalPackets.end()
        );

        if (
            decodedPackets.size() !=
            1
            )
        {
            return false;
        }

        const auto message =
            HubMessageCodec::Decode(
                decodedPackets[0]
            );

        if (!message.has_value())
        {
            return false;
        }

        const auto* result =
            std::get_if<InputEvent>(
                &*message
            );

        if (result == nullptr)
        {
            return false;
        }

        return
            result->nodeId ==
            original.nodeId &&
            result->controlId ==
            original.controlId &&
            result->type ==
            original.type &&
            result->value ==
            original.value &&
            result->timestamp ==
            original.timestamp;
    }
}

namespace srh::smoketest
{
    bool RunHubMessageSmokeTest()
    {
        if (!TestHelloMessage())
        {
            std::cout
                << "FAIL: Hub Hello message\n";

            return false;
        }

        if (!TestHelloAckMessage())
        {
            std::cout
                << "FAIL: Hub HelloAck message\n";

            return false;
        }

        if (!TestHeartbeatMessage())
        {
            std::cout
                << "FAIL: Hub Heartbeat message\n";

            return false;
        }

        if (!TestInputEventMessage())
        {
            std::cout
                << "FAIL: Hub InputEvent message\n";

            return false;
        }

        if (!TestAllInputEventTypes())
        {
            std::cout
                << "FAIL: Hub InputEvent types\n";

            return false;
        }

        if (!TestUnknownInputEventRejected())
        {
            std::cout
                << "FAIL: Hub unknown InputEvent rejection\n";

            return false;
        }

        if (!TestMalformedPayloadRejected())
        {
            std::cout
                << "FAIL: Hub malformed payload rejection\n";

            return false;
        }

        if (!TestReservedBytesRejected())
        {
            std::cout
                << "FAIL: Hub reserved payload rejection\n";

            return false;
        }

        if (!TestFullFramingRoundTrip())
        {
            std::cout
                << "FAIL: Hub message framing round trip\n";

            return false;
        }

        return true;
    }
}