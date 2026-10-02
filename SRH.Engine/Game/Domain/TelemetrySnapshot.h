#pragma once

#include "Game/Domain/TelemetryCommon.h"
#include "Game/Domain/TelemetryNativeChannel.h"
#include "Game/Domain/TelemetryState.h"

#include <vector>

namespace srh::engine::game
{
    //
    // This is the logical coherent view exposed by
    // the telemetry layer.
    //
    // v1.8b will define how Static / Session /
    // Realtime generations are stored internally
    // without copying all data at physics rate.
    //

    struct TelemetrySnapshot
    {
        TelemetryFrameMeta frame;

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