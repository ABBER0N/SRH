#pragma once

#include "Domain/DeviceState.h"

#include <memory>
#include <optional>

namespace srh::engine
{
    class SrhEngine
    {
    public:
        SrhEngine();
        ~SrhEngine();

        SrhEngine(
            const SrhEngine&
        ) = delete;

        SrhEngine& operator=(
            const SrhEngine&
            ) = delete;

        SrhEngine(
            SrhEngine&&
        ) noexcept;

        SrhEngine& operator=(
            SrhEngine&&
            ) noexcept;

        void SetHubState(
            HubState state
        );

        void UpsertNodeState(
            NodeState state
        );

        void RemoveNode(
            NodeId nodeId
        );

        void ClearNodes();

        [[nodiscard]]
        HubState GetHubState() const;

        [[nodiscard]]
        std::optional<NodeState> FindNode(
            NodeId nodeId
        ) const;

        [[nodiscard]]
        DeviceSnapshot GetDeviceSnapshot() const;

    private:
        class Impl;

        std::unique_ptr<Impl> m_impl;
    };
}