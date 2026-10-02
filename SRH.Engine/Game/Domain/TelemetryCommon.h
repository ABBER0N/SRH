#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace srh::engine::game
{
    //
    // SRH normalized units:
    //
    // distance             meters
    // linear velocity      m/s
    // linear acceleration  m/s^2
    // angles               radians
    // angular velocity     rad/s
    // force                N
    // torque               N*m
    // power                W
    // pressure             Pa
    // temperature          Celsius
    // mass                 kg
    // normalized values    0.0 ... 1.0
    // timestamps           microseconds
    //
    // SRH local vehicle coordinate system:
    //
    // +X = right
    // +Y = forward
    // +Z = up
    //

    using TelemetryTimestampUs =
        std::uint64_t;

    struct Vec2
    {
        double x{ 0.0 };
        double y{ 0.0 };
    };

    struct Vec3
    {
        double x{ 0.0 };
        double y{ 0.0 };
        double z{ 0.0 };
    };

    struct Quaternion
    {
        double x{ 0.0 };
        double y{ 0.0 };
        double z{ 0.0 };
        double w{ 1.0 };
    };

    enum class GameRunState
    {
        Unknown,

        NotRunning,
        Starting,
        Menu,
        Loading,
        Active,
        Paused,
        Replay,
        Finished
    };

    enum class SessionType
    {
        Unknown,

        Practice,
        Qualifying,
        Race,
        Warmup,
        Test,
        TimeTrial,
        Hotlap,
        Drift,
        Drag,
        Rally,
        Rallycross
    };

    enum class SessionPhase
    {
        Unknown,

        Garage,
        Formation,
        Grid,
        Countdown,
        Green,
        FullCourseYellow,
        SafetyCar,
        VirtualSafetyCar,
        RedFlag,
        Checkered,
        Finished
    };

    enum class TrackLocation
    {
        Unknown,

        NotInWorld,
        Track,
        PitLane,
        PitEntry,
        PitExit,
        PitBox,
        Garage,
        OffTrack
    };

    enum class RaceFlag
    {
        Unknown,

        None,
        Green,
        Yellow,
        DoubleYellow,
        Blue,
        White,
        Black,
        BlackWhite,
        BlackOrange,
        Red,
        Checkered
    };

    enum class PenaltyType
    {
        Unknown,

        Warning,
        DriveThrough,
        StopAndGo,
        TimePenalty,
        SlowDown,
        Disqualification,
        TrackLimits,
        PitSpeeding,
        UnsafePitRelease
    };

    enum class ParticipantPositionSource
    {
        Unavailable,

        WorldCoordinates,
        NormalizedTrackPosition,
        EstimatedTrackPosition,
        ProximityOnly
    };

    enum class WheelPosition
    {
        Unknown,

        FrontLeft,
        FrontRight,
        RearLeft,
        RearRight,

        Extra1,
        Extra2,
        Extra3,
        Extra4
    };

    enum class SurfaceType
    {
        Unknown,

        Asphalt,
        Concrete,
        Kerb,
        Grass,
        Gravel,
        Sand,
        Dirt,
        Mud,
        Ice,
        Snow,
        Water
    };

    enum class TransmissionGearState
    {
        Unknown,

        Reverse,
        Neutral,
        Forward
    };

    enum class TelemetryValueState
    {
        Unsupported,

        //
        // Provider supports the value but no valid
        // value is available in this frame/session.
        //

        Unavailable,

        Available
    };

    template <typename T>
    struct TelemetryValue
    {
        TelemetryValueState state{
            TelemetryValueState::Unsupported
        };

        T value{};

        [[nodiscard]]
        bool IsSupported() const
            noexcept
        {
            return
                state !=
                TelemetryValueState::
                Unsupported;
        }

        [[nodiscard]]
        bool HasValue() const
            noexcept
        {
            return
                state ==
                TelemetryValueState::
                Available;
        }

        void SetUnsupported()
            noexcept
        {
            state =
                TelemetryValueState::
                Unsupported;

            value =
                T{};
        }

        void SetUnavailable()
            noexcept
        {
            state =
                TelemetryValueState::
                Unavailable;

            value =
                T{};
        }

        void SetValue(
            const T& newValue
        )
        {
            value =
                newValue;

            state =
                TelemetryValueState::
                Available;
        }

        void SetValue(
            T&& newValue
        )
        {
            value =
                std::move(
                    newValue
                );

            state =
                TelemetryValueState::
                Available;
        }
    };

    struct TelemetryCapabilityEntry
    {
        //
        // Stable normalized SRH field path.
        //
        // Example:
        // vehicle.powertrain.engineRpm
        // participants.worldPosition
        //

        std::string fieldPath;

        bool supported{ false };

        //
        // Provider-native source used to populate
        // the normalized field.
        //
        // Example:
        // ACC.Physics.rpms
        // iRacing.RPM
        //

        std::string sourceName;
    };

    struct TelemetryCapabilities
    {
        std::vector<
            TelemetryCapabilityEntry
        > fields;
    };
}