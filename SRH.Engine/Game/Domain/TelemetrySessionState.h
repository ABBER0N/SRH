#pragma once

#include "Game/Domain/TelemetryState.h"

#include <cstdint>
#include <vector>

namespace srh::engine::game
{
    //
    // Medium-frequency normalized telemetry.
    //
    // Contains session/race state rather than
    // high-rate vehicle physics.
    //
    // Typical ACC sources:
    //
    //     acpmf_graphics
    //     Broadcasting API
    //

    struct TelemetrySessionState
    {
        std::uint64_t generation{ 0 };

        TelemetryFrameMeta frame;

        SessionState session;

        EnvironmentState environment;

        std::vector<
            WeatherForecastEntry
        > weatherForecast;

        std::vector<
            ParticipantState
        > participants;

        RaceControlState raceControl;

        //
        // Player data that logically follows the
        // race/session state rather than physics
        // rate.
        //

        TimingState playerTiming;
        PitState playerPit;
        StrategyState playerStrategy;
    };
}