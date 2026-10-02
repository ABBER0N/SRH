#pragma once

#include "Game/Domain/TelemetryNativeState.h"
#include "Game/Domain/TelemetryRealtimeState.h"
#include "Game/Domain/TelemetrySessionState.h"
#include "Game/Domain/TelemetryStaticState.h"

#include <cstdint>

namespace srh::engine::game
{
    //
    // Internal telemetry state owned by the future
    // TelemetryService.
    //
    // Each partition has an independent generation
    // counter so a high-rate realtime update does
    // not imply that static/session data changed.
    //
    // epoch identifies the complete provider state
    // lifetime.
    //
    // It will be incremented when the active
    // provider is replaced/reset so consumers can
    // never accidentally combine data belonging
    // to different simulator connections.
    //

    struct TelemetryState
    {
        std::uint64_t epoch{ 0 };

        TelemetryStaticState staticState;
        TelemetrySessionState sessionState;
        TelemetryRealtimeState realtimeState;

        TelemetryNativeState nativeState;
    };
}