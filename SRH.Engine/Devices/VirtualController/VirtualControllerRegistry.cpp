#include "pch.h"

#include "Devices/VirtualController/VirtualControllerRegistry.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace srh::engine
{
    void VirtualControllerRegistry::
        SetControllers(
            std::vector<VirtualControllerState>
            controllers
        )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_controllers.clear();

        for (
            auto& controller :
            controllers
            )
        {
            m_controllers[
                controller.deviceId
            ] =
                std::move(
                    controller
                );
        }
    }

    void VirtualControllerRegistry::
        AddOrUpdate(
            VirtualControllerState controller
        )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_controllers[
            controller.deviceId
        ] =
            std::move(
                controller
            );
    }

    bool VirtualControllerRegistry::
        Remove(
            const VirtualDeviceId deviceId
        )
    {
        std::unique_lock lock(
            m_mutex
        );

        return
            m_controllers.erase(
                deviceId
            ) != 0;
    }

    void VirtualControllerRegistry::
        Clear()
    {
        std::unique_lock lock(
            m_mutex
        );

        m_controllers.clear();
    }

    std::optional<VirtualControllerState>
        VirtualControllerRegistry::
        Find(
            const VirtualDeviceId deviceId
        ) const
    {
        std::shared_lock lock(
            m_mutex
        );

        const auto iterator =
            m_controllers.find(
                deviceId
            );

        if (
            iterator ==
            m_controllers.end()
            )
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    std::vector<VirtualControllerState>
        VirtualControllerRegistry::
        GetControllers() const
    {
        std::shared_lock lock(
            m_mutex
        );

        std::vector<VirtualControllerState>
            result;

        result.reserve(
            m_controllers.size()
        );

        for (
            const auto& [
                deviceId,
                controller
            ] :
            m_controllers
            )
        {
            (void)deviceId;

            result.push_back(
                controller
            );
        }

        std::sort(
            result.begin(),
            result.end(),
            [](
                const VirtualControllerState& left,
                const VirtualControllerState& right
                )
            {
                return
                    left.deviceId <
                    right.deviceId;
            }
        );

        return result;
    }

    VirtualControllerSnapshot
        VirtualControllerRegistry::
        GetSnapshot() const
    {
        VirtualControllerSnapshot
            snapshot;

        snapshot.controllers =
            GetControllers();

        return snapshot;
    }

    bool VirtualControllerRegistry::
        SetEnabled(
            const VirtualDeviceId deviceId,
            const bool enabled
        )
    {
        std::unique_lock lock(
            m_mutex
        );

        const auto iterator =
            m_controllers.find(
                deviceId
            );

        if (
            iterator ==
            m_controllers.end()
            )
        {
            return false;
        }

        iterator->second.enabled =
            enabled;

        return true;
    }

    bool VirtualControllerRegistry::
        SetActive(
            const VirtualDeviceId deviceId,
            const bool active
        )
    {
        std::unique_lock lock(
            m_mutex
        );

        const auto iterator =
            m_controllers.find(
                deviceId
            );

        if (
            iterator ==
            m_controllers.end()
            )
        {
            return false;
        }

        iterator->second.active =
            active;

        return true;
    }
}