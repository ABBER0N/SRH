#include "pch.h"

#include "Public/SrhEngine.h"

#include "Devices/DeviceRegistry.h"
#include "Input/InputEventBus.h"
#include "Input/InputStateStore.h"
#include "Mapping/MappingService.h"

#include <utility>

namespace srh::engine
{
    class SrhEngine::Impl
    {
    public:
        DeviceRegistry
            deviceRegistry;

        InputStateStore
            inputStateStore;

        InputEventBus
            inputEventBus;

        MappingService
            mappingService;
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

    //
    // Devices
    //

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

        m_impl->inputStateStore.ClearNode(
            nodeId
        );
    }

    void SrhEngine::ClearNodes()
    {
        m_impl->deviceRegistry.ClearNodes();

        m_impl->inputStateStore.Clear();
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

    //
    // Input
    //

    bool SrhEngine::SubmitInputEvent(
        const InputEvent& event
    )
    {
        const bool applied =
            m_impl->inputStateStore.Apply(
                event
            );

        if (!applied)
        {
            return false;
        }

        m_impl->inputEventBus.Publish(
            event
        );

        return true;
    }

    std::optional<InputControlState>
        SrhEngine::FindInputState(
            const NodeId nodeId,
            const ControlId controlId
        ) const
    {
        return
            m_impl->inputStateStore
            .Find(
                nodeId,
                controlId
            );
    }

    InputSnapshot
        SrhEngine::GetInputSnapshot() const
    {
        return
            m_impl->inputStateStore
            .GetSnapshot();
    }

    void SrhEngine::ClearInputState()
    {
        m_impl->inputStateStore.Clear();
    }

    //
    // Input events
    //

    SrhEngine::InputSubscriptionId
        SrhEngine::SubscribeInput(
            InputCallback callback
        )
    {
        return
            m_impl->inputEventBus.Subscribe(
                std::move(callback)
            );
    }

    void SrhEngine::UnsubscribeInput(
        const InputSubscriptionId subscriptionId
    )
    {
        m_impl->inputEventBus.Unsubscribe(
            subscriptionId
        );
    }

    //
    // Mapping
    //

    void SrhEngine::SetMappingRules(
        std::vector<MappingRule> rules
    )
    {
        m_impl->mappingService.SetRules(
            std::move(rules)
        );
    }

    void SrhEngine::AddOrUpdateMappingRule(
        MappingRule rule
    )
    {
        m_impl->mappingService.AddOrUpdateRule(
            std::move(rule)
        );
    }

    void SrhEngine::RemoveMappingRule(
        const MappingRuleId ruleId
    )
    {
        m_impl->mappingService.RemoveRule(
            ruleId
        );
    }

    void SrhEngine::ClearMappingRules()
    {
        m_impl->mappingService.Clear();
    }

    std::optional<MappingRule>
        SrhEngine::FindMappingRule(
            const MappingRuleId ruleId
        ) const
    {
        return
            m_impl->mappingService.FindRule(
                ruleId
            );
    }

    std::vector<MappingRule>
        SrhEngine::GetMappingRules() const
    {
        return
            m_impl->mappingService.GetRules();
    }

    std::vector<Action>
        SrhEngine::ResolveActions(
            const InputEvent& event
        ) const
    {
        return
            m_impl->mappingService.Resolve(
                event
            );
    }
}