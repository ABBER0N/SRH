#include "HubProtocolSmokeTest.h"

#include "Hub/Protocol/HubProtocol.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
    using srh::engine::HubMessageType;
    using srh::engine::HubPacketFlags;

    using srh::engine::hub::protocol::
        HubPacket;

    using srh::engine::hub::protocol::
        HubProtocol;

    using srh::engine::hub::protocol::
        HubStreamDecoder;

    bool PacketsEqual(
        const HubPacket& left,
        const HubPacket& right
    )
    {
        return
            left.protocolVersion ==
            right.protocolVersion &&
            left.messageType ==
            right.messageType &&
            left.flags ==
            right.flags &&
            left.sequence ==
            right.sequence &&
            left.payload ==
            right.payload;
    }

    void AppendPackets(
        std::vector<HubPacket>& destination,
        std::vector<HubPacket> source
    )
    {
        destination.insert(
            destination.end(),
            std::make_move_iterator(
                source.begin()
            ),
            std::make_move_iterator(
                source.end()
            )
        );
    }

    bool TestKnownCrc32()
    {
        //
        // Standard CRC-32/IEEE test vector.
        //
        // "123456789" must produce:
        //
        // 0xCBF43926
        //

        constexpr std::array<
            std::uint8_t,
            9
        > TestVector
        {
            static_cast<std::uint8_t>('1'),
            static_cast<std::uint8_t>('2'),
            static_cast<std::uint8_t>('3'),
            static_cast<std::uint8_t>('4'),
            static_cast<std::uint8_t>('5'),
            static_cast<std::uint8_t>('6'),
            static_cast<std::uint8_t>('7'),
            static_cast<std::uint8_t>('8'),
            static_cast<std::uint8_t>('9')
        };

        const auto crc =
            HubProtocol::CalculateCrc32(
                TestVector.data(),
                TestVector.size()
            );

        return
            crc ==
            0xCBF43926u;
    }

    HubPacket CreateTestPacket()
    {
        HubPacket packet;

        packet.protocolVersion =
            srh::engine::
            HubProtocolVersion;

        packet.messageType =
            HubMessageType::
            InputEvent;

        packet.flags =
            HubPacketFlags::
            ResponseRequired;

        packet.sequence =
            0x12345678u;

        packet.payload =
        {
            0x01,
            0x02,
            0x03,
            0x10,
            0x20,
            0x30,
            0x40,
            0x7F,
            0x80,
            0xFF
        };

        return packet;
    }

    HubPacket CreateSecondPacket()
    {
        HubPacket packet;

        packet.protocolVersion =
            srh::engine::
            HubProtocolVersion;

        packet.messageType =
            HubMessageType::
            Heartbeat;

        packet.flags =
            HubPacketFlags::
            None;

        packet.sequence =
            0x01020304u;

        return packet;
    }

    HubPacket CreateThirdPacket()
    {
        HubPacket packet;

        packet.protocolVersion =
            srh::engine::
            HubProtocolVersion;

        packet.messageType =
            HubMessageType::
            Error;

        packet.flags =
            HubPacketFlags::
            Response |
            HubPacketFlags::
            Error;

        packet.sequence =
            0xAABBCCDDu;

        packet.payload =
        {
            0x11,
            0x22,
            0x33,
            0x44
        };

        return packet;
    }

    bool TestEncodeAndSingleDecode()
    {
        const auto original =
            CreateTestPacket();

        const auto encoded =
            HubProtocol::Encode(
                original
            );

        if (encoded.empty())
        {
            return false;
        }

        const std::size_t
            expectedSize =
            srh::engine::hub::
            protocol::
            HubPacketHeaderSize +
            original.payload.size() +
            srh::engine::hub::
            protocol::
            HubPacketCrcSize;

        if (
            encoded.size() !=
            expectedSize
            )
        {
            return false;
        }

        HubStreamDecoder decoder;

        const auto decoded =
            decoder.Push(
                encoded
            );

        if (
            decoded.size() !=
            1
            )
        {
            return false;
        }

        if (
            !PacketsEqual(
                original,
                decoded[0]
            )
            )
        {
            return false;
        }

        if (
            decoder.BufferedByteCount() !=
            0
            )
        {
            return false;
        }

        if (
            decoder.InvalidPacketCount() !=
            0
            )
        {
            return false;
        }

        return true;
    }

    bool TestOneByteAtATime()
    {
        const auto original =
            CreateTestPacket();

        const auto encoded =
            HubProtocol::Encode(
                original
            );

        if (encoded.empty())
        {
            return false;
        }

        HubStreamDecoder decoder;

        std::vector<HubPacket>
            decoded;

        for (
            const std::uint8_t byte :
        encoded
            )
        {
            const auto packets =
                decoder.Push(
                    &byte,
                    1
                );

            AppendPackets(
                decoded,
                packets
            );
        }

        if (
            decoded.size() !=
            1
            )
        {
            return false;
        }

        if (
            !PacketsEqual(
                original,
                decoded[0]
            )
            )
        {
            return false;
        }

        return
            decoder.BufferedByteCount() ==
            0 &&
            decoder.InvalidPacketCount() ==
            0;
    }

    bool TestMultiplePacketsInOneChunk()
    {
        const auto first =
            CreateTestPacket();

        const auto second =
            CreateSecondPacket();

        const auto third =
            CreateThirdPacket();

        const auto firstBytes =
            HubProtocol::Encode(
                first
            );

        const auto secondBytes =
            HubProtocol::Encode(
                second
            );

        const auto thirdBytes =
            HubProtocol::Encode(
                third
            );

        if (
            firstBytes.empty() ||
            secondBytes.empty() ||
            thirdBytes.empty()
            )
        {
            return false;
        }

        std::vector<std::uint8_t>
            stream;

        stream.reserve(
            firstBytes.size() +
            secondBytes.size() +
            thirdBytes.size()
        );

        stream.insert(
            stream.end(),
            firstBytes.begin(),
            firstBytes.end()
        );

        stream.insert(
            stream.end(),
            secondBytes.begin(),
            secondBytes.end()
        );

        stream.insert(
            stream.end(),
            thirdBytes.begin(),
            thirdBytes.end()
        );

        HubStreamDecoder decoder;

        const auto decoded =
            decoder.Push(
                stream
            );

        if (
            decoded.size() !=
            3
            )
        {
            return false;
        }

        return
            PacketsEqual(
                first,
                decoded[0]
            ) &&
            PacketsEqual(
                second,
                decoded[1]
            ) &&
            PacketsEqual(
                third,
                decoded[2]
            ) &&
            decoder.BufferedByteCount() ==
            0;
    }

    bool TestSplitMagic()
    {
        const auto original =
            CreateTestPacket();

        const auto encoded =
            HubProtocol::Encode(
                original
            );

        if (
            encoded.size() <
            4
            )
        {
            return false;
        }

        HubStreamDecoder decoder;

        //
        // First USB read ends halfway through:
        //
        // "SR"
        //

        const auto firstPart =
            decoder.Push(
                encoded.data(),
                2
            );

        if (!firstPart.empty())
        {
            return false;
        }

        //
        // Second USB read starts with:
        //
        // "HP..."
        //

        const auto secondPart =
            decoder.Push(
                encoded.data() + 2,
                encoded.size() - 2
            );

        if (
            secondPart.size() !=
            1
            )
        {
            return false;
        }

        return
            PacketsEqual(
                original,
                secondPart[0]
            );
    }

    bool TestGarbageBeforePacket()
    {
        const auto original =
            CreateTestPacket();

        const auto encoded =
            HubProtocol::Encode(
                original
            );

        if (encoded.empty())
        {
            return false;
        }

        std::vector<std::uint8_t>
            stream
        {
            0xDE,
            0xAD,
            0xBE,
            0xEF,
            0x01,
            0x02,
            0x03
        };

        stream.insert(
            stream.end(),
            encoded.begin(),
            encoded.end()
        );

        HubStreamDecoder decoder;

        const auto decoded =
            decoder.Push(
                stream
            );

        if (
            decoded.size() !=
            1
            )
        {
            return false;
        }

        if (
            !PacketsEqual(
                original,
                decoded[0]
            )
            )
        {
            return false;
        }

        if (
            decoder.DroppedByteCount() <
            7
            )
        {
            return false;
        }

        return true;
    }

    bool TestGarbageAndSplitMagic()
    {
        const auto original =
            CreateTestPacket();

        const auto encoded =
            HubProtocol::Encode(
                original
            );

        if (
            encoded.size() <
            4
            )
        {
            return false;
        }

        HubStreamDecoder decoder;

        //
        // Garbage followed by only "SR".
        //

        const std::array<
            std::uint8_t,
            4
        > firstChunk
        {
            0xAA,
            0xBB,
            static_cast<std::uint8_t>('S'),
            static_cast<std::uint8_t>('R')
        };

        const auto firstResult =
            decoder.Push(
                firstChunk.data(),
                firstChunk.size()
            );

        if (!firstResult.empty())
        {
            return false;
        }

        //
        // Continue the original frame from "H".
        //

        const auto secondResult =
            decoder.Push(
                encoded.data() + 2,
                encoded.size() - 2
            );

        if (
            secondResult.size() !=
            1
            )
        {
            return false;
        }

        if (
            !PacketsEqual(
                original,
                secondResult[0]
            )
            )
        {
            return false;
        }

        return
            decoder.DroppedByteCount() >=
            2;
    }

    bool TestCorruptedCrcRecovery()
    {
        const auto corruptedPacket =
            CreateTestPacket();

        const auto recoveryPacket =
            CreateSecondPacket();

        auto corruptedBytes =
            HubProtocol::Encode(
                corruptedPacket
            );

        const auto recoveryBytes =
            HubProtocol::Encode(
                recoveryPacket
            );

        if (
            corruptedBytes.empty() ||
            recoveryBytes.empty()
            )
        {
            return false;
        }

        //
        // Corrupt one payload byte without updating
        // the CRC.
        //

        const std::size_t payloadOffset =
            srh::engine::hub::
            protocol::
            HubPacketHeaderSize;

        if (
            payloadOffset >=
            corruptedBytes.size()
            )
        {
            return false;
        }

        corruptedBytes[
            payloadOffset
        ] ^=
            0x80u;

            std::vector<std::uint8_t>
                stream;

            stream.reserve(
                corruptedBytes.size() +
                recoveryBytes.size()
            );

            stream.insert(
                stream.end(),
                corruptedBytes.begin(),
                corruptedBytes.end()
            );

            stream.insert(
                stream.end(),
                recoveryBytes.begin(),
                recoveryBytes.end()
            );

            HubStreamDecoder decoder;

            const auto decoded =
                decoder.Push(
                    stream
                );

            //
            // Corrupted packet must be rejected, but
            // the following valid frame must survive.
            //

            if (
                decoded.size() !=
                1
                )
            {
                return false;
            }

            if (
                !PacketsEqual(
                    recoveryPacket,
                    decoded[0]
                )
                )
            {
                return false;
            }

            if (
                decoder.InvalidPacketCount() <
                1
                )
            {
                return false;
            }

            if (
                decoder.DroppedByteCount() ==
                0
                )
            {
                return false;
            }

            return true;
    }

    bool TestUnsupportedVersionRecovery()
    {
        auto invalidPacket =
            CreateTestPacket();

        invalidPacket.protocolVersion =
            99;

        const auto validPacket =
            CreateSecondPacket();

        const auto invalidBytes =
            HubProtocol::Encode(
                invalidPacket
            );

        const auto validBytes =
            HubProtocol::Encode(
                validPacket
            );

        if (
            invalidBytes.empty() ||
            validBytes.empty()
            )
        {
            return false;
        }

        std::vector<std::uint8_t>
            stream;

        stream.reserve(
            invalidBytes.size() +
            validBytes.size()
        );

        stream.insert(
            stream.end(),
            invalidBytes.begin(),
            invalidBytes.end()
        );

        stream.insert(
            stream.end(),
            validBytes.begin(),
            validBytes.end()
        );

        HubStreamDecoder decoder;

        const auto decoded =
            decoder.Push(
                stream
            );

        if (
            decoded.size() !=
            1
            )
        {
            return false;
        }

        if (
            !PacketsEqual(
                validPacket,
                decoded[0]
            )
            )
        {
            return false;
        }

        return
            decoder.InvalidPacketCount() >=
            1;
    }

    bool TestInvalidMessageTypeRecovery()
    {
        auto invalidPacket =
            CreateTestPacket();

        invalidPacket.messageType =
            static_cast<HubMessageType>(
                199
                );

        const auto validPacket =
            CreateSecondPacket();

        const auto invalidBytes =
            HubProtocol::Encode(
                invalidPacket
            );

        const auto validBytes =
            HubProtocol::Encode(
                validPacket
            );

        if (
            invalidBytes.empty() ||
            validBytes.empty()
            )
        {
            return false;
        }

        std::vector<std::uint8_t>
            stream;

        stream.reserve(
            invalidBytes.size() +
            validBytes.size()
        );

        stream.insert(
            stream.end(),
            invalidBytes.begin(),
            invalidBytes.end()
        );

        stream.insert(
            stream.end(),
            validBytes.begin(),
            validBytes.end()
        );

        HubStreamDecoder decoder;

        const auto decoded =
            decoder.Push(
                stream
            );

        if (
            decoded.size() !=
            1
            )
        {
            return false;
        }

        return
            PacketsEqual(
                validPacket,
                decoded[0]
            ) &&
            decoder.InvalidPacketCount() >=
            1;
    }

    bool TestMaximumPayload()
    {
        HubPacket packet;

        packet.messageType =
            HubMessageType::
            SetConfiguration;

        packet.sequence =
            42;

        packet.payload.resize(
            srh::engine::hub::
            protocol::
            HubPacketMaximumPayloadSize,
            0x5Au
        );

        const auto encoded =
            HubProtocol::Encode(
                packet
            );

        if (encoded.empty())
        {
            return false;
        }

        HubStreamDecoder decoder;

        const auto decoded =
            decoder.Push(
                encoded
            );

        if (
            decoded.size() !=
            1
            )
        {
            return false;
        }

        return
            PacketsEqual(
                packet,
                decoded[0]
            );
    }

    bool TestOversizedPayloadRejected()
    {
        HubPacket packet;

        packet.messageType =
            HubMessageType::
            SetConfiguration;

        packet.sequence =
            43;

        packet.payload.resize(
            srh::engine::hub::
            protocol::
            HubPacketMaximumPayloadSize +
            1,
            0x5Au
        );

        const auto encoded =
            HubProtocol::Encode(
                packet
            );

        return
            encoded.empty();
    }

    bool TestReset()
    {
        const auto packet =
            CreateTestPacket();

        const auto encoded =
            HubProtocol::Encode(
                packet
            );

        if (
            encoded.size() <
            8
            )
        {
            return false;
        }

        HubStreamDecoder decoder;

        const auto result =
            decoder.Push(
                encoded.data(),
                8
            );

        if (!result.empty())
        {
            return false;
        }

        if (
            decoder.BufferedByteCount() ==
            0
            )
        {
            return false;
        }

        decoder.Reset();

        return
            decoder.BufferedByteCount() ==
            0 &&
            decoder.DroppedByteCount() ==
            0 &&
            decoder.InvalidPacketCount() ==
            0;
    }
}

namespace srh::smoketest
{
    bool RunHubProtocolSmokeTest()
    {
        if (!TestKnownCrc32())
        {
            std::cout
                << "FAIL: Hub protocol CRC-32\n";

            return false;
        }

        if (!TestEncodeAndSingleDecode())
        {
            std::cout
                << "FAIL: Hub protocol encode/decode\n";

            return false;
        }

        if (!TestOneByteAtATime())
        {
            std::cout
                << "FAIL: Hub protocol fragmented stream\n";

            return false;
        }

        if (!TestMultiplePacketsInOneChunk())
        {
            std::cout
                << "FAIL: Hub protocol multi-packet stream\n";

            return false;
        }

        if (!TestSplitMagic())
        {
            std::cout
                << "FAIL: Hub protocol split magic\n";

            return false;
        }

        if (!TestGarbageBeforePacket())
        {
            std::cout
                << "FAIL: Hub protocol stream resync\n";

            return false;
        }

        if (!TestGarbageAndSplitMagic())
        {
            std::cout
                << "FAIL: Hub protocol partial resync\n";

            return false;
        }

        if (!TestCorruptedCrcRecovery())
        {
            std::cout
                << "FAIL: Hub protocol CRC recovery\n";

            return false;
        }

        if (!TestUnsupportedVersionRecovery())
        {
            std::cout
                << "FAIL: Hub protocol version recovery\n";

            return false;
        }

        if (!TestInvalidMessageTypeRecovery())
        {
            std::cout
                << "FAIL: Hub protocol message recovery\n";

            return false;
        }

        if (!TestMaximumPayload())
        {
            std::cout
                << "FAIL: Hub protocol maximum payload\n";

            return false;
        }

        if (!TestOversizedPayloadRejected())
        {
            std::cout
                << "FAIL: Hub protocol oversized payload\n";

            return false;
        }

        if (!TestReset())
        {
            std::cout
                << "FAIL: Hub protocol decoder reset\n";

            return false;
        }

        return true;
    }
}