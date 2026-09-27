#include "pch.h"

#include "Execution/VirtualController/VirtualControllerService.h"

#include <algorithm>
#include <limits>

namespace srh::engine::virtual_controller
{
    ActionExecutionStatus
        VirtualControllerService::Execute(
            const VirtualControllerAction& action
        ) noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        auto& report =
            m_reports[
                action.deviceId
            ];

        if (
            !ApplyAction(
                report,
                action
            )
            )
        {
            return
                ActionExecutionStatus::
                Failed;
        }

        if (!EnsureConnected())
        {
            return
                ActionExecutionStatus::
                Failed;
        }

        const bool submitted =
            m_client.SubmitReport(
                action.deviceId,
                report
            );

        return
            submitted
            ? ActionExecutionStatus::
            Executed
            : ActionExecutionStatus::
            Failed;
    }

    bool VirtualControllerService::
        ConnectFirstAvailable()
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        return
            EnsureConnected();
    }

    void VirtualControllerService::
        Disconnect()
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        m_client.Close();
    }

    bool VirtualControllerService::
        IsConnected() const
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        return
            m_client.IsOpen();
    }

    std::uint32_t
        VirtualControllerService::
        LastError() const
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        return
            m_client.LastError();
    }

    void VirtualControllerService::
        ClearState()
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        m_reports.clear();
    }

    bool VirtualControllerService::
        EnsureConnected()
        noexcept
    {
        if (
            m_client.IsOpen()
            )
        {
            return true;
        }

        return
            m_client.OpenFirstAvailable();
    }

    bool VirtualControllerService::
        ApplyAction(
            InputReportV1& report,
            const VirtualControllerAction& action
        ) noexcept
    {
        switch (action.kind)
        {
        case VirtualControllerActionKind::Button:
        {
            //
            // Public SRH button numbering:
            //
            // 1 ... 128
            //

            if (
                action.controlId < 1 ||
                action.controlId >
                ButtonCount
                )
            {
                return false;
            }

            const std::size_t
                buttonIndex =
                static_cast<std::size_t>(
                    action.controlId - 1
                    );

            const std::size_t
                byteIndex =
                buttonIndex / 8;

            const std::size_t
                bitIndex =
                buttonIndex % 8;

            const std::uint8_t mask =
                static_cast<std::uint8_t>(
                    1u << bitIndex
                    );

            if (
                action.value != 0
                )
            {
                report.buttons[
                    byteIndex
                ] |= mask;
            }
            else
            {
                report.buttons[
                    byteIndex
                ] &=
                    static_cast<std::uint8_t>(
                        ~mask
                        );
            }

            return true;
        }

        case VirtualControllerActionKind::Axis:
        {
            //
            // Public SRH axis numbering:
            //
            // 1 ... 8
            //

            if (
                action.controlId < 1 ||
                action.controlId >
                AxisCount
                )
            {
                return false;
            }

            const std::size_t index =
                static_cast<std::size_t>(
                    action.controlId - 1
                    );

            report.axes[
                index
            ] =
                EncodeAxis(
                    action.value
                );

                return true;
        }

        case VirtualControllerActionKind::Pov:
        {
            //
            // Public SRH POV numbering:
            //
            // 1 ... 4
            //
            // Canonical value:
            //
            // -1     = centered
            // 0      = north
            // 4500   = north-east
            // 9000   = east
            // ...
            // 31500  = north-west
            //

            if (
                action.controlId < 1 ||
                action.controlId >
                PovCount
                )
            {
                return false;
            }

            const std::size_t index =
                static_cast<std::size_t>(
                    action.controlId - 1
                    );

            report.pov[
                index
            ] =
                EncodePov(
                    action.value
                );

                return true;
        }

        default:
            return false;
        }
    }

    std::int16_t
        VirtualControllerService::
        EncodeAxis(
            const std::int32_t value
        ) noexcept
    {
        const auto minimum =
            static_cast<std::int32_t>(
                std::numeric_limits<
                std::int16_t
                >::min()
                );

        const auto maximum =
            static_cast<std::int32_t>(
                std::numeric_limits<
                std::int16_t
                >::max()
                );

        return
            static_cast<std::int16_t>(
                std::clamp(
                    value,
                    minimum,
                    maximum
                )
                );
    }

    std::uint8_t
        VirtualControllerService::
        EncodePov(
            const std::int32_t value
        ) noexcept
    {
        if (value < 0)
        {
            return 8;
        }

        constexpr std::int32_t
            FullCircle = 36000;

        constexpr std::int32_t
            SectorSize = 4500;

        constexpr std::int32_t
            HalfSector =
            SectorSize / 2;

        auto normalized =
            value % FullCircle;

        if (normalized < 0)
        {
            normalized +=
                FullCircle;
        }

        const auto sector =
            (
                normalized +
                HalfSector
                ) /
            SectorSize;

        return
            static_cast<std::uint8_t>(
                sector % 8
                );
    }
}