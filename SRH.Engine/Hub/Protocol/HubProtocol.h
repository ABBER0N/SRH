#pragma once

#include "Hub/Protocol/HubPacket.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace srh::engine::hub::protocol
{
    inline constexpr std::size_t
        HubPacketHeaderSize = 16;

    inline constexpr std::size_t
        HubPacketCrcSize = 4;

    inline constexpr std::size_t
        HubPacketMinimumSize =
        HubPacketHeaderSize +
        HubPacketCrcSize;

    inline constexpr std::size_t
        HubPacketMaximumPayloadSize =
        4096;

    class HubProtocol
    {
    public:
        [[nodiscard]]
        static std::vector<std::uint8_t>
            Encode(
                const HubPacket& packet
            );

        [[nodiscard]]
        static std::uint32_t
            CalculateCrc32(
                const std::uint8_t* data,
                std::size_t size
            ) noexcept;

        [[nodiscard]]
        static bool IsValidMessageType(
            HubMessageType type
        ) noexcept;

        [[nodiscard]]
        static bool IsSupportedVersion(
            std::uint8_t version
        ) noexcept;
    };

    class HubStreamDecoder
    {
    public:
        HubStreamDecoder() = default;

        [[nodiscard]]
        std::vector<HubPacket> Push(
            const std::uint8_t* data,
            std::size_t size
        );

        [[nodiscard]]
        std::vector<HubPacket> Push(
            const std::vector<std::uint8_t>& data
        );

        void Reset();

        [[nodiscard]]
        std::size_t BufferedByteCount() const
            noexcept;

        [[nodiscard]]
        std::uint64_t DroppedByteCount() const
            noexcept;

        [[nodiscard]]
        std::uint64_t InvalidPacketCount() const
            noexcept;

    private:
        void DiscardFront(
            std::size_t count
        );

        [[nodiscard]]
        bool AlignToMagic();

        std::vector<std::uint8_t>
            m_buffer;

        std::uint64_t m_droppedByteCount{
            0
        };

        std::uint64_t m_invalidPacketCount{
            0
        };
    };
}