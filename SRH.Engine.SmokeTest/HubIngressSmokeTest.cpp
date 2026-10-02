#include "HubIngressSmokeTest.h"

#include "Hub/HubIngressService.h"
#include "Hub/Protocol/HubMessageCodec.h"
#include "Hub/Protocol/HubProtocol.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

namespace
{
    using namespace
        srh::engine;

    using namespace
        srh::engine::hub;

    using namespace
        srh::engine::hub::protocol;

    std::vector<std::uint8_t>
        EncodeMessageBytes(
            const HubMessage& message,
            const std::uint32_t sequence,
            const HubPacketFlags flags =
            HubPacketFlags::None
        )
    {
        const auto packet =
            HubMessageCodec::Encode(
                message,
                sequence,
                flags
            );

        if (!packet.has_value())
        {
            return {};
        }

        return
            HubProtocol::Encode(
                *packet
            );
    }

    void AddTestMapping(
        SrhEngine& engine
    )
    {
        MappingRule rule;

        rule.id =
            9000;

        rule.enabled =
            true;

        rule.input.nodeId =
            42;

        rule.input.controlId =
            7;

        rule.input.eventType =
            InputEventType::
            ButtonDown;

        rule.input.valueCondition =
            InputValueCondition::
            Any;

        VirtualControllerAction action;

        action.kind =
            VirtualControllerActionKind::
            Button;

        //
        // This DeviceId is deliberately not
        // registered.
        //
        // It proves that the event travelled through
        // Mapping and ActionExecutor without sending
        // anything to the real virtual-device driver.
        //

        action.deviceId =
            60000;

        action.controlId =
            1;

        action.value =
            1;

        rule.action =
            action;

        rule.valueMode =
            ActionValueMode::
            Fixed;

        engine.AddOrUpdateMappingRule(
            std::move(
                rule
            )
        );
    }

    bool TestInputBeforeHelloRejected()
    {
        SrhEngine engine;

        HubIngressService ingress(
            engine
        );

        InputEvent event;

        event.nodeId =
            42;

        event.controlId =
            7;

        event.type =
            InputEventType::
            ButtonDown;

        event.value =
            1;

        event.timestamp =
            100;

        const auto bytes =
            EncodeMessageBytes(
                HubMessage{
                    event
                },
                1
            );

        if (bytes.empty())
        {
            return false;
        }

        const auto result =
            ingress.Push(
                bytes
            );

        if (
            result.packetCount !=
            1 ||
            result.messageCount !=
            0 ||
            result.rejectedMessageCount !=
            1 ||
            !result.inputResults.empty()
            )
        {
            return false;
        }

        return
            !engine
            .FindInputState(
                42,
                7
            )
            .has_value();
    }

    bool TestHandshakeAndInputPipeline()
    {
        SrhEngine engine;

        HubIngressService ingress(
            engine
        );

        AddTestMapping(
            engine
        );

        std::uint32_t receivedEvents =
            0;

        const auto subscriptionId =
            engine.SubscribeInput(
                [&receivedEvents](
                    const InputEvent& event
                    )
                {
                    if (
                        event.nodeId ==
                        42 &&
                        event.controlId ==
                        7 &&
                        event.type ==
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

        //
        // 1. Hub Hello.
        //

        HelloMessage hello;

        hello.hubUid =
            0x1122334455667788ull;

        hello.firmwareMajor =
            2;

        hello.firmwareMinor =
            7;

        hello.firmwarePatch =
            11;

        hello.hardwareRevision =
            3;

        const auto helloBytes =
            EncodeMessageBytes(
                HubMessage{
                    hello
                },
                10
            );

        if (
            helloBytes.size() <
            3
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        //
        // Simulate USB CDC fragmentation.
        //

        const auto firstHelloChunk =
            ingress.Push(
                helloBytes.data(),
                3
            );

        if (
            firstHelloChunk.packetCount !=
            0 ||
            firstHelloChunk.messageCount !=
            0
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto secondHelloChunk =
            ingress.Push(
                helloBytes.data() + 3,
                helloBytes.size() - 3
            );

        if (
            secondHelloChunk.packetCount !=
            1 ||
            secondHelloChunk.messageCount !=
            1 ||
            secondHelloChunk.rejectedMessageCount !=
            0
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        if (
            !ingress
            .IsSessionEstablished()
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const HubState hubState =
            engine.GetHubState();

        if (
            !hubState.connected ||
            hubState.physicalUid !=
            hello.hubUid ||
            hubState.firmwareVersion !=
            "2.7.11" ||
            hubState.protocolVersion !=
            "1" ||
            hubState.health !=
            DeviceHealth::
            Healthy
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        //
        // 2. Physical InputEvent from Hub.
        //

        InputEvent event;

        event.nodeId =
            42;

        event.controlId =
            7;

        event.type =
            InputEventType::
            ButtonDown;

        event.value =
            1;

        event.timestamp =
            123456789ull;

        const auto inputBytes =
            EncodeMessageBytes(
                HubMessage{
                    event
                },
                11
            );

        if (inputBytes.empty())
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        std::vector<InputProcessingResult>
            inputResults;

        std::size_t acceptedMessages =
            0;

        //
        // Feed one byte at a time.
        //

        for (
            const std::uint8_t byte :
        inputBytes
            )
        {
            auto result =
                ingress.Push(
                    &byte,
                    1
                );

            acceptedMessages +=
                result.messageCount;

            inputResults.insert(
                inputResults.end(),
                result.inputResults.begin(),
                result.inputResults.end()
            );
        }

        if (
            acceptedMessages !=
            1 ||
            inputResults.size() !=
            1
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto& processingResult =
            inputResults[0];

        if (
            !processingResult.accepted ||
            processingResult.executions.size() !=
            1
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        //
        // Unknown virtual DeviceId = 60000 must
        // fail at ActionExecutor/registry level.
        //
        // This proves that Hub input reached the
        // existing mapping/execution pipeline.
        //

        if (
            processingResult
            .executions[0]
            .status !=
            ActionExecutionStatus::
            Failed
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        if (
            receivedEvents !=
            1
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto inputState =
            engine.FindInputState(
                event.nodeId,
                event.controlId
            );

        if (
            !inputState.has_value() ||
            inputState->type !=
            event.type ||
            inputState->value !=
            event.value ||
            inputState->timestamp !=
            event.timestamp
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        //
        // 3. Heartbeat.
        //

        HeartbeatMessage heartbeat;

        heartbeat.uptimeMs =
            987654321ull;

        const auto heartbeatBytes =
            EncodeMessageBytes(
                HubMessage{
                    heartbeat
                },
                12
            );

        if (heartbeatBytes.empty())
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto heartbeatResult =
            ingress.Push(
                heartbeatBytes
            );

        if (
            heartbeatResult.packetCount !=
            1 ||
            heartbeatResult.messageCount !=
            1 ||
            heartbeatResult.rejectedMessageCount !=
            0
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto lastHeartbeat =
            ingress
            .LastHeartbeatUptimeMs();

        if (
            !lastHeartbeat.has_value() ||
            *lastHeartbeat !=
            heartbeat.uptimeMs
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        //
        // 4. HelloAck is PC -> Hub and therefore
        // must be rejected when received from Hub.
        //

        HelloAckMessage helloAck;

        helloAck.heartbeatIntervalMs =
            1000;

        const auto helloAckBytes =
            EncodeMessageBytes(
                HubMessage{
                    helloAck
                },
                13,
                HubPacketFlags::
                Response
            );

        if (helloAckBytes.empty())
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto helloAckResult =
            ingress.Push(
                helloAckBytes
            );

        if (
            helloAckResult.packetCount !=
            1 ||
            helloAckResult.messageCount !=
            0 ||
            helloAckResult.rejectedMessageCount !=
            1
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        //
        // 5. Simulate USB disconnect.
        //

        ingress.Reset();

        if (
            ingress
            .IsSessionEstablished()
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        if (
            ingress
            .LastHeartbeatUptimeMs()
            .has_value()
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        const auto disconnectedState =
            engine.GetHubState();

        if (
            disconnectedState.connected ||
            disconnectedState.health !=
            DeviceHealth::
            Unknown ||
            disconnectedState.statusMessage !=
            "Disconnected"
            )
        {
            engine.UnsubscribeInput(
                subscriptionId
            );

            return false;
        }

        engine.UnsubscribeInput(
            subscriptionId
        );

        return true;
    }
}

namespace srh::smoketest
{
    bool RunHubIngressSmokeTest()
    {
        if (
            !TestInputBeforeHelloRejected()
            )
        {
            std::cout
                << "FAIL: Hub ingress session gate\n";

            return false;
        }

        if (
            !TestHandshakeAndInputPipeline()
            )
        {
            std::cout
                << "FAIL: Hub ingress Engine pipeline\n";

            return false;
        }

        std::cout
            << "INFO: Hub ingress Engine pipeline verified\n";

        return true;
    }
}