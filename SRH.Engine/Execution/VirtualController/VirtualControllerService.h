#pragma once

#include "Domain/Action.h"
#include "Domain/ExecutionTypes.h"
#include "Execution/VirtualController/VirtualDeviceClient.h"
#include "Execution/VirtualController/VirtualDeviceProtocol.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace srh::engine::virtual_controller
{
    class VirtualControllerService
    {
    public:
        VirtualControllerService() = default;

        ~VirtualControllerService();

        VirtualControllerService(
            const VirtualControllerService&
        ) = delete;

        VirtualControllerService& operator=(
            const VirtualControllerService&
            ) = delete;

        [[nodiscard]]
        ActionExecutionStatus Execute(
            const VirtualControllerAction& action
        );

        [[nodiscard]]
        bool ConnectFirstAvailable()
            noexcept;

        void Disconnect()
            noexcept;

        [[nodiscard]]
        bool IsConnected() const
            noexcept;

        [[nodiscard]]
        std::uint32_t LastError() const
            noexcept;

        [[nodiscard]]
        bool ResetDevice(
            VirtualDeviceId deviceId
        ) noexcept;

        [[nodiscard]]
        bool ResetAll()
            noexcept;

    private:
        [[nodiscard]]
        bool EnsureConnected()
            noexcept;

        [[nodiscard]]
        bool NeutralizeAllLocked()
            noexcept;

        [[nodiscard]]
        static bool ApplyAction(
            InputReportV1& report,
            const VirtualControllerAction& action
        ) noexcept;

        [[nodiscard]]
        static std::int16_t EncodeAxis(
            std::int32_t value
        ) noexcept;

        [[nodiscard]]
        static std::uint8_t EncodePov(
            std::int32_t value
        ) noexcept;

        mutable std::mutex
            m_mutex;

        VirtualDeviceClient
            m_client;

        std::unordered_map<
            VirtualDeviceId,
            InputReportV1
        > m_reports;
    };
}