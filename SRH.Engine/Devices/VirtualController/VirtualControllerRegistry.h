#pragma once

#include "Domain/VirtualControllerState.h"

#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace srh::engine
{
    class VirtualControllerRegistry
    {
    public:
        void SetControllers(
            std::vector<VirtualControllerState>
            controllers
        );

        void AddOrUpdate(
            VirtualControllerState controller
        );

        [[nodiscard]]
        bool Remove(
            VirtualDeviceId deviceId
        );

        void Clear();

        [[nodiscard]]
        std::optional<VirtualControllerState>
            Find(
                VirtualDeviceId deviceId
            ) const;

        [[nodiscard]]
        std::vector<VirtualControllerState>
            GetControllers() const;

        [[nodiscard]]
        VirtualControllerSnapshot
            GetSnapshot() const;

        [[nodiscard]]
        bool SetEnabled(
            VirtualDeviceId deviceId,
            bool enabled
        );

        [[nodiscard]]
        bool SetActive(
            VirtualDeviceId deviceId,
            bool active
        );

        void SetAllActive(
            bool active
        );

    private:
        mutable std::shared_mutex
            m_mutex;

        std::unordered_map<
            VirtualDeviceId,
            VirtualControllerState
        > m_controllers;
    };
}