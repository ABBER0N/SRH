#include "Public/SrhEngine.h"

#include <cstdint>
#include <iostream>
#include <variant>

namespace
{
    constexpr srh::engine::NodeId
        TestNodeId = 1;

    constexpr srh::engine::ControlId
        TestControlId = 10;

    constexpr srh::engine::MappingRuleId
        TestMappingRuleId = 1;

    //
    // Windows virtual-key code for F24.
    //
    // We intentionally use F24 because it normally
    // has no visible side effect in ordinary software.
    //
    constexpr std::uint32_t
        TestVirtualKey = 0x87;

    bool TestDeviceRegistry(
        srh::engine::SrhEngine& engine
    )
    {
        srh::engine::HubState hub;

        hub.connected = true;
        hub.name = "SRH Test Hub";
        hub.firmwareVersion = "0.1.0";
        hub.protocolVersion = "1";
        hub.health =
            srh::engine::DeviceHealth::Healthy;

        engine.SetHubState(
            hub
        );

        srh::engine::NodeState node;

        node.nodeId =
            TestNodeId;

        node.physicalUid =
            0x12345678;

        node.type =
            srh::engine::NodeType::ButtonBox;

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
            srh::engine::DeviceHealth::Healthy;

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

        if (
            snapshot.nodes[0].nodeId !=
            TestNodeId
            )
        {
            return false;
        }

        return true;
    }

    bool TestMapping(
        srh::engine::SrhEngine& engine
    )
    {
        srh::engine::MappingRule rule;

        rule.id =
            TestMappingRuleId;

        rule.enabled =
            true;

        rule.input.nodeId =
            TestNodeId;

        rule.input.controlId =
            TestControlId;

        rule.input.eventType =
            srh::engine::InputEventType::
            ButtonDown;

        rule.input.valueCondition =
            srh::engine::InputValueCondition::
            Any;

        srh::engine::SystemAction action;

        action.kind =
            srh::engine::SystemActionKind::
            KeyboardPress;

        action.code =
            TestVirtualKey;

        rule.action =
            action;

        rule.valueMode =
            srh::engine::ActionValueMode::
            Fixed;

        engine.AddOrUpdateMappingRule(
            rule
        );

        srh::engine::InputEvent event;

        event.nodeId =
            TestNodeId;

        event.controlId =
            TestControlId;

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

        if (
            systemAction->kind !=
            srh::engine::SystemActionKind::
            KeyboardPress
            )
        {
            return false;
        }

        if (
            systemAction->code !=
            TestVirtualKey
            )
        {
            return false;
        }

        return true;
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
                    const srh::engine::InputEvent& event
                    )
                {
                    if (
                        event.nodeId ==
                        TestNodeId &&
                        event.controlId ==
                        TestControlId &&
                        event.type ==
                        srh::engine::InputEventType::
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

        srh::engine::InputEvent event;

        event.nodeId =
            TestNodeId;

        event.controlId =
            TestControlId;

        event.type =
            srh::engine::InputEventType::
            ButtonDown;

        event.value =
            1;

        event.timestamp =
            2000;

        const bool submitted =
            engine.SubmitInputEvent(
                event
            );

        engine.UnsubscribeInput(
            subscriptionId
        );

        if (!submitted)
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
                TestControlId
            );

        if (!state.has_value())
        {
            return false;
        }

        if (
            state->value !=
            1
            )
        {
            return false;
        }

        if (
            state->type !=
            srh::engine::InputEventType::
            ButtonDown
            )
        {
            return false;
        }

        return true;
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

    std::cout
        << "PASS: Mapping -> Action -> Windows execution\n";

    std::cout
        << "\nAll smoke tests passed.\n";

    return 0;
}