#include "Public/SrhEngine.h"

#include "Execution/Windows/Audio/AudioEndpointService.h"
#include "Execution/Windows/Audio/AudioService.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    constexpr srh::engine::NodeId
        TestNodeId = 1;

    constexpr srh::engine::ControlId
        KeyboardControlId = 10;

    constexpr srh::engine::MappingRuleId
        KeyboardRuleId = 1;

    constexpr std::uint32_t
        TestVirtualKey = 0x87;

    void AddSystemActionRule(
        srh::engine::SrhEngine& engine,
        const srh::engine::MappingRuleId ruleId,
        const srh::engine::ControlId controlId,
        srh::engine::SystemAction action
    )
    {
        srh::engine::MappingRule rule;

        rule.id =
            ruleId;

        rule.enabled =
            true;

        rule.input.nodeId =
            TestNodeId;

        rule.input.controlId =
            controlId;

        rule.input.eventType =
            srh::engine::InputEventType::
            ButtonDown;

        rule.input.valueCondition =
            srh::engine::InputValueCondition::
            Any;

        rule.action =
            std::move(action);

        rule.valueMode =
            srh::engine::ActionValueMode::
            Fixed;

        engine.AddOrUpdateMappingRule(
            std::move(rule)
        );
    }

    srh::engine::InputProcessingResult
        SubmitButton(
            srh::engine::SrhEngine& engine,
            const srh::engine::ControlId controlId,
            const std::uint64_t timestamp
        )
    {
        srh::engine::InputEvent event;

        event.nodeId =
            TestNodeId;

        event.controlId =
            controlId;

        event.type =
            srh::engine::InputEventType::
            ButtonDown;

        event.value =
            1;

        event.timestamp =
            timestamp;

        return
            engine.SubmitInputEvent(
                event
            );
    }

    std::optional<srh::engine::AudioEndpointInfo>
        FindDefaultEndpoint(
            const std::vector<
            srh::engine::AudioEndpointInfo
            >& endpoints
        )
    {
        const auto iterator =
            std::find_if(
                endpoints.begin(),
                endpoints.end(),
                [](
                    const srh::engine::
                    AudioEndpointInfo& endpoint
                    )
                {
                    return
                        endpoint.isDefault;
                }
            );

        if (
            iterator ==
            endpoints.end()
            )
        {
            return std::nullopt;
        }

        return *iterator;
    }

    bool TestDeviceRegistry(
        srh::engine::SrhEngine& engine
    )
    {
        srh::engine::HubState hub;

        hub.connected =
            true;

        hub.name =
            "SRH Test Hub";

        hub.firmwareVersion =
            "0.1.0";

        hub.protocolVersion =
            "1";

        hub.health =
            srh::engine::DeviceHealth::
            Healthy;

        engine.SetHubState(
            hub
        );

        srh::engine::NodeState node;

        node.nodeId =
            TestNodeId;

        node.physicalUid =
            0x12345678;

        node.type =
            srh::engine::NodeType::
            ButtonBox;

        node.name =
            "Smoke Test Node";

        node.connected =
            true;

        node.configured =
            true;

        node.enabled =
            true;

        node.active =
            true;

        node.health =
            srh::engine::DeviceHealth::
            Healthy;

        node.firmwareVersion =
            "0.1.0";

        engine.UpsertNodeState(
            node
        );

        const auto snapshot =
            engine.GetDeviceSnapshot();

        if (!snapshot.hub.connected)
        {
            return false;
        }

        if (
            snapshot.nodes.size() !=
            1
            )
        {
            return false;
        }

        return
            snapshot.nodes[0].nodeId ==
            TestNodeId;
    }

    bool TestMapping(
        srh::engine::SrhEngine& engine
    )
    {
        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            KeyboardPress;

        action.code =
            TestVirtualKey;

        AddSystemActionRule(
            engine,
            KeyboardRuleId,
            KeyboardControlId,
            action
        );

        srh::engine::InputEvent event;

        event.nodeId =
            TestNodeId;

        event.controlId =
            KeyboardControlId;

        event.type =
            srh::engine::InputEventType::
            ButtonDown;

        event.value =
            1;

        event.timestamp =
            1000;

        const auto actions =
            engine.ResolveActions(
                event
            );

        if (
            actions.size() !=
            1
            )
        {
            return false;
        }

        const auto* systemAction =
            std::get_if<
            srh::engine::SystemAction
            >(
                &actions[0]
            );

        if (
            systemAction ==
            nullptr
            )
        {
            return false;
        }

        return
            systemAction->kind ==
            srh::engine::
            SystemActionKind::
            KeyboardPress &&
            systemAction->code ==
            TestVirtualKey;
    }

    bool TestInputPipeline(
        srh::engine::SrhEngine& engine
    )
    {
        std::uint32_t receivedEvents =
            0;

        const auto subscriptionId =
            engine.SubscribeInput(
                [&receivedEvents](
                    const srh::engine::
                    InputEvent& event
                    )
                {
                    if (
                        event.nodeId ==
                        TestNodeId &&
                        event.controlId ==
                        KeyboardControlId &&
                        event.type ==
                        srh::engine::
                        InputEventType::
                        ButtonDown
                        )
                    {
                        ++receivedEvents;
                    }
                }
            );

        if (
            subscriptionId ==
            0
            )
        {
            return false;
        }

        const auto result =
            SubmitButton(
                engine,
                KeyboardControlId,
                2000
            );

        engine.UnsubscribeInput(
            subscriptionId
        );

        if (!result.accepted)
        {
            return false;
        }

        if (
            receivedEvents !=
            1
            )
        {
            return false;
        }

        const auto state =
            engine.FindInputState(
                TestNodeId,
                KeyboardControlId
            );

        if (!state.has_value())
        {
            return false;
        }

        return
            state->value == 1 &&
            state->type ==
            srh::engine::
            InputEventType::
            ButtonDown;
    }

    bool TestWindowsExecution(
        srh::engine::SrhEngine& engine
    )
    {
        const auto result =
            SubmitButton(
                engine,
                KeyboardControlId,
                3000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Executed &&
            result.AllExecuted();
    }

    bool TestMasterAudioExecution(
        srh::engine::SrhEngine& engine
    )
    {
        constexpr srh::engine::ControlId
            ControlId = 20;

        constexpr srh::engine::MappingRuleId
            RuleId = 2;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            MasterVolumeAdjust;

        //
        // Safe no-op:
        // current volume + 0.0
        //

        action.value =
            0.0f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        const auto result =
            SubmitButton(
                engine,
                ControlId,
                4000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Executed &&
            result.AllExecuted();
    }

    bool TestAudioSessionEnumeration()
    {
        srh::engine::AudioService
            audioService;

        const auto sessions =
            audioService.EnumerateSessions();

        for (
            const auto& session :
            sessions
            )
        {
            if (
                session.sessionInstanceId
                .empty()
                )
            {
                return false;
            }

            if (
                session.volume < 0.0f ||
                session.volume > 1.0f
                )
            {
                return false;
            }
        }

        return true;
    }

    bool TestApplicationAudioRouting(
        srh::engine::SrhEngine& engine
    )
    {
        constexpr srh::engine::ControlId
            ControlId = 21;

        constexpr srh::engine::MappingRuleId
            RuleId = 3;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            ApplicationVolumeSet;

        action.targetId =
            "SRH_DOES_NOT_EXIST";

        action.value =
            0.5f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        const auto result =
            SubmitButton(
                engine,
                ControlId,
                5000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        //
        // Function is implemented,
        // but target does not exist.
        //

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Failed;
    }

    bool TestEndpointEnumeration(
        std::optional<
        srh::engine::AudioEndpointInfo
        >& defaultOutput,
        std::optional<
        srh::engine::AudioEndpointInfo
        >& defaultInput
    )
    {
        srh::engine::AudioEndpointService
            service;

        const auto outputs =
            service.EnumerateOutputDevices();

        const auto inputs =
            service.EnumerateInputDevices();

        if (outputs.empty())
        {
            return false;
        }

        if (inputs.empty())
        {
            return false;
        }

        for (
            const auto& endpoint :
            outputs
            )
        {
            if (endpoint.id.empty())
            {
                return false;
            }

            if (
                endpoint.type !=
                srh::engine::
                AudioEndpointType::
                Render
                )
            {
                return false;
            }

            if (
                endpoint.volume < 0.0f ||
                endpoint.volume > 1.0f
                )
            {
                return false;
            }
        }

        for (
            const auto& endpoint :
            inputs
            )
        {
            if (endpoint.id.empty())
            {
                return false;
            }

            if (
                endpoint.type !=
                srh::engine::
                AudioEndpointType::
                Capture
                )
            {
                return false;
            }

            if (
                endpoint.volume < 0.0f ||
                endpoint.volume > 1.0f
                )
            {
                return false;
            }
        }

        const auto defaultOutputCount =
            std::count_if(
                outputs.begin(),
                outputs.end(),
                [](
                    const srh::engine::
                    AudioEndpointInfo& endpoint
                    )
                {
                    return
                        endpoint.isDefault;
                }
            );

        const auto defaultInputCount =
            std::count_if(
                inputs.begin(),
                inputs.end(),
                [](
                    const srh::engine::
                    AudioEndpointInfo& endpoint
                    )
                {
                    return
                        endpoint.isDefault;
                }
            );

        if (
            defaultOutputCount !=
            1
            )
        {
            return false;
        }

        if (
            defaultInputCount !=
            1
            )
        {
            return false;
        }

        defaultOutput =
            FindDefaultEndpoint(
                outputs
            );

        defaultInput =
            FindDefaultEndpoint(
                inputs
            );

        return
            defaultOutput.has_value() &&
            defaultInput.has_value();
    }

    bool TestOutputEndpointExecution(
        srh::engine::SrhEngine& engine,
        const srh::engine::
        AudioEndpointInfo& endpoint
    )
    {
        constexpr srh::engine::ControlId
            ControlId = 30;

        constexpr srh::engine::MappingRuleId
            RuleId = 4;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            EndpointVolumeAdjust;

        action.targetId =
            endpoint.id;

        //
        // Safe no-op:
        // endpoint volume + 0.0
        //

        action.value =
            0.0f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        const auto result =
            SubmitButton(
                engine,
                ControlId,
                6000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Executed &&
            result.AllExecuted();
    }

    bool TestInputEndpointExecution(
        srh::engine::SrhEngine& engine,
        const srh::engine::
        AudioEndpointInfo& endpoint
    )
    {
        constexpr srh::engine::ControlId
            ControlId = 31;

        constexpr srh::engine::MappingRuleId
            RuleId = 5;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            EndpointVolumeAdjust;

        action.targetId =
            endpoint.id;

        //
        // Safe no-op for microphone:
        // current input level + 0.0
        //

        action.value =
            0.0f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        const auto result =
            SubmitButton(
                engine,
                ControlId,
                7000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Executed &&
            result.AllExecuted();
    }

    bool TestMissingEndpointRouting(
        srh::engine::SrhEngine& engine
    )
    {
        constexpr srh::engine::ControlId
            ControlId = 32;

        constexpr srh::engine::MappingRuleId
            RuleId = 6;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            EndpointVolumeSet;

        action.targetId =
            "SRH_ENDPOINT_DOES_NOT_EXIST";

        action.value =
            0.5f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        const auto result =
            SubmitButton(
                engine,
                ControlId,
                8000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        //
        // Endpoint actions are implemented,
        // but the requested device does not exist.
        //

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Failed;
    }

    bool TestUnsupportedExecution(
        srh::engine::SrhEngine& engine
    )
    {
        constexpr srh::engine::ControlId
            ControlId = 40;

        constexpr srh::engine::MappingRuleId
            RuleId = 7;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            LaunchApplication;

        action.argument =
            "not-yet-implemented";

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        const auto result =
            SubmitButton(
                engine,
                ControlId,
                9000
            );

        if (!result.accepted)
        {
            return false;
        }

        if (
            result.executions.size() !=
            1
            )
        {
            return false;
        }

        return
            result.executions[0].status ==
            srh::engine::
            ActionExecutionStatus::
            Unsupported &&
            !result.AllExecuted();
    }
}

int main()
{
    srh::engine::SrhEngine
        engine;

    std::cout
        << "SRH.Engine smoke test\n"
        << "---------------------\n";

    if (
        !TestDeviceRegistry(
            engine
        )
        )
    {
        std::cout
            << "FAIL: DeviceRegistry\n";

        return 1;
    }

    std::cout
        << "PASS: DeviceRegistry\n";

    if (
        !TestMapping(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Mapping\n";

        return 1;
    }

    std::cout
        << "PASS: Mapping\n";

    if (
        !TestInputPipeline(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Input pipeline\n";

        return 1;
    }

    std::cout
        << "PASS: Input pipeline\n";

    if (
        !TestWindowsExecution(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Windows execution\n";

        return 1;
    }

    std::cout
        << "PASS: Windows execution\n";

    if (
        !TestMasterAudioExecution(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Master audio execution\n";

        return 1;
    }

    std::cout
        << "PASS: Master audio execution\n";

    if (
        !TestAudioSessionEnumeration()
        )
    {
        std::cout
            << "FAIL: Audio session enumeration\n";

        return 1;
    }

    std::cout
        << "PASS: Audio session enumeration\n";

    if (
        !TestApplicationAudioRouting(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Application audio routing\n";

        return 1;
    }

    std::cout
        << "PASS: Application audio routing\n";

    std::optional<
        srh::engine::AudioEndpointInfo
    > defaultOutput;

    std::optional<
        srh::engine::AudioEndpointInfo
    > defaultInput;

    if (
        !TestEndpointEnumeration(
            defaultOutput,
            defaultInput
        )
        )
    {
        std::cout
            << "FAIL: Audio endpoint enumeration\n";

        return 1;
    }

    std::cout
        << "PASS: Audio endpoint enumeration\n";

    std::cout
        << "PASS: Default output detection\n";

    std::cout
        << "PASS: Default input detection\n";

    if (
        !TestOutputEndpointExecution(
            engine,
            *defaultOutput
        )
        )
    {
        std::cout
            << "FAIL: Output endpoint execution\n";

        return 1;
    }

    std::cout
        << "PASS: Output endpoint execution\n";

    if (
        !TestInputEndpointExecution(
            engine,
            *defaultInput
        )
        )
    {
        std::cout
            << "FAIL: Input endpoint execution\n";

        return 1;
    }

    std::cout
        << "PASS: Input endpoint execution\n";

    if (
        !TestMissingEndpointRouting(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Missing endpoint routing\n";

        return 1;
    }

    std::cout
        << "PASS: Missing endpoint routing\n";

    if (
        !TestUnsupportedExecution(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Unsupported execution reporting\n";

        return 1;
    }

    std::cout
        << "PASS: Unsupported execution reporting\n";

    std::cout
        << "\nAll smoke tests passed.\n";

    return 0;
}