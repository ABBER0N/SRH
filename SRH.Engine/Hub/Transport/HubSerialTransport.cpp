#include "pch.h"

#include "Hub/Transport/HubSerialTransport.h"

#include "Execution/Windows/Text/Utf8.h"

#include <Windows.h>

//
// Windows.h defines legacy min/max macros unless
// NOMINMAX was active before the first Windows
// header was included.
//
// pch.h may already have included Windows headers,
// therefore defining NOMINMAX here would be too late.
// Remove the macros explicitly at this Windows API
// boundary.
//

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

namespace
{
    constexpr DWORD
        ReceiveBufferSize =
        64u * 1024u;

    constexpr DWORD
        TransmitBufferSize =
        64u * 1024u;

    //
    // ReadFile() may wait briefly for incoming
    // CDC data.
    //
    // The future Hub worker will repeatedly call
    // ReadSome(), so the timeout must remain short
    // enough for responsive shutdown/reconnect.
    //

    constexpr DWORD
        ReadTimeoutMs =
        50;

    constexpr DWORD
        WriteTimeoutMs =
        2000;

    std::wstring MakeWindowsPortPath(
        const std::string_view portName
    )
    {
        std::wstring wideName =
            srh::engine::windows::
            Utf8ToWide(
                portName
            );

        if (wideName.empty())
        {
            return {};
        }

        constexpr wchar_t
            DevicePrefix[] =
            L"\\\\.\\";

        constexpr std::size_t
            DevicePrefixLength =
            4;

        if (
            wideName.size() >=
            DevicePrefixLength &&
            wideName.compare(
                0,
                DevicePrefixLength,
                DevicePrefix
            ) == 0
            )
        {
            return wideName;
        }

        return
            std::wstring(
                DevicePrefix
            ) +
            wideName;
    }
}

namespace srh::engine::hub
{
    class HubSerialTransport::Impl
    {
    public:
        HANDLE handle{
            INVALID_HANDLE_VALUE
        };

        std::uint32_t lastError{
            ERROR_SUCCESS
        };

        std::string portName;

        void CloseHandleOnly()
            noexcept
        {
            if (
                handle ==
                INVALID_HANDLE_VALUE
                )
            {
                return;
            }

            ::CloseHandle(
                handle
            );

            handle =
                INVALID_HANDLE_VALUE;
        }
    };

    HubSerialTransport::
        HubSerialTransport()
        : m_impl(
            std::make_unique<Impl>()
        )
    {
    }

    HubSerialTransport::
        ~HubSerialTransport()
    {
        Close();
    }

    HubSerialTransport::
        HubSerialTransport(
            HubSerialTransport&&
        ) noexcept = default;

    HubSerialTransport&
        HubSerialTransport::operator=(
            HubSerialTransport&&
            ) noexcept = default;

    bool HubSerialTransport::Open(
        const std::string_view portName,
        const std::uint32_t baudRate
    )
    {
        //
        // A moved-from transport is still allowed
        // to be assigned/reused safely.
        //

        if (!m_impl)
        {
            m_impl =
                std::make_unique<Impl>();
        }

        Close();

        if (
            portName.empty() ||
            baudRate == 0
            )
        {
            m_impl->lastError =
                ERROR_INVALID_PARAMETER;

            return false;
        }

        const std::wstring portPath =
            MakeWindowsPortPath(
                portName
            );

        if (portPath.empty())
        {
            m_impl->lastError =
                ERROR_INVALID_NAME;

            return false;
        }

        HANDLE handle =
            ::CreateFileW(
                portPath.c_str(),
                GENERIC_READ |
                GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );

        if (
            handle ==
            INVALID_HANDLE_VALUE
            )
        {
            m_impl->lastError =
                ::GetLastError();

            return false;
        }

        //
        // Configure Windows-side serial buffers.
        //

        if (
            !::SetupComm(
                handle,
                ReceiveBufferSize,
                TransmitBufferSize
            )
            )
        {
            m_impl->lastError =
                ::GetLastError();

            ::CloseHandle(
                handle
            );

            return false;
        }

        DCB dcb{};

        dcb.DCBlength =
            sizeof(dcb);

        if (
            !::GetCommState(
                handle,
                &dcb
            )
            )
        {
            m_impl->lastError =
                ::GetLastError();

            ::CloseHandle(
                handle
            );

            return false;
        }

        dcb.BaudRate =
            baudRate;

        dcb.ByteSize =
            8;

        dcb.Parity =
            NOPARITY;

        dcb.StopBits =
            ONESTOPBIT;

        dcb.fBinary =
            TRUE;

        dcb.fParity =
            FALSE;

        //
        // SRH provides its own packet framing,
        // integrity checking and protocol state.
        //
        // Legacy UART flow control is not used.
        //

        dcb.fOutxCtsFlow =
            FALSE;

        dcb.fOutxDsrFlow =
            FALSE;

        dcb.fDsrSensitivity =
            FALSE;

        dcb.fTXContinueOnXoff =
            TRUE;

        dcb.fOutX =
            FALSE;

        dcb.fInX =
            FALSE;

        dcb.fErrorChar =
            FALSE;

        dcb.fNull =
            FALSE;

        dcb.fAbortOnError =
            FALSE;

        //
        // DTR/RTS are not part of SRH protocol
        // semantics.
        //
        // We may revisit the actual CDC line-state
        // policy when the ESP32-S3 hardware arrives,
        // because TinyUSB firmware can optionally
        // observe DTR/RTS.
        //

        dcb.fDtrControl =
            DTR_CONTROL_DISABLE;

        dcb.fRtsControl =
            RTS_CONTROL_DISABLE;

        if (
            !::SetCommState(
                handle,
                &dcb
            )
            )
        {
            m_impl->lastError =
                ::GetLastError();

            ::CloseHandle(
                handle
            );

            return false;
        }

        COMMTIMEOUTS timeouts{};

        //
        // ReadFile():
        //
        // - returns available bytes without waiting
        //   for the requested buffer to fill;
        //
        // - returns after ReadTimeoutMs if no bytes
        //   arrive.
        //

        timeouts.ReadIntervalTimeout =
            MAXDWORD;

        timeouts.ReadTotalTimeoutMultiplier =
            0;

        timeouts.ReadTotalTimeoutConstant =
            ReadTimeoutMs;

        timeouts.WriteTotalTimeoutMultiplier =
            0;

        timeouts.WriteTotalTimeoutConstant =
            WriteTimeoutMs;

        if (
            !::SetCommTimeouts(
                handle,
                &timeouts
            )
            )
        {
            m_impl->lastError =
                ::GetLastError();

            ::CloseHandle(
                handle
            );

            return false;
        }

        //
        // Remove stale bytes from a previous
        // connection before SRHP parsing begins.
        //

        if (
            !::PurgeComm(
                handle,
                PURGE_RXABORT |
                PURGE_RXCLEAR |
                PURGE_TXABORT |
                PURGE_TXCLEAR
            )
            )
        {
            m_impl->lastError =
                ::GetLastError();

            ::CloseHandle(
                handle
            );

            return false;
        }

        m_impl->handle =
            handle;

        m_impl->portName =
            std::string(
                portName
            );

        m_impl->lastError =
            ERROR_SUCCESS;

        return true;
    }

    void HubSerialTransport::Close()
        noexcept
    {
        if (!m_impl)
        {
            return;
        }

        m_impl->CloseHandleOnly();

        m_impl->portName.clear();

        m_impl->lastError =
            ERROR_SUCCESS;
    }

    bool HubSerialTransport::IsOpen() const
        noexcept
    {
        return
            m_impl &&
            m_impl->handle !=
            INVALID_HANDLE_VALUE;
    }

    bool HubSerialTransport::ReadSome(
        std::uint8_t* buffer,
        const std::size_t capacity,
        std::size_t& bytesRead
    ) noexcept
    {
        bytesRead =
            0;

        if (!m_impl)
        {
            return false;
        }

        if (
            capacity != 0 &&
            buffer == nullptr
            )
        {
            m_impl->lastError =
                ERROR_INVALID_PARAMETER;

            return false;
        }

        if (capacity == 0)
        {
            m_impl->lastError =
                ERROR_SUCCESS;

            return true;
        }

        if (
            m_impl->handle ==
            INVALID_HANDLE_VALUE
            )
        {
            m_impl->lastError =
                ERROR_INVALID_HANDLE;

            return false;
        }

        const std::size_t maxDword =
            static_cast<std::size_t>(
                std::numeric_limits<
                DWORD
                >::max()
                );

        const DWORD requested =
            static_cast<DWORD>(
                std::min(
                    capacity,
                    maxDword
                )
                );

        DWORD transferred =
            0;

        if (
            !::ReadFile(
                m_impl->handle,
                buffer,
                requested,
                &transferred,
                nullptr
            )
            )
        {
            m_impl->lastError =
                ::GetLastError();

            return false;
        }

        bytesRead =
            static_cast<std::size_t>(
                transferred
                );

        m_impl->lastError =
            ERROR_SUCCESS;

        return true;
    }

    bool HubSerialTransport::WriteAll(
        const std::uint8_t* data,
        const std::size_t size
    ) noexcept
    {
        if (!m_impl)
        {
            return false;
        }

        if (
            size != 0 &&
            data == nullptr
            )
        {
            m_impl->lastError =
                ERROR_INVALID_PARAMETER;

            return false;
        }

        if (size == 0)
        {
            m_impl->lastError =
                ERROR_SUCCESS;

            return true;
        }

        if (
            m_impl->handle ==
            INVALID_HANDLE_VALUE
            )
        {
            m_impl->lastError =
                ERROR_INVALID_HANDLE;

            return false;
        }

        const std::size_t maxDword =
            static_cast<std::size_t>(
                std::numeric_limits<
                DWORD
                >::max()
                );

        std::size_t offset =
            0;

        while (
            offset <
            size
            )
        {
            const std::size_t remaining =
                size -
                offset;

            const DWORD requested =
                static_cast<DWORD>(
                    std::min(
                        remaining,
                        maxDword
                    )
                    );

            DWORD transferred =
                0;

            if (
                !::WriteFile(
                    m_impl->handle,
                    data + offset,
                    requested,
                    &transferred,
                    nullptr
                )
                )
            {
                m_impl->lastError =
                    ::GetLastError();

                return false;
            }

            if (
                transferred ==
                0
                )
            {
                m_impl->lastError =
                    ERROR_WRITE_FAULT;

                return false;
            }

            offset +=
                static_cast<std::size_t>(
                    transferred
                    );
        }

        m_impl->lastError =
            ERROR_SUCCESS;

        return true;
    }

    std::uint32_t
        HubSerialTransport::
        LastError() const
        noexcept
    {
        if (!m_impl)
        {
            return
                ERROR_INVALID_HANDLE;
        }

        return
            m_impl->lastError;
    }

    std::string
        HubSerialTransport::
        PortName() const
    {
        if (!m_impl)
        {
            return {};
        }

        return
            m_impl->portName;
    }
}