#pragma once

#include "Game/Domain/TelemetryCommon.h"
#include "Game/Domain/TelemetryNativeChannel.h"
#include "Game/Domain/TelemetryState.h"

#include <cstdint>
#include <vector>

namespace srh::engine::game
{
    //
    // Metadata describing exactly which internal
    // generations were used to construct this
    // coherent snapshot.
    //
    // A consumer can therefore determine which
    // partition changed without comparing the
    // complete telemetry payload.
    //

    struct TelemetrySnapshotMeta
    {
        bool active{ false };

        std::uint64_t epoch{ 0 };

        std::uint64_t staticGeneration{ 0 };
        std::uint64_t sessionGeneration{ 0 };
        std::uint64_t realtimeGeneration{ 0 };
        std::uint64_t nativeGeneration{ 0 };

        TelemetryFrameMeta staticFrame;
        TelemetryFrameMeta sessionFrame;
        TelemetryFrameMeta realtimeFrame;
        TelemetryFrameMeta nativeFrame;
    };

    //
    // Logical coherent telemetry view exposed to
    // consumers such as:
    //
    // SRH.App
    // Hub output
    // Mapping
    // Diagnostics
    //
    // Internally TelemetryService keeps the four
    // partitions independent. They are combined
    // only when a complete snapshot is requested.
    //

    struct TelemetrySnapshot
    {
        TelemetrySnapshotMeta meta;

        GameState game;

        SessionState session;
        TrackState track;

        EnvironmentState environment;

        std::vector<
            WeatherForecastEntry
        > weatherForecast;

        VehicleState player;

        std::vector<
            ParticipantState
        > participants;

        RaceControlState raceControl;

        ProximityState proximity;

        NetworkPerformanceState
            performance;

        TelemetryCapabilities
            capabilities;

        NativeChannelRegistry
            nativeChannels;
    };
}