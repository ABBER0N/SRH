#include "ConsoleUtf8.h"
#include "HubProtocolSmokeTest.h"

#include "Public/SrhEngine.h"

#include "Execution/Windows/Audio/AudioEndpointService.h"
#include "Execution/Windows/Audio/AudioService.h"
#include "Execution/Windows/Text/Utf8.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
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

    constexpr srh::engine::VirtualDeviceId
        TestVirtualDeviceId = 0;

    class VirtualControllerSessionGuard
    {
    public:
        explicit VirtualControllerSessionGuard(
            srh::engine::SrhEngine& engine
        )
            : m_engine(
                engine
            )
        {
        }

        ~VirtualControllerSessionGuard()
        {
            (void)m_engine
                .ResetVirtualControllers();

            m_engine
                .DisconnectVirtualControllerDriver();
        }

        VirtualControllerSessionGuard(
            const VirtualControllerSessionGuard&
        ) = delete;

        VirtualControllerSessionGuard&
            operator=(
                const VirtualControllerSessionGuard&
                ) = delete;

    private:
        srh::engine::SrhEngine&
            m_engine;
    };

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
            srh::engine::
            InputValueCondition::
            Any;

        rule.action =
            std::move(action);

        rule.valueMode =
            srh::engine::
            ActionValueMode::
            Fixed;

        engine.AddOrUpdateMappingRule(
            std::move(rule)
        );
    }

    void AddVirtualControllerRule(
        srh::engine::SrhEngine& engine,
        const srh::engine::MappingRuleId ruleId,
        const srh::engine::ControlId inputControlId,
        const srh::engine::InputEventType eventType,
        const srh::engine::
        VirtualControllerActionKind actionKind,
        const srh::engine::VirtualDeviceId deviceId,
        const srh::engine::ControlId outputControlId,
        const std::int32_t outputValue
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
            inputControlId;

        rule.input.eventType =
            eventType;

        rule.input.valueCondition =
            srh::engine::
            InputValueCondition::
            Any;

        srh::engine::VirtualControllerAction
            action;

        action.kind =
            actionKind;

        action.deviceId =
            deviceId;

        action.controlId =
            outputControlId;

        action.value =
            outputValue;

        rule.action =
            action;

        rule.valueMode =
            srh::engine::
            ActionValueMode::
            Fixed;

        engine.AddOrUpdateMappingRule(
            std::move(rule)
        );
    }

    srh::engine::InputProcessingResult
        SubmitInput(
            srh::engine::SrhEngine& engine,
            const srh::engine::ControlId controlId,
            const srh::engine::InputEventType type,
            const std::int32_t value,
            const std::uint64_t timestamp
        )
    {
        srh::engine::InputEvent event;

        event.nodeId =
            TestNodeId;

        event.controlId =
            controlId;

        event.type =
            type;

        event.value =
            value;

        event.timestamp =
            timestamp;

        return
            engine.SubmitInputEvent(
                event
            );
    }

    srh::engine::InputProcessingResult
        SubmitButton(
            srh::engine::SrhEngine& engine,
            const srh::engine::ControlId controlId,
            const std::uint64_t timestamp
        )
    {
        return
            SubmitInput(
                engine,
                controlId,
                srh::engine::
                InputEventType::
                ButtonDown,
                1,
                timestamp
            );
    }

    bool IsSingleExecutionSuccessful(
        const srh::engine::
        InputProcessingResult& result
    )
    {
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

    bool IsSingleExecutionFailed(
        const srh::engine::
        InputProcessingResult& result
    )
    {
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
            Failed &&
            !result.AllExecuted();
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

    const char* MediaPlaybackStatusToString(
        const srh::engine::
        MediaPlaybackStatus status
    )
    {
        using Status =
            srh::engine::
            MediaPlaybackStatus;

        switch (status)
        {
        case Status::Closed:
            return "Closed";

        case Status::Opened:
            return "Opened";

        case Status::Changing:
            return "Changing";

        case Status::Stopped:
            return "Stopped";

        case Status::Playing:
            return "Playing";

        case Status::Paused:
            return "Paused";

        case Status::Unknown:
        default:
            return "Unknown";
        }
    }

    bool TestUtf8RoundTrip()
    {
        const std::string original =
            "Привет, SRH — русский текст — 日本語 — 🚗";

        const std::wstring wide =
            srh::engine::windows::
            Utf8ToWide(
                original
            );

        if (wide.empty())
        {
            return false;
        }

        const std::string converted =
            srh::engine::windows::
            WideToUtf8(
                wide
            );

        if (
            converted !=
            original
            )
        {
            return false;
        }

        std::cout
            << "INFO: UTF-8 text: "
            << converted
            << '\n';

        return true;
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
            srh::engine::
            DeviceHealth::
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
            srh::engine::
            NodeType::
            ButtonBox;

        node.name =
            "Тестовый модуль SRH";

        node.connected =
            true;

        node.configured =
            true;

        node.enabled =
            true;

        node.active =
            true;

        node.health =
            srh::engine::
            DeviceHealth::
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

        if (
            snapshot.nodes[0].nodeId !=
            TestNodeId
            )
        {
            return false;
        }

        if (
            snapshot.nodes[0].name !=
            "Тестовый модуль SRH"
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
        srh::engine::SystemAction action;

        action.kind =
            srh::engine::
            SystemActionKind::
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
            srh::engine::
            InputEventType::
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

        return
            IsSingleExecutionSuccessful(
                result
            );
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
            srh::engine::
            SystemActionKind::
            MasterVolumeAdjust;

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

        return
            IsSingleExecutionSuccessful(
                result
            );
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
            srh::engine::
            SystemActionKind::
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

        return
            IsSingleExecutionFailed(
                result
            );
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

        if (
            outputs.empty() ||
            inputs.empty()
            )
        {
            return false;
        }

        for (
            const auto& endpoint :
            outputs
            )
        {
            if (
                endpoint.id.empty() ||
                endpoint.type !=
                srh::engine::
                AudioEndpointType::
                Render ||
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
            if (
                endpoint.id.empty() ||
                endpoint.type !=
                srh::engine::
                AudioEndpointType::
                Capture ||
                endpoint.volume < 0.0f ||
                endpoint.volume > 1.0f
                )
            {
                return false;
            }
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
            srh::engine::
            SystemActionKind::
            EndpointVolumeAdjust;

        action.targetId =
            endpoint.id;

        action.value =
            0.0f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        return
            IsSingleExecutionSuccessful(
                SubmitButton(
                    engine,
                    ControlId,
                    6000
                )
            );
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
            srh::engine::
            SystemActionKind::
            EndpointVolumeAdjust;

        action.targetId =
            endpoint.id;

        action.value =
            0.0f;

        AddSystemActionRule(
            engine,
            RuleId,
            ControlId,
            action
        );

        return
            IsSingleExecutionSuccessful(
                SubmitButton(
                    engine,
                    ControlId,
                    7000
                )
            );
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
            srh::engine::
            SystemActionKind::
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

        return
            IsSingleExecutionFailed(
                SubmitButton(
                    engine,
                    ControlId,
                    8000
                )
            );
    }

    bool TestMediaState(
        srh::engine::SrhEngine& engine
    )
    {
        const auto media =
            engine.GetCurrentMediaSession();

        if (!media.has_value())
        {
            std::cout
                << "INFO: No active Windows media session\n";

            return true;
        }

        std::cout
            << "INFO: Media source: "
            << media->sourceAppId
            << '\n';

        std::cout
            << "INFO: Media title: "
            << media->title
            << '\n';

        std::cout
            << "INFO: Media artist: "
            << media->artist
            << '\n';

        std::cout
            << "INFO: Media album: "
            << media->albumTitle
            << '\n';

        std::cout
            << "INFO: Media status: "
            << MediaPlaybackStatusToString(
                media->playbackStatus
            )
            << '\n';

        return true;
    }

    bool TestVirtualControllerExecution(
        srh::engine::SrhEngine& engine
    )
    {
        VirtualControllerSessionGuard
            sessionGuard(
                engine
            );

        if (
            !engine
            .ConnectVirtualControllerDriver()
            )
        {
            std::cout
                << "INFO: Virtual controller driver error: "
                << engine
                .GetVirtualControllerDriverError()
                << '\n';

            return false;
        }

        constexpr srh::engine::ControlId
            ButtonInputControl = 100;

        constexpr srh::engine::ControlId
            AxisInputControl = 101;

        constexpr srh::engine::ControlId
            PovInputControl = 102;

        AddVirtualControllerRule(
            engine,
            100,
            ButtonInputControl,
            srh::engine::
            InputEventType::
            ButtonDown,
            srh::engine::
            VirtualControllerActionKind::
            Button,
            TestVirtualDeviceId,
            1,
            1
        );

        AddVirtualControllerRule(
            engine,
            101,
            ButtonInputControl,
            srh::engine::
            InputEventType::
            ButtonUp,
            srh::engine::
            VirtualControllerActionKind::
            Button,
            TestVirtualDeviceId,
            1,
            0
        );

        AddVirtualControllerRule(
            engine,
            102,
            AxisInputControl,
            srh::engine::
            InputEventType::
            AxisValue,
            srh::engine::
            VirtualControllerActionKind::
            Axis,
            TestVirtualDeviceId,
            1,
            0
        );

        AddVirtualControllerRule(
            engine,
            103,
            PovInputControl,
            srh::engine::
            InputEventType::
            AxisValue,
            srh::engine::
            VirtualControllerActionKind::
            Pov,
            TestVirtualDeviceId,
            1,
            -1
        );

        auto result =
            SubmitInput(
                engine,
                ButtonInputControl,
                srh::engine::
                InputEventType::
                ButtonDown,
                1,
                10000
            );

        if (
            !IsSingleExecutionSuccessful(
                result
            )
            )
        {
            return false;
        }

        std::cout
            << "INFO: Virtual Button 1 down submitted\n";

        result =
            SubmitInput(
                engine,
                ButtonInputControl,
                srh::engine::
                InputEventType::
                ButtonUp,
                0,
                10001
            );

        if (
            !IsSingleExecutionSuccessful(
                result
            )
            )
        {
            return false;
        }

        std::cout
            << "INFO: Virtual Button 1 up submitted\n";

        result =
            SubmitInput(
                engine,
                AxisInputControl,
                srh::engine::
                InputEventType::
                AxisValue,
                16384,
                10002
            );

        if (
            !IsSingleExecutionSuccessful(
                result
            )
            )
        {
            return false;
        }

        std::cout
            << "INFO: Virtual Axis 1 submitted\n";

        result =
            SubmitInput(
                engine,
                PovInputControl,
                srh::engine::
                InputEventType::
                AxisValue,
                9000,
                10003
            );

        if (
            !IsSingleExecutionSuccessful(
                result
            )
            )
        {
            return false;
        }

        std::cout
            << "INFO: Virtual POV 1 submitted\n";

        if (
            !engine
            .ResetVirtualControllers()
            )
        {
            return false;
        }

        std::cout
            << "INFO: Virtual controller neutral state submitted\n";

        return true;
    }

    bool TestVirtualControllerLifecycle()
    {
        srh::engine::SrhEngine
            engine;

        VirtualControllerSessionGuard
            sessionGuard(
                engine
            );

        auto controller =
            engine.FindVirtualController(
                0
            );

        if (
            !controller.has_value() ||
            !controller->enabled ||
            controller->active
            )
        {
            return false;
        }

        srh::engine::VirtualControllerState
            secondaryController;

        secondaryController.deviceId =
            1;

        secondaryController.name =
            "Виртуальный контроллер 2";

        secondaryController.enabled =
            true;

        secondaryController.active =
            true;

        engine.AddOrUpdateVirtualController(
            secondaryController
        );

        const auto secondary =
            engine.FindVirtualController(
                1
            );

        if (
            !secondary.has_value() ||
            secondary->active
            )
        {
            return false;
        }

        constexpr srh::engine::ControlId
            UnknownInput = 200;

        AddVirtualControllerRule(
            engine,
            200,
            UnknownInput,
            srh::engine::
            InputEventType::
            ButtonDown,
            srh::engine::
            VirtualControllerActionKind::
            Button,
            60000,
            1,
            1
        );

        if (
            !IsSingleExecutionFailed(
                SubmitButton(
                    engine,
                    UnknownInput,
                    20000
                )
            )
            )
        {
            return false;
        }

        if (
            !engine
            .ConnectVirtualControllerDriver()
            )
        {
            return false;
        }

        constexpr srh::engine::ControlId
            DeviceInput = 201;

        AddVirtualControllerRule(
            engine,
            201,
            DeviceInput,
            srh::engine::
            InputEventType::
            ButtonDown,
            srh::engine::
            VirtualControllerActionKind::
            Button,
            0,
            2,
            1
        );

        if (
            !IsSingleExecutionSuccessful(
                SubmitButton(
                    engine,
                    DeviceInput,
                    20001
                )
            )
            )
        {
            return false;
        }

        controller =
            engine.FindVirtualController(
                0
            );

        if (
            !controller.has_value() ||
            !controller->active
            )
        {
            return false;
        }

        if (
            !engine
            .SetVirtualControllerEnabled(
                0,
                false
            )
            )
        {
            return false;
        }

        controller =
            engine.FindVirtualController(
                0
            );

        if (
            !controller.has_value() ||
            controller->enabled ||
            controller->active
            )
        {
            return false;
        }

        if (
            !IsSingleExecutionFailed(
                SubmitButton(
                    engine,
                    DeviceInput,
                    20002
                )
            )
            )
        {
            return false;
        }

        if (
            !engine
            .SetVirtualControllerEnabled(
                0,
                true
            )
            )
        {
            return false;
        }

        if (
            !engine
            .RemoveVirtualController(
                0
            )
            )
        {
            return false;
        }

        if (
            engine
            .FindVirtualController(
                0
            )
            .has_value()
            )
        {
            return false;
        }

        if (
            !engine
            .RemoveVirtualController(
                1
            )
            )
        {
            return false;
        }

        std::cout
            << "INFO: Virtual controller registry lifecycle verified\n";

        return true;
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
            srh::engine::
            SystemActionKind::
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
                30000
            );

        if (
            !result.accepted ||
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
    srh::smoketest::
        ConfigureUtf8Console();

    srh::engine::SrhEngine
        engine;

    std::cout
        << "SRH.Engine smoke test\n"
        << "---------------------\n";

    if (
        !TestUtf8RoundTrip()
        )
    {
        std::cout
            << "FAIL: UTF-8 round trip\n";

        return 1;
    }

    std::cout
        << "PASS: UTF-8 round trip\n";

    //
    // Hub protocol framing.
    //

    if (
        !srh::smoketest::
        RunHubProtocolSmokeTest()
        )
    {
        std::cout
            << "FAIL: Hub protocol framing\n";

        return 1;
    }

    std::cout
        << "PASS: Hub protocol framing\n";

    //
    // Hub message codec.
    //

    if (
        !srh::smoketest::
        RunHubMessageSmokeTest()
        )
    {
        std::cout
            << "FAIL: Hub message codec\n";

        return 1;
    }

    std::cout
        << "PASS: Hub message codec\n";

    //
    // Hub ingress -> SrhEngine.
    //

    if (
        !srh::smoketest::
        RunHubIngressSmokeTest()
        )
    {
        std::cout
            << "FAIL: Hub ingress pipeline\n";

        return 1;
    }

    std::cout
        << "PASS: Hub ingress pipeline\n";

    //
    // Windows USB CDC / COM transport API.
    //
    // This test is fully offline and does not
    // require an ESP32-S3 or a real COM port.
    //

    if (
        !srh::smoketest::
        RunHubSerialTransportSmokeTest()
        )
    {
        std::cout
            << "FAIL: Hub serial transport\n";

        return 1;
    }

    std::cout
        << "PASS: Hub serial transport\n";

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
        !TestMediaState(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Media state query\n";

        return 1;
    }

    std::cout
        << "PASS: Media state query\n";

    if (
        !TestVirtualControllerExecution(
            engine
        )
        )
    {
        std::cout
            << "FAIL: Virtual controller execution\n";

        return 1;
    }

    std::cout
        << "PASS: Virtual controller execution\n";

    if (
        !TestVirtualControllerLifecycle()
        )
    {
        std::cout
            << "FAIL: Virtual controller lifecycle\n";

        return 1;
    }

    std::cout
        << "PASS: Virtual controller lifecycle\n";

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