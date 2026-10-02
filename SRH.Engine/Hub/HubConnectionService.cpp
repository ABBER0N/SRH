#include "pch.h"

#include "Hub/HubConnectionService.h"

#include "Hub/Protocol/HubMessageCodec.h"
#include "Hub/Protocol/HubProtocol.h"

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>

namespace
{
    constexpr std::size_t
        ReadBufferSize =
        4096;
}

namespace srh::engine::hub
{
    HubConnectionService::
        HubConnectionService(
            SrhEngine& engine
        )
        : m_engine(
            engine
        ),
        m_ingress(
            engine
        )
    {
    }

    HubConnectionService::
        ~HubConnectionService()
    {
        Disconnect();
    }

    bool HubConnectionService::Connect(
        const std::string_view portName,
        const std::uint32_t baudRate
    )
    {
        Disconnect();

        {
            std::scoped_lock ioLock(
                m_ioMutex
            );

            if (
                !m_transport.Open(
                    portName,
                    baudRate
                )
                )
            {
                const std::uint32_t error =
                    m_transport.LastError();

                std::scoped_lock stateLock(
                    m_stateMutex
                );

                m_transportConnected =
                    false;

                m_lastError =
                    error;

                m_portName.clear();

                return false;
            }
        }

        {
            std::scoped_lock ioLock(
                m_ioMutex
            );

            std::scoped_lock stateLock(
                m_stateMutex
            );

            m_transportConnected =
                true;

            m_lastError =
                ERROR_SUCCESS;

            m_portName =
                m_transport.PortName();
        }

        m_stopRequested.store(
            false,
            std::memory_order_release
        );

        try
        {
            m_worker =
                std::thread(
                    &HubConnectionService::
                    WorkerMain,
                    this
                );
        }
        catch (
            const std::system_error&
            )
        {
            {
                std::scoped_lock ioLock(
                    m_ioMutex
                );

                m_transport.Close();
            }

            {
                std::scoped_lock stateLock(
                    m_stateMutex
                );

                m_transportConnected =
                    false;

                m_lastError =
                    ERROR_NOT_ENOUGH_MEMORY;

                m_portName.clear();
            }

            m_ingress.Reset();

            return false;
        }
        catch (...)
        {
            {
                std::scoped_lock ioLock(
                    m_ioMutex
                );

                m_transport.Close();
            }

            {
                std::scoped_lock stateLock(
                    m_stateMutex
                );

                m_transportConnected =
                    false;

                m_lastError =
                    ERROR_GEN_FAILURE;

                m_portName.clear();
            }

            m_ingress.Reset();

            return false;
        }

        return true;
    }

    void HubConnectionService::Disconnect()
        noexcept
    {
        m_stopRequested.store(
            true,
            std::memory_order_release
        );

        if (
            m_worker.joinable()
            )
        {
            m_worker.join();
        }

        {
            std::scoped_lock ioLock(
                m_ioMutex
            );

            m_transport.Close();
        }

        {
            std::scoped_lock stateLock(
                m_stateMutex
            );

            m_transportConnected =
                false;

            m_lastError =
                ERROR_SUCCESS;

            m_portName.clear();
        }

        m_ingress.Reset();
    }

    bool HubConnectionService::
        IsTransportConnected() const
        noexcept
    {
        std::scoped_lock stateLock(
            m_stateMutex
        );

        return
            m_transportConnected;
    }

    bool HubConnectionService::
        IsSessionEstablished() const
        noexcept
    {
        return
            m_ingress
            .IsSessionEstablished();
    }

    std::uint32_t
        HubConnectionService::
        LastError() const
        noexcept
    {
        std::scoped_lock stateLock(
            m_stateMutex
        );

        return
            m_lastError;
    }

    std::string
        HubConnectionService::
        PortName() const
    {
        std::scoped_lock stateLock(
            m_stateMutex
        );

        return
            m_portName;
    }

    bool HubConnectionService::
        SendHubMessage(
            const protocol::HubMessage& message,
            const HubPacketFlags flags
        )
    {
        const std::uint32_t sequence =
            AllocateSequence();

        const auto packet =
            protocol::
            HubMessageCodec::
            Encode(
                message,
                sequence,
                flags
            );

        if (!packet.has_value())
        {
            std::scoped_lock stateLock(
                m_stateMutex
            );

            m_lastError =
                ERROR_INVALID_DATA;

            return false;
        }

        const auto bytes =
            protocol::
            HubProtocol::
            Encode(
                *packet
            );

        if (bytes.empty())
        {
            std::scoped_lock stateLock(
                m_stateMutex
            );

            m_lastError =
                ERROR_INVALID_DATA;

            return false;
        }

        std::uint32_t error =
            ERROR_SUCCESS;

        {
            std::scoped_lock ioLock(
                m_ioMutex
            );

            if (
                !m_transport.IsOpen()
                )
            {
                error =
                    ERROR_INVALID_HANDLE;
            }
            else if (
                !m_transport.WriteAll(
                    bytes.data(),
                    bytes.size()
                )
                )
            {
                error =
                    m_transport.LastError();

                m_transport.Close();
            }
        }

        if (
            error !=
            ERROR_SUCCESS
            )
        {
            m_stopRequested.store(
                true,
                std::memory_order_release
            );

            {
                std::scoped_lock stateLock(
                    m_stateMutex
                );

                m_transportConnected =
                    false;

                m_lastError =
                    error;

                m_portName.clear();
            }

            m_ingress.Reset();

            return false;
        }

        {
            std::scoped_lock stateLock(
                m_stateMutex
            );

            m_lastError =
                ERROR_SUCCESS;
        }

        return true;
    }

    std::optional<std::uint64_t>
        HubConnectionService::
        LastHeartbeatUptimeMs() const
        noexcept
    {
        return
            m_ingress
            .LastHeartbeatUptimeMs();
    }

    void HubConnectionService::
        WorkerMain()
        noexcept
    {
        std::array<
            std::uint8_t,
            ReadBufferSize
        > buffer{};

        for (;;)
        {
            if (
                m_stopRequested.load(
                    std::memory_order_acquire
                )
                )
            {
                break;
            }

            std::size_t bytesRead =
                0;

            bool readSucceeded =
                false;

            std::uint32_t readError =
                ERROR_SUCCESS;

            {
                std::scoped_lock ioLock(
                    m_ioMutex
                );

                if (
                    !m_transport.IsOpen()
                    )
                {
                    readError =
                        ERROR_INVALID_HANDLE;
                }
                else
                {
                    readSucceeded =
                        m_transport.ReadSome(
                            buffer.data(),
                            buffer.size(),
                            bytesRead
                        );

                    if (!readSucceeded)
                    {
                        readError =
                            m_transport
                            .LastError();
                    }
                }
            }

            if (!readSucceeded)
            {
                if (
                    m_stopRequested.load(
                        std::memory_order_acquire
                    )
                    )
                {
                    break;
                }

                {
                    std::scoped_lock ioLock(
                        m_ioMutex
                    );

                    m_transport.Close();
                }

                {
                    std::scoped_lock stateLock(
                        m_stateMutex
                    );

                    m_transportConnected =
                        false;

                    m_lastError =
                        readError;

                    m_portName.clear();
                }

                m_ingress.Reset();

                return;
            }

            if (
                bytesRead ==
                0
                )
            {
                continue;
            }

            (void)m_ingress.Push(
                buffer.data(),
                bytesRead
            );
        }
    }

    std::uint32_t
        HubConnectionService::
        AllocateSequence()
        noexcept
    {
        return
            m_nextSequence.fetch_add(
                1,
                std::memory_order_relaxed
            );
    }
}