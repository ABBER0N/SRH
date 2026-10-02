#pragma once

#include "Game/Domain/TelemetryState.h"

#include <cstdint>
#include <vector>

namespace srh::engine::game
{
    //
    // High-frequency normalized telemetry.
    //
    // Typical ACC source:
    //     acpmf_physics
    //
    // This state is expected to update much more
    // frequently than StaticState or SessionState.
    //

    struct TelemetryRealtimeState
    {
        std::uint64_t generation{ 0 };

        TelemetryFrameMeta frame;

        ControlsState controls;
        MotionState motion;

        ForceFeedbackState forceFeedback;

        PowertrainState powertrain;
        TransmissionState transmission;
        DifferentialState differential;

        std::vector<
            WheelState
        > wheels;

        BrakeSystemState brakes;
        AeroState aero;

        ElectronicsState electronics;

        std::vector<
            DriverControl
        > driverControls;

        InstrumentationState instrumentation;

        LightingWiperState lightingWipers;

        EnergyState energy;
        DamageState damage;

        ProximityState proximity;

        NetworkPerformanceState performance;
    };
}