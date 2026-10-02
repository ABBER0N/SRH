#include "TelemetryDomainSmokeTest.h"

#include "Game/Domain/TelemetrySnapshot.h"

#include <iostream>
#include <string>
#include <utility>

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
        // Zero is a real value and must not mean
        // "unsupported".
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

        std::cout
            << "INFO: Telemetry domain model verified\n";

        return true;
    }
}