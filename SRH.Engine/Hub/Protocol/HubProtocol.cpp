#include "pch.h"

#include "Hub/Protocol/HubProtocol.h"

#include <algorithm>
#include <array>
#include <limits>

namespace
{
    constexpr std::array<
        std::uint8_t,
        4
    > Magic
    {
        static_cast<std::uint8_t>('S'),
        static_cast<std::uint8_t>('R'),
        static_cast<std::uint8_t>('H'),
        static_cast<std::uint8_t>('P')
    };

    constexpr std::size_t
        VersionOffset = 4;

    constexpr std::size_t
        MessageTypeOffset = 5;

    constexpr std::size_t
        FlagsOffset = 6;

    constexpr std::size_t
        SequenceOffset = 8;

    constexpr std::size_t
        PayloadLengthOffset = 12;

    constexpr std::size_t
        ReservedOffset = 14;

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

    bool StartsWithMagic(
        const std::vector<std::uint8_t>& buffer
    )
    {
        if (
            buffer.size() <
            Magic.size()
            )
        {
            return false;
        }

        return
            std::equal(
                Magic.begin(),
                Magic.end(),
                buffer.begin()
            );
    }
}

namespace srh::engine::hub::protocol
{
    std::vector<std::uint8_t>
        HubProtocol::Encode(
            const HubPacket& packet
        )
    {
        if (
            packet.payload.size() >
            HubPacketMaximumPayloadSize
            )
        {
            return {};
        }

        if (
            packet.payload.size() >
            static_cast<std::size_t>(
                std::numeric_limits<
                std::uint16_t
                >::max()
                )
            )
        {
            return {};
        }

        const std::size_t totalSize =
            HubPacketHeaderSize +
            packet.payload.size() +
            HubPacketCrcSize;

        std::vector<std::uint8_t>
            encoded(
                totalSize,
                0
            );

        std::copy(
            Magic.begin(),
            Magic.end(),
            encoded.begin()
        );

        encoded[
            VersionOffset
        ] =
            packet.protocolVersion;

            encoded[
                MessageTypeOffset
            ] =
                static_cast<std::uint8_t>(
                    packet.messageType
                    );

                WriteUInt16Le(
                    encoded,
                    FlagsOffset,
                    static_cast<std::uint16_t>(
                        packet.flags
                        )
                );

                WriteUInt32Le(
                    encoded,
                    SequenceOffset,
                    packet.sequence
                );

                WriteUInt16Le(
                    encoded,
                    PayloadLengthOffset,
                    static_cast<std::uint16_t>(
                        packet.payload.size()
                        )
                );

                //
                // Reserved.
                //

                WriteUInt16Le(
                    encoded,
                    ReservedOffset,
                    0
                );

                if (!packet.payload.empty())
                {
                    std::copy(
                        packet.payload.begin(),
                        packet.payload.end(),
                        encoded.begin() +
                        HubPacketHeaderSize
                    );
                }

                //
                // CRC covers:
                //
                // Version
                // MessageType
                // Flags
                // Sequence
                // PayloadLength
                // Reserved
                // Payload
                //
                // Magic is deliberately excluded.
                //

                const std::size_t crcDataOffset =
                    VersionOffset;

                const std::size_t crcDataSize =
                    (
                        HubPacketHeaderSize -
                        VersionOffset
                        ) +
                    packet.payload.size();

                const std::uint32_t crc =
                    CalculateCrc32(
                        encoded.data() +
                        crcDataOffset,
                        crcDataSize
                    );

                const std::size_t crcOffset =
                    HubPacketHeaderSize +
                    packet.payload.size();

                WriteUInt32Le(
                    encoded,
                    crcOffset,
                    crc
                );

                return encoded;
    }

    std::uint32_t
        HubProtocol::CalculateCrc32(
            const std::uint8_t* data,
            const std::size_t size
        ) noexcept
    {
        if (
            data == nullptr &&
            size != 0
            )
        {
            return 0;
        }

        std::uint32_t crc =
            0xFFFFFFFFu;

        for (
            std::size_t index = 0;
            index < size;
            ++index
            )
        {
            crc ^=
                static_cast<std::uint32_t>(
                    data[index]
                    );

            for (
                int bit = 0;
                bit < 8;
                ++bit
                )
            {
                if (
                    (crc & 1u) != 0
                    )
                {
                    crc =
                        (crc >> 1) ^
                        0xEDB88320u;
                }
                else
                {
                    crc >>=
                        1;
                }
            }
        }

        return
            crc ^
            0xFFFFFFFFu;
    }

    bool HubProtocol::IsValidMessageType(
        const HubMessageType type
    ) noexcept
    {
        switch (type)
        {
        case HubMessageType::Hello:
        case HubMessageType::HelloAck:
        case HubMessageType::Heartbeat:

        case HubMessageType::HubState:
        case HubMessageType::RequestState:

        case HubMessageType::NodeDiscovered:
        case HubMessageType::NodeRemoved:
        case HubMessageType::NodeState:

        case HubMessageType::InputEvent:

        case HubMessageType::HubAction:
        case HubMessageType::NodeCommand:
        case HubMessageType::SetConfiguration:

        case HubMessageType::Error:
            return true;

        case HubMessageType::Unknown:
        default:
            return false;
        }
    }

    bool HubProtocol::IsSupportedVersion(
        const std::uint8_t version
    ) noexcept
    {
        return
            version ==
            HubProtocolVersion;
    }

    std::vector<HubPacket>
        HubStreamDecoder::Push(
            const std::uint8_t* data,
            const std::size_t size
        )
    {
        std::vector<HubPacket>
            packets;

        if (
            data == nullptr &&
            size != 0
            )
        {
            return packets;
        }

        if (size != 0)
        {
            m_buffer.insert(
                m_buffer.end(),
                data,
                data + size
            );
        }

        for (;;)
        {
            if (
                !AlignToMagic()
                )
            {
                break;
            }

            if (
                m_buffer.size() <
                HubPacketHeaderSize
                )
            {
                break;
            }

            const auto version =
                m_buffer[
                    VersionOffset
                ];

            const auto messageType =
                static_cast<HubMessageType>(
                    m_buffer[
                        MessageTypeOffset
                    ]
                    );

            const auto payloadLength =
                static_cast<std::size_t>(
                    ReadUInt16Le(
                        m_buffer.data() +
                        PayloadLengthOffset
                    )
                    );

            if (
                payloadLength >
                HubPacketMaximumPayloadSize
                )
            {
                ++m_invalidPacketCount;

                DiscardFront(
                    1
                );

                continue;
            }

            const std::size_t frameSize =
                HubPacketHeaderSize +
                payloadLength +
                HubPacketCrcSize;

            if (
                m_buffer.size() <
                frameSize
                )
            {
                break;
            }

            if (
                !HubProtocol::
                IsSupportedVersion(
                    version
                ) ||
                !HubProtocol::
                IsValidMessageType(
                    messageType
                )
                )
            {
                ++m_invalidPacketCount;

                DiscardFront(
                    1
                );

                continue;
            }

            const std::size_t crcOffset =
                HubPacketHeaderSize +
                payloadLength;

            const std::uint32_t storedCrc =
                ReadUInt32Le(
                    m_buffer.data() +
                    crcOffset
                );

            const std::size_t crcDataOffset =
                VersionOffset;

            const std::size_t crcDataSize =
                (
                    HubPacketHeaderSize -
                    VersionOffset
                    ) +
                payloadLength;

            const std::uint32_t calculatedCrc =
                HubProtocol::
                CalculateCrc32(
                    m_buffer.data() +
                    crcDataOffset,
                    crcDataSize
                );

            if (
                storedCrc !=
                calculatedCrc
                )
            {
                ++m_invalidPacketCount;

                DiscardFront(
                    1
                );

                continue;
            }

            HubPacket packet;

            packet.protocolVersion =
                version;

            packet.messageType =
                messageType;

            packet.flags =
                static_cast<HubPacketFlags>(
                    ReadUInt16Le(
                        m_buffer.data() +
                        FlagsOffset
                    )
                    );

            packet.sequence =
                ReadUInt32Le(
                    m_buffer.data() +
                    SequenceOffset
                );

            if (payloadLength != 0)
            {
                const auto payloadBegin =
                    m_buffer.begin() +
                    HubPacketHeaderSize;

                packet.payload.assign(
                    payloadBegin,
                    payloadBegin +
                    payloadLength
                );
            }

            packets.push_back(
                std::move(
                    packet
                )
            );

            DiscardFront(
                frameSize
            );
        }

        return packets;
    }

    std::vector<HubPacket>
        HubStreamDecoder::Push(
            const std::vector<std::uint8_t>& data
        )
    {
        return
            Push(
                data.data(),
                data.size()
            );
    }

    void HubStreamDecoder::Reset()
    {
        m_buffer.clear();

        m_droppedByteCount =
            0;

        m_invalidPacketCount =
            0;
    }

    std::size_t
        HubStreamDecoder::
        BufferedByteCount() const
        noexcept
    {
        return
            m_buffer.size();
    }

    std::uint64_t
        HubStreamDecoder::
        DroppedByteCount() const
        noexcept
    {
        return
            m_droppedByteCount;
    }

    std::uint64_t
        HubStreamDecoder::
        InvalidPacketCount() const
        noexcept
    {
        return
            m_invalidPacketCount;
    }

    void HubStreamDecoder::DiscardFront(
        const std::size_t count
    )
    {
        if (count == 0)
        {
            return;
        }

        const std::size_t actualCount =
            std::min(
                count,
                m_buffer.size()
            );

        m_buffer.erase(
            m_buffer.begin(),
            m_buffer.begin() +
            actualCount
        );

        m_droppedByteCount +=
            actualCount;
    }

    bool HubStreamDecoder::AlignToMagic()
    {
        if (
            m_buffer.size() <
            Magic.size()
            )
        {
            return false;
        }

        if (
            StartsWithMagic(
                m_buffer
            )
            )
        {
            return true;
        }

        const auto iterator =
            std::search(
                m_buffer.begin() + 1,
                m_buffer.end(),
                Magic.begin(),
                Magic.end()
            );

        if (
            iterator !=
            m_buffer.end()
            )
        {
            const auto count =
                static_cast<std::size_t>(
                    std::distance(
                        m_buffer.begin(),
                        iterator
                    )
                    );

            DiscardFront(
                count
            );

            return true;
        }

        //
        // Keep at most the final 3 bytes because
        // they may be the beginning of "SRHP"
        // split across two USB reads.
        //

        constexpr std::size_t
            PrefixRetention =
            Magic.size() - 1;

        if (
            m_buffer.size() >
            PrefixRetention
            )
        {
            DiscardFront(
                m_buffer.size() -
                PrefixRetention
            );
        }

        return false;
    }
}