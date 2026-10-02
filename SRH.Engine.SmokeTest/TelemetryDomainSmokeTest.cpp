#include "TelemetryDomainSmokeTest.h"

#include "Game/Domain/TelemetryAggregateState.h"
#include "Game/Domain/TelemetrySnapshot.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using namespace
        srh::engine::game;

    bool TestTelemetryValueStates()
    {
        TelemetryValue<double>
            value;

        if (
            value.state !=
            TelemetryValueState::
            Unsupported
            )
        {
            return false;
        }

        if (value.IsSupported())
        {
            return false;
        }

        if (value.HasValue())
        {
            return false;
        }

        value.SetUnavailable();

        if (
            value.state !=
            TelemetryValueState::
            Unavailable
            )
        {
            return false;
        }

        if (!value.IsSupported())
        {
            return false;
        }

        if (value.HasValue())
        {
            return false;
        }

        value.SetValue(
            0.0
        );

        if (!value.IsSupported())
        {
            return false;
        }

        if (!value.HasValue())
        {
            return false;
        }

        if (
            value.value !=
            0.0
            )
        {
            return false;
        }

        value.SetUnsupported();

        return
            !value.IsSupported() &&
            !value.HasValue();
    }

    bool TestNormalizedSnapshot()
    {
        TelemetrySnapshot
            snapshot;

        snapshot.game.providerId =
            "acc";

        snapshot.game.gameId =
            "assetto-corsa-competizione";

        snapshot.game.gameName =
            "Assetto Corsa Competizione";

        snapshot.game.connected =
            true;

        snapshot.game.runState =
            GameRunState::Active;

        snapshot.player
            .powertrain
            .engineRpm
            .SetValue(
                7200.0
            );

        snapshot.player
            .motion
            .speedMps
            .SetValue(
                55.0
            );

        snapshot.player
            .controls
            .throttleInput01
            .SetValue(
                1.0
            );

        snapshot.player
            .controls
            .brakeInput01
            .SetValue(
                0.0
            );

        if (
            !snapshot.player
            .powertrain
            .engineRpm
            .HasValue()
            )
        {
            return false;
        }

        if (
            snapshot.player
            .powertrain
            .engineRpm
            .value !=
            7200.0
            )
        {
            return false;
        }

        if (
            !snapshot.player
            .controls
            .brakeInput01
            .HasValue()
            )
        {
            return false;
        }

        //
        // Zero is a valid value.
        //
        // It must never implicitly mean
        // "unsupported" or "unavailable".
        //

        if (
            snapshot.player
            .controls
            .brakeInput01
            .value !=
            0.0
            )
        {
            return false;
        }

        return true;
    }

    bool TestParticipantPositionSemantics()
    {
        TelemetrySnapshot
            snapshot;

        ParticipantState
            participant;

        participant.participantId =
            42;

        participant.driver.lastName =
            "Test";

        participant.positionSource =
            ParticipantPositionSource::
            WorldCoordinates;

        participant
            .normalizedTrackPosition
            .SetValue(
                0.375
            );

        participant
            .worldPositionMeters
            .SetValue(
                Vec3{
                    100.0,
                    20.0,
                    5.0
                }
            );

        snapshot.participants.push_back(
            std::move(
                participant
            )
        );

        if (
            snapshot.participants.size() !=
            1
            )
        {
            return false;
        }

        const auto& stored =
            snapshot.participants.front();

        if (
            stored.positionSource !=
            ParticipantPositionSource::
            WorldCoordinates
            )
        {
            return false;
        }

        if (
            !stored
            .worldPositionMeters
            .HasValue()
            )
        {
            return false;
        }

        if (
            !stored
            .normalizedTrackPosition
            .HasValue()
            )
        {
            return false;
        }

        return true;
    }

    bool TestDynamicNativeChannel()
    {
        TelemetrySnapshot
            snapshot;

        NativeChannel
            channel;

        channel.metadata.name =
            "iRacing.CarIdxLapDistPct";

        channel.metadata.type =
            NativeChannelType::
            Float32Array;

        channel.metadata.elementCount =
            64;

        channel.metadata.unit =
            "%";

        channel.metadata.description =
            "Per-car normalized lap distance";

        channel.valid =
            true;

        channel.value =
            std::vector<float>{
                0.10f,
                0.25f,
                0.75f
        };

        snapshot
            .nativeChannels
            .channels
            .push_back(
                std::move(
                    channel
                )
            );

        if (
            snapshot
            .nativeChannels
            .channels
            .size() !=
            1
            )
        {
            return false;
        }

        const auto& stored =
            snapshot
            .nativeChannels
            .channels
            .front();

        if (
            stored.metadata.name !=
            "iRacing.CarIdxLapDistPct"
            )
        {
            return false;
        }

        if (!stored.valid)
        {
            return false;
        }

        const auto* values =
            std::get_if<
            std::vector<float>
            >(
                &stored.value
            );

        if (values == nullptr)
        {
            return false;
        }

        return
            values->size() ==
            3;
    }

    bool TestGranularCapabilities()
    {
        TelemetrySnapshot
            snapshot;

        snapshot
            .capabilities
            .fields
            .push_back(
                TelemetryCapabilityEntry{
                    "vehicle.powertrain.engineRpm",
                    true,
                    "ACC.Physics.rpms"
                }
            );

        snapshot
            .capabilities
            .fields
            .push_back(
                TelemetryCapabilityEntry{
                    "vehicle.tires.innerTemperature",
                    false,
                    {}
                }
            );

        if (
            snapshot
            .capabilities
            .fields
            .size() !=
            2
            )
        {
            return false;
        }

        return
            snapshot
            .capabilities
            .fields[0]
            .supported &&
            !snapshot
            .capabilities
            .fields[1]
            .supported;
    }

    bool TestTelemetryStatePartitioning()
    {
        TelemetryState
            state;

        state.epoch =
            7;

        //
        // Static update.
        //

        state.staticState.generation =
            2;

        state.staticState
            .game
            .providerId =
            "acc";

        state.staticState
            .game
            .gameName =
            "Assetto Corsa Competizione";

        state.staticState
            .track
            .name =
            "Monza";

        state.staticState
            .playerVehicle
            .className =
            "GT3";

        //
        // Session update.
        //

        state.sessionState.generation =
            31;

        state.sessionState
            .session
            .type =
            SessionType::Race;

        state.sessionState
            .session
            .playerOverallPosition
            .SetValue(
                8
            );

        ParticipantState
            participant;

        participant.participantId =
            1001;

        participant
            .overallPosition
            .SetValue(
                1
            );

        participant
            .normalizedTrackPosition
            .SetValue(
                0.64
            );

        participant.positionSource =
            ParticipantPositionSource::
            NormalizedTrackPosition;

        state.sessionState
            .participants
            .push_back(
                std::move(
                    participant
                )
            );

        //
        // Realtime updates may advance many times
        // without changing Static or Session
        // generations.
        //

        state.realtimeState.generation =
            14820;

        state.realtimeState
            .powertrain
            .engineRpm
            .SetValue(
                6942.0
            );

        state.realtimeState
            .motion
            .speedMps
            .SetValue(
                61.5
            );

        state.realtimeState
            .controls
            .throttleInput01
            .SetValue(
                0.91
            );

        //
        // Native channels have their own update
        // generation and therefore do not force
        // normalized generations to change.
        //

        state.nativeState.generation =
            21000;

        NativeChannel
            rpmChannel;

        rpmChannel.metadata.name =
            "ACC.Physics.rpms";

        rpmChannel.metadata.type =
            NativeChannelType::
            Int32;

        rpmChannel.metadata.elementCount =
            1;

        rpmChannel.metadata.unit =
            "rpm";

        rpmChannel.valid =
            true;

        rpmChannel.value =
            std::int32_t{
                6942
        };

        state.nativeState
            .channels
            .channels
            .push_back(
                std::move(
                    rpmChannel
                )
            );

        if (
            state.epoch !=
            7
            )
        {
            return false;
        }

        if (
            state.staticState.generation !=
            2
            )
        {
            return false;
        }

        if (
            state.sessionState.generation !=
            31
            )
        {
            return false;
        }

        if (
            state.realtimeState.generation !=
            14820
            )
        {
            return false;
        }

        if (
            state.nativeState.generation !=
            21000
            )
        {
            return false;
        }

        if (
            state.staticState
            .track
            .name !=
            "Monza"
            )
        {
            return false;
        }

        if (
            state.sessionState
            .participants
            .size() !=
            1
            )
        {
            return false;
        }

        if (
            !state.realtimeState
            .powertrain
            .engineRpm
            .HasValue()
            )
        {
            return false;
        }

        if (
            state.realtimeState
            .powertrain
            .engineRpm
            .value !=
            6942.0
            )
        {
            return false;
        }

        if (
            state.nativeState
            .channels
            .channels
            .size() !=
            1
            )
        {
            return false;
        }

        const auto& nativeChannel =
            state.nativeState
            .channels
            .channels
            .front();

        if (
            nativeChannel.metadata.name !=
            "ACC.Physics.rpms"
            )
        {
            return false;
        }

        const auto* nativeRpm =
            std::get_if<std::int32_t>(
                &nativeChannel.value
            );

        if (nativeRpm == nullptr)
        {
            return false;
        }

        return
            *nativeRpm ==
            6942;
    }
}

namespace srh::smoketest
{
    bool RunTelemetryDomainSmokeTest()
    {
        if (!TestTelemetryValueStates())
        {
            std::cout
                << "FAIL: Telemetry value availability semantics\n";

            return false;
        }

        if (!TestNormalizedSnapshot())
        {
            std::cout
                << "FAIL: Normalized telemetry snapshot\n";

            return false;
        }

        if (!TestParticipantPositionSemantics())
        {
            std::cout
                << "FAIL: Participant position semantics\n";

            return false;
        }

        if (!TestDynamicNativeChannel())
        {
            std::cout
                << "FAIL: Dynamic native telemetry channel\n";

            return false;
        }

        if (!TestGranularCapabilities())
        {
            std::cout
                << "FAIL: Granular telemetry capabilities\n";

            return false;
        }

        if (!TestTelemetryStatePartitioning())
        {
            std::cout
                << "FAIL: Telemetry state partitioning\n";

            return false;
        }

        std::cout
            << "INFO: Telemetry state partitioning verified\n";

        std::cout
            << "INFO: Telemetry domain model verified\n";

        return true;
    }
}