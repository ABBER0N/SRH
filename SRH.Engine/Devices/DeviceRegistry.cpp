#include "pch.h"

#include "Devices/DeviceRegistry.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace srh::engine
{
    void DeviceRegistry::SetHubState(
        HubState state
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_hubState =
            std::move(state);
    }

    HubState DeviceRegistry::GetHubState() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return m_hubState;
    }

    void DeviceRegistry::UpsertNode(
        NodeState state
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_nodes[state.nodeId] =
            std::move(state);
    }

    void DeviceRegistry::RemoveNode(
        const NodeId nodeId
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_nodes.erase(
            nodeId
        );
    }

    void DeviceRegistry::ClearNodes()
    {
        std::unique_lock lock(
            m_mutex
        );

        m_nodes.clear();
    }

    std::optional<NodeState>
        DeviceRegistry::FindNode(
            const NodeId nodeId
        ) const
    {
        std::shared_lock lock(
            m_mutex
        );

        const auto iterator =
            m_nodes.find(
                nodeId
            );

        if (
            iterator ==
            m_nodes.end()
            )
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    DeviceSnapshot
        DeviceRegistry::GetSnapshot() const
    {
        std::shared_lock lock(
            m_mutex
        );

        DeviceSnapshot snapshot;

        snapshot.hub =
            m_hubState;

        snapshot.nodes.reserve(
            m_nodes.size()
        );

        for (
            const auto& [nodeId, node] :
            m_nodes
            )
        {
            (void)nodeId;

            snapshot.nodes.push_back(
                node
            );
        }

        std::sort(
            snapshot.nodes.begin(),
            snapshot.nodes.end(),
            [](
                const NodeState& left,
                const NodeState& right
                )
            {
                return
                    left.nodeId <
                    right.nodeId;
            }
        );

        return snapshot;
    }
}