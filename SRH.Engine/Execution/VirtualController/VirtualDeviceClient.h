#pragma once

#include "Execution/VirtualController/VirtualDeviceProtocol.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace srh::engine::virtual_controller
{
    class VirtualDeviceClient
    {
    public:
        VirtualDeviceClient();

        ~VirtualDeviceClient();

        VirtualDeviceClient(
            const VirtualDeviceClient&
        ) = delete;

        VirtualDeviceClient& operator=(
            const VirtualDeviceClient&
            ) = delete;

        [[nodiscard]]
        bool Open(
            std::wstring_view devicePath
        ) noexcept;

        [[nodiscard]]
        bool OpenFirstAvailable()
            noexcept;

        void Close()
            noexcept;

        [[nodiscard]]
        bool IsOpen() const
            noexcept;

        [[nodiscard]]
        std::uint32_t LastError() const
            noexcept;

        [[nodiscard]]
        bool SubmitReport(
            std::uint16_t deviceId,
            const InputReportV1& report
        ) noexcept;

    private:
        struct Impl;

        std::unique_ptr<Impl>
            m_impl;
    };
}