#include "TelemetryServiceSmokeTest.h"

#include "Game/Telemetry/TelemetryService.h"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    using namespace
        srh::engine::game;

    bool TestEpochLifecycle()
    {
        TelemetryService
            service;

        if (service.IsEpochActive())
        {
            return false;
        }

        if (
            service.CurrentEpoch() !=
            0
            )
        {
            return false;
        }

        const std::uint64_t firstEpoch =
            service.BeginEpoch();

        if (
            firstEpoch ==
            0
            )
        {
            return false;
        }

        if (!service.IsEpochActive())
        {
            return false;
        }

        if (
            service.CurrentEpoch() !=
            firstEpoch
            )
        {
            return false;
        }

        TelemetryStaticState
            staticState;

        staticState.game.providerId =
            "acc";

        if (
            !service.PublishStatic(
                firstEpoch,
                std::move(
                    staticState
                )
            )
            )
        {
            return false;
        }

        const auto published =
            service.GetStaticState();

        if (
            published.generation !=
            1
            )
        {
            return false;
        }

        if (
            published.game.providerId !=
            "acc"
            )
        {
            return false;
        }

        if (
            !service.EndEpoch(
                firstEpoch
            )
            )
        {
            return false;
        }

        if (service.IsEpochActive())
        {
            return false;
        }

        const std::uint64_t
            invalidationEpoch =
            service.CurrentEpoch();

        if (
            invalidationEpoch ==
            firstEpoch
            )
        {
            return false;
        }

        const auto cleared =
            service.GetState();

        if (
            cleared.staticState.generation !=
            0
            )
        {
            return false;
        }

        if (
            !cleared
            .staticState
            .game
            .providerId
            .empty()
            )
        {
            return false;
        }

        //
        // The old provider must no longer be able
        // to publish after its epoch ends.
        //

        TelemetryRealtimeState
            staleRealtime;

        staleRealtime
            .powertrain
            .engineRpm
            .SetValue(
                7000.0
            );

        if (
            service.PublishRealtime(
                firstEpoch,
                std::move(
                    staleRealtime
                )
            )
            )
        {
            return false;
        }

        //
        // Ending an already ended epoch must not
        // mutate the service again.
        //

        if (
            service.EndEpoch(
                firstEpoch
            )
            )
        {
            return false;
        }

        const std::uint64_t secondEpoch =
            service.BeginEpoch();

        if (
            secondEpoch ==
            0 ||
            secondEpoch ==
            firstEpoch ||
            secondEpoch ==
            invalidationEpoch
            )
        {
            return false;
        }

        return
            service.IsEpochActive();
    }

    bool TestIndependentGenerations()
    {
        TelemetryService
            service;

        const std::uint64_t epoch =
            service.BeginEpoch();

        TelemetryStaticState
            staticState;

        staticState.generation =
            999;

        staticState.game.providerId =
            "acc";

        if (
            !service.PublishStatic(
                epoch,
                staticState
            )
            )
        {
            return false;
        }

        //
        // Caller-supplied generation is ignored.
        //

        if (
            service
            .GetStaticState()
            .generation !=
            1
            )
        {
            return false;
        }

        TelemetrySessionState
            sessionState;

        sessionState
            .session
            .type =
            SessionType::Race;

        if (
            !service.PublishSession(
                epoch,
                sessionState
            )
            )
        {
            return false;
        }

        for (
            std::uint32_t update = 0;
            update < 10;
            ++update
            )
        {
            TelemetryRealtimeState
                realtimeState;

            realtimeState
                .powertrain
                .engineRpm
                .SetValue(
                    5000.0 +
                    static_cast<double>(
                        update
                        )
                );

            if (
                !service.PublishRealtime(
                    epoch,
                    std::move(
                        realtimeState
                    )
                )
                )
            {
                return false;
            }
        }

        TelemetryNativeState
            nativeState;

        if (
            !service.PublishNative(
                epoch,
                nativeState
            )
            )
        {
            return false;
        }

        const auto state =
            service.GetState();

        if (
            state.staticState.generation !=
            1
            )
        {
            return false;
        }

        if (
            state.sessionState.generation !=
            1
            )
        {
            return false;
        }

        if (
            state.realtimeState.generation !=
            10
            )
        {
            return false;
        }

        if (
            state.nativeState.generation !=
            1
            )
        {
            return false;
        }

        return true;
    }

    bool TestCoherentSnapshot()
    {
        TelemetryService
            service;

        const std::uint64_t epoch =
            service.BeginEpoch();

        //
        // Static.
        //

        TelemetryStaticState
            staticState;

        staticState.frame.frameId =
            10;

        staticState.game.providerId =
            "acc";

        staticState.game.gameId =
            "assetto-corsa-competizione";

        staticState.game.gameName =
            "Assetto Corsa Competizione";

        staticState.game.connected =
            true;

        staticState.track.name =
            "Monza";

        staticState.playerDriver.firstName =
            "SRH";

        staticState.playerDriver.lastName =
            "Driver";

        staticState.playerVehicle.model =
            "GT3 Test Car";

        staticState
            .capabilities
            .fields
            .push_back(
                TelemetryCapabilityEntry{
                    "vehicle.powertrain.engineRpm",
                    true,
                    "ACC.Physics.rpms"
                }
            );

        if (
            !service.PublishStatic(
                epoch,
                std::move(
                    staticState
                )
            )
            )
        {
            return false;
        }

        //
        // Session.
        //

        TelemetrySessionState
            sessionState;

        sessionState.frame.frameId =
            20;

        sessionState.session.type =
            SessionType::Race;

        sessionState
            .session
            .playerOverallPosition
            .SetValue(
                7
            );

        sessionState
            .environment
            .ambientTemperatureC
            .SetValue(
                24.0
            );

        sessionState
            .playerTiming
            .bestLapTimeUs
            .SetValue(
                90'500'000
            );

        ParticipantState
            participant;

        participant.participantId =
            1001;

        participant.positionSource =
            ParticipantPositionSource::
            WorldCoordinates;

        participant
            .overallPosition
            .SetValue(
                1
            );

        participant
            .worldPositionMeters
            .SetValue(
                Vec3{
                    100.0,
                    200.0,
                    5.0
                }
            );

        sessionState
            .participants
            .push_back(
                std::move(
                    participant
                )
            );

        if (
            !service.PublishSession(
                epoch,
                std::move(
                    sessionState
                )
            )
            )
        {
            return false;
        }

        //
        // Realtime.
        //

        TelemetryRealtimeState
            realtimeState;

        realtimeState.frame.frameId =
            30;

        realtimeState
            .powertrain
            .engineRpm
            .SetValue(
                7123.0
            );

        realtimeState
            .motion
            .speedMps
            .SetValue(
                62.5
            );

        realtimeState
            .controls
            .throttleInput01
            .SetValue(
                0.84
            );

        realtimeState
            .energy
            .fuelLiters
            .SetValue(
                48.2
            );

        if (
            !service.PublishRealtime(
                epoch,
                std::move(
                    realtimeState
                )
            )
            )
        {
            return false;
        }

        //
        // Native.
        //

        TelemetryNativeState
            nativeState;

        nativeState.frame.frameId =
            40;

        NativeChannel
            nativeRpm;

        nativeRpm.metadata.name =
            "ACC.Physics.rpms";

        nativeRpm.metadata.type =
            NativeChannelType::Int32;

        nativeRpm.metadata.unit =
            "rpm";

        nativeRpm.valid =
            true;

        nativeRpm.value =
            std::int32_t{
                7123
        };

        nativeState
            .channels
            .channels
            .push_back(
                std::move(
                    nativeRpm
                )
            );

        if (
            !service.PublishNative(
                epoch,
                std::move(
                    nativeState
                )
            )
            )
        {
            return false;
        }

        const TelemetrySnapshot snapshot =
            service.GetSnapshot();

        if (!snapshot.meta.active)
        {
            return false;
        }

        if (
            snapshot.meta.epoch !=
            epoch
            )
        {
            return false;
        }

        if (
            snapshot.meta.staticGeneration !=
            1 ||
            snapshot.meta.sessionGeneration !=
            1 ||
            snapshot.meta.realtimeGeneration !=
            1 ||
            snapshot.meta.nativeGeneration !=
            1
            )
        {
            return false;
        }

        if (
            snapshot.meta
            .staticFrame
            .frameId !=
            10
            )
        {
            return false;
        }

        if (
            snapshot.meta
            .sessionFrame
            .frameId !=
            20
            )
        {
            return false;
        }

        if (
            snapshot.meta
            .realtimeFrame
            .frameId !=
            30
            )
        {
            return false;
        }

        if (
            snapshot.meta
            .nativeFrame
            .frameId !=
            40
            )
        {
            return false;
        }

        if (
            snapshot.game.providerId !=
            "acc"
            )
        {
            return false;
        }

        if (
            snapshot.track.name !=
            "Monza"
            )
        {
            return false;
        }

        if (
            snapshot.player.driver.lastName !=
            "Driver"
            )
        {
            return false;
        }

        if (
            snapshot.player.identity.model !=
            "GT3 Test Car"
            )
        {
            return false;
        }

        if (
            snapshot.session.type !=
            SessionType::Race
            )
        {
            return false;
        }

        if (
            snapshot.participants.size() !=
            1
            )
        {
            return false;
        }

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
            7123.0
            )
        {
            return false;
        }

        if (
            !snapshot.player
            .timing
            .bestLapTimeUs
            .HasValue()
            )
        {
            return false;
        }

        if (
            snapshot.player
            .timing
            .bestLapTimeUs
            .value !=
            90'500'000
            )
        {
            return false;
        }

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

        return true;
    }

    bool TestConcurrentPublication()
    {
        TelemetryService
            service;

        const std::uint64_t epoch =
            service.BeginEpoch();

        constexpr std::uint32_t
            ThreadCount =
            4;

        constexpr std::uint32_t
            UpdatesPerThread =
            250;

        std::atomic_bool
            failed{
                false
        };

        std::vector<std::thread>
            workers;

        workers.reserve(
            ThreadCount
        );

        for (
            std::uint32_t threadIndex = 0;
            threadIndex < ThreadCount;
            ++threadIndex
            )
        {
            workers.emplace_back(
                [
                    &service,
                    &failed,
                    epoch,
                    threadIndex
                ]()
                {
                    for (
                        std::uint32_t update = 0;
                        update < UpdatesPerThread;
                        ++update
                        )
                    {
                        TelemetryRealtimeState
                            realtimeState;

                        realtimeState.frame.frameId =
                            (
                                static_cast<std::uint64_t>(
                                    threadIndex
                                    ) *
                                UpdatesPerThread
                                ) +
                            update +
                            1;

                        realtimeState
                            .powertrain
                            .engineRpm
                            .SetValue(
                                4000.0 +
                                static_cast<double>(
                                    update
                                    )
                            );

                        if (
                            !service.PublishRealtime(
                                epoch,
                                std::move(
                                    realtimeState
                                )
                            )
                            )
                        {
                            failed.store(
                                true,
                                std::memory_order_release
                            );

                            return;
                        }
                    }
                }
            );
        }

        for (
            auto& worker :
            workers
            )
        {
            worker.join();
        }

        if (
            failed.load(
                std::memory_order_acquire
            )
            )
        {
            return false;
        }

        const auto realtime =
            service.GetRealtimeState();

        const std::uint64_t expectedGeneration =
            static_cast<std::uint64_t>(
                ThreadCount
                ) *
            UpdatesPerThread;

        if (
            realtime.generation !=
            expectedGeneration
            )
        {
            return false;
        }

        //
        // Other partitions must remain untouched.
        //

        const auto state =
            service.GetState();

        return
            state.staticState.generation ==
            0 &&
            state.sessionState.generation ==
            0 &&
            state.nativeState.generation ==
            0;
    }
}

namespace srh::smoketest
{
    bool RunTelemetryServiceSmokeTest()
    {
        if (!TestEpochLifecycle())
        {
            std::cout
                << "FAIL: Telemetry epoch lifecycle\n";

            return false;
        }

        if (!TestIndependentGenerations())
        {
            std::cout
                << "FAIL: Telemetry independent generations\n";

            return false;
        }

        if (!TestCoherentSnapshot())
        {
            std::cout
                << "FAIL: Telemetry coherent snapshot\n";

            return false;
        }

        if (!TestConcurrentPublication())
        {
            std::cout
                << "FAIL: Telemetry concurrent publication\n";

            return false;
        }

        std::cout
            << "INFO: Telemetry service lifecycle verified\n";

        std::cout
            << "INFO: Telemetry service concurrency verified\n";

        return true;
    }
}