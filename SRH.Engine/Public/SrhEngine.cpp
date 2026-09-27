#include "pch.h"

#include "Public/SrhEngine.h"

#include "Devices/DeviceRegistry.h"

#include <utility>

namespace srh::engine
{
    class SrhEngine::Impl
    {
    public:
        DeviceRegistry deviceRegistry;
    };

    SrhEngine::SrhEngine()
        : m_impl(
            std::make_unique<Impl>()
        )
    {
    }

    SrhEngine::~SrhEngine() =
        default;

    SrhEngine::SrhEngine(
        SrhEngine&&
    ) noexcept = default;

    SrhEngine& SrhEngine::operator=(
        SrhEngine&&
        ) noexcept = default;

    void SrhEngine::SetHubState(
        HubState state
    )
    {
        m_impl->deviceRegistry.SetHubState(
            std::move(state)
        );
    }

    void SrhEngine::UpsertNodeState(
        NodeState state
    )
    {
        m_impl->deviceRegistry.UpsertNode(
            std::move(state)
        );
    }

    void SrhEngine::RemoveNode(
        const NodeId nodeId
    )
    {
        m_impl->deviceRegistry.RemoveNode(
            nodeId
        );
    }

    void SrhEngine::ClearNodes()
    {
        m_impl->deviceRegistry.ClearNodes();
    }

    HubState SrhEngine::GetHubState() const
    {
        return
            m_impl->deviceRegistry
            .GetHubState();
    }

    std::optional<NodeState>
        SrhEngine::FindNode(
            const NodeId nodeId
        ) const
    {
        return
            m_impl->deviceRegistry
            .FindNode(
                nodeId
            );
    }

    DeviceSnapshot
        SrhEngine::GetDeviceSnapshot() const
    {
        return
            m_impl->deviceRegistry
            .GetSnapshot();
    }
}