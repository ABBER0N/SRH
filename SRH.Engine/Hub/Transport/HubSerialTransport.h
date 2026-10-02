#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace srh::engine::hub
{
    class HubSerialTransport
    {
    public:
        static constexpr std::uint32_t
            DefaultBaudRate = 115200;

        HubSerialTransport();

        ~HubSerialTransport();

        HubSerialTransport(
            const HubSerialTransport&
        ) = delete;

        HubSerialTransport& operator=(
            const HubSerialTransport&
            ) = delete;

        HubSerialTransport(
            HubSerialTransport&&
        ) noexcept;

        HubSerialTransport& operator=(
            HubSerialTransport&&
            ) noexcept;

        //
        // portName is UTF-8.
        //
        // Examples:
        //
        // "COM3"
        // "COM12"
        //
        // The Windows \\.\ prefix is added
        // automatically.
        //

        [[nodiscard]]
        bool Open(
            std::string_view portName,
            std::uint32_t baudRate =
            DefaultBaudRate
        );

        void Close()
            noexcept;

        [[nodiscard]]
        bool IsOpen() const
            noexcept;

        //
        // Reads up to capacity bytes.
        //
        // true:
        //   operation completed normally.
        //   bytesRead may be zero when no data was
        //   available during the configured timeout.
        //
        // false:
        //   Windows I/O error occurred.
        //

        [[nodiscard]]
        bool ReadSome(
            std::uint8_t* buffer,
            std::size_t capacity,
            std::size_t& bytesRead
        ) noexcept;

        //
        // Writes the entire supplied buffer.
        //
        // Returns false if the complete buffer
        // cannot be written.
        //

        [[nodiscard]]
        bool WriteAll(
            const std::uint8_t* data,
            std::size_t size
        ) noexcept;

        [[nodiscard]]
        std::uint32_t LastError() const
            noexcept;

        [[nodiscard]]
        std::string PortName() const;

    private:
        class Impl;

        std::unique_ptr<Impl>
            m_impl;
    };
}