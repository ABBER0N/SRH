#pragma once

#include "Game/Domain/TelemetryNativeChannel.h"
#include "Game/Domain/TelemetryState.h"

#include <cstdint>

namespace srh::engine::game
{
    //
    // Provider-native data is intentionally kept
    // orthogonal to the normalized SRH model.
    //
    // Examples:
    //
    // ACC.Physics.rpms
    // ACC.Graphics.normalizedCarPosition
    // iRacing.CarIdxLapDistPct
    //
    // A provider may expose a native channel even
    // before SRH has a normalized semantic mapping
    // for it.
    //

    struct TelemetryNativeState
    {
        std::uint64_t generation{ 0 };

        TelemetryFrameMeta frame;

        NativeChannelRegistry channels;
    };
}