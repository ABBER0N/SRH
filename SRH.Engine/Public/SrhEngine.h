#pragma once

#include "Domain/Action.h"
#include "Domain/DeviceState.h"
#include "Domain/ExecutionTypes.h"
#include "Domain/InputEvent.h"
#include "Domain/InputTypes.h"
#include "Domain/MediaState.h"
#include "Domain/VirtualControllerState.h"
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
        InputProcessingResult SubmitInputEvent(
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

        //
        // Virtual controller configuration
        //

        void SetVirtualControllers(
            std::vector<VirtualControllerState>
            controllers
        );

        void AddOrUpdateVirtualController(
            VirtualControllerState controller
        );

        [[nodiscard]]
        bool RemoveVirtualController(
            VirtualDeviceId deviceId
        );

        void ClearVirtualControllers();

        [[nodiscard]]
        std::optional<VirtualControllerState>
            FindVirtualController(
                VirtualDeviceId deviceId
            ) const;

        [[nodiscard]]
        std::vector<VirtualControllerState>
            GetVirtualControllers() const;

        [[nodiscard]]
        VirtualControllerSnapshot
            GetVirtualControllerSnapshot() const;

        [[nodiscard]]
        bool SetVirtualControllerEnabled(
            VirtualDeviceId deviceId,
            bool enabled
        );

        //
        // Virtual controller driver
        //

        [[nodiscard]]
        bool ConnectVirtualControllerDriver()
            noexcept;

        void DisconnectVirtualControllerDriver()
            noexcept;

        [[nodiscard]]
        bool IsVirtualControllerDriverConnected()
            const noexcept;

        [[nodiscard]]
        std::uint32_t
            GetVirtualControllerDriverError()
            const noexcept;

        [[nodiscard]]
        bool ResetVirtualControllers()
            noexcept;

        //
        // Media state
        //

        [[nodiscard]]
        std::optional<MediaSessionInfo>
            GetCurrentMediaSession() const;

    private:
        class Impl;

        std::unique_ptr<Impl>
            m_impl;
    };
}