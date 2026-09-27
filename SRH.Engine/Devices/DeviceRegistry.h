#pragma once

#include "Domain/DeviceState.h"

#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace srh::engine
{
    class DeviceRegistry
    {
    public:
        DeviceRegistry() = default;

        DeviceRegistry(
            const DeviceRegistry&
        ) = delete;

        DeviceRegistry& operator=(
            const DeviceRegistry&
            ) = delete;

        void SetHubState(
            HubState state
        );

        [[nodiscard]]
        HubState GetHubState() const;

        void UpsertNode(
            NodeState state
        );

        void RemoveNode(
            NodeId nodeId
        );

        void ClearNodes();

        [[nodiscard]]
        std::optional<NodeState> FindNode(
            NodeId nodeId
        ) const;

        [[nodiscard]]
        DeviceSnapshot GetSnapshot() const;

    private:
        mutable std::shared_mutex m_mutex;

        HubState m_hubState;

        std::unordered_map<
            NodeId,
            NodeState
        > m_nodes;
    };
}