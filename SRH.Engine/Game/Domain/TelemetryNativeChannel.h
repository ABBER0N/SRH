#pragma once

#include "Game/Domain/TelemetryCommon.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace srh::engine::game
{
    enum class NativeChannelType
    {
        Unknown,

        Boolean,

        Int32,
        UInt32,

        Int64,
        UInt64,

        Float32,
        Float64,

        String,

        BooleanArray,

        Int32Array,
        UInt32Array,

        Int64Array,
        UInt64Array,

        Float32Array,
        Float64Array,

        StringArray
    };

    using NativeChannelValue =
        std::variant <
        std::monostate,

        bool,

        std::int32_t,
        std::uint32_t,

        std::int64_t,
        std::uint64_t,

        float,
        double,

        std::string,

        std::vector<bool>,

        std::vector<std::int32_t>,
        std::vector<std::uint32_t>,

        std::vector<std::int64_t>,
        std::vector<std::uint64_t>,

        std::vector<float>,
        std::vector<double>,

        std::vector<std::string>
        > ;

    struct NativeChannelMetadata
    {
        //
        // Provider-native stable channel name.
        //
        // Examples:
        //
        // ACC.Physics.rpms
        // ACC.Graphics.normalizedCarPosition
        // iRacing.CarIdxLapDistPct
        //

        std::string name;

        NativeChannelType type{
            NativeChannelType::Unknown
        };

        std::uint32_t elementCount{ 1 };

        std::string unit;
        std::string description;
    };

    struct NativeChannel
    {
        NativeChannelMetadata metadata;

        TelemetryTimestampUs
            sourceTimestampUs{ 0 };

        bool valid{ false };

        NativeChannelValue value;
    };

    struct NativeChannelRegistry
    {
        std::vector<
            NativeChannel
        > channels;
    };
}