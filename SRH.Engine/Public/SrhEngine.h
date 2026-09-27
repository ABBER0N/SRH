#pragma once

#include "Domain/Action.h"
#include "Domain/DeviceState.h"
#include "Domain/InputEvent.h"
#include "Domain/InputTypes.h"
#include "Mapping/MappingRule.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace srh::engine
{
    class SrhEngine
    {
    public:
        using InputSubscriptionId =
            std::uint64_t;

        using InputCallback =
            std::function<
            void(const InputEvent&)
            >;

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

        //
        // Devices
        //

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

        //
        // Input
        //

        [[nodiscard]]
        bool SubmitInputEvent(
            const InputEvent& event
        );

        [[nodiscard]]
        std::optional<InputControlState>
            FindInputState(
                NodeId nodeId,
                ControlId controlId
            ) const;

        [[nodiscard]]
        InputSnapshot GetInputSnapshot() const;

        void ClearInputState();

        //
        // Input events
        //

        [[nodiscard]]
        InputSubscriptionId SubscribeInput(
            InputCallback callback
        );

        void UnsubscribeInput(
            InputSubscriptionId subscriptionId
        );

        //
        // Mapping
        //

        void SetMappingRules(
            std::vector<MappingRule> rules
        );

        void AddOrUpdateMappingRule(
            MappingRule rule
        );

        void RemoveMappingRule(
            MappingRuleId ruleId
        );

        void ClearMappingRules();

        [[nodiscard]]
        std::optional<MappingRule>
            FindMappingRule(
                MappingRuleId ruleId
            ) const;

        [[nodiscard]]
        std::vector<MappingRule>
            GetMappingRules() const;

        [[nodiscard]]
        std::vector<Action>
            ResolveActions(
                const InputEvent& event
            ) const;

    private:
        class Impl;

        std::unique_ptr<Impl>
            m_impl;
    };
}