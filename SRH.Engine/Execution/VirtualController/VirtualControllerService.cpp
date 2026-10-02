#include "pch.h"

#include "Execution/VirtualController/VirtualControllerService.h"

#include <algorithm>
#include <limits>

namespace srh::engine::virtual_controller
{
    VirtualControllerService::
        ~VirtualControllerService()
    {
        Disconnect();
    }

    ActionExecutionStatus
        VirtualControllerService::Execute(
            const VirtualControllerAction& action
        )
    {
        std::scoped_lock lock(
            m_mutex
        );

        InputReportV1 report;

        const auto existing =
            m_reports.find(
                action.deviceId
            );

        if (
            existing !=
            m_reports.end()
            )
        {
            report =
                existing->second;
        }

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

        //
        // Keep the desired complete state even if
        // the driver is temporarily unavailable.
        //

        m_reports[
            action.deviceId
        ] =
            report;

            if (!EnsureConnected())
            {
                return
                    ActionExecutionStatus::
                    Failed;
            }

            if (
                !m_client.SubmitReport(
                    action.deviceId,
                    report
                )
                )
            {
                //
                // Do not retain a potentially dead
                // device handle. The next action will
                // attempt discovery again.
                //

                m_client.Close();

                return
                    ActionExecutionStatus::
                    Failed;
            }

            return
                ActionExecutionStatus::
                Executed;
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

        if (m_client.IsOpen())
        {
            (void)NeutralizeAllLocked();
        }

        m_reports.clear();

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

    bool VirtualControllerService::
        ResetDevice(
            const VirtualDeviceId deviceId
        ) noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        const auto iterator =
            m_reports.find(
                deviceId
            );

        if (
            iterator ==
            m_reports.end()
            )
        {
            return true;
        }

        bool success =
            true;

        if (!EnsureConnected())
        {
            success =
                false;
        }
        else
        {
            InputReportV1
                neutralReport;

            if (
                !m_client.SubmitReport(
                    deviceId,
                    neutralReport
                )
                )
            {
                success =
                    false;

                m_client.Close();
            }
        }

        //
        // Never restore the old active state after
        // disabling/removing this controller.
        //

        m_reports.erase(
            iterator
        );

        return success;
    }

    bool VirtualControllerService::
        ResetAll()
        noexcept
    {
        std::scoped_lock lock(
            m_mutex
        );

        if (m_reports.empty())
        {
            return true;
        }

        if (!EnsureConnected())
        {
            m_reports.clear();

            return false;
        }

        const bool success =
            NeutralizeAllLocked();

        m_reports.clear();

        if (!success)
        {
            m_client.Close();
        }

        return success;
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
        NeutralizeAllLocked()
        noexcept
    {
        if (m_reports.empty())
        {
            return true;
        }

        if (!m_client.IsOpen())
        {
            return false;
        }

        bool success =
            true;

        for (
            const auto& [
                deviceId,
                currentReport
            ] :
            m_reports
            )
        {
            (void)currentReport;

            InputReportV1
                neutralReport;

            if (
                !m_client.SubmitReport(
                    deviceId,
                    neutralReport
                )
                )
            {
                success =
                    false;
            }
        }

        return success;
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