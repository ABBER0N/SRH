#pragma once

#include "Game/Domain/TelemetryState.h"

#include <cstdint>

namespace srh::engine::game
{
    //
    // Rarely-changing normalized telemetry data.
    //
    // Typical ACC source:
    //     acpmf_static
    //
    // Typical iRacing source:
    //     SessionInfo + stable provider metadata
    //

    struct TelemetryStaticState
    {
        std::uint64_t generation{ 0 };

        TelemetryFrameMeta frame;

        GameState game;

        TrackState track;

        DriverIdentity playerDriver;
        VehicleIdentity playerVehicle;

        TelemetryCapabilities capabilities;
    };
}