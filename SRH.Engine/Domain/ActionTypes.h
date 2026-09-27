#pragma once

#include <cstdint>

namespace srh::engine
{
    using VirtualDeviceId =
        std::uint16_t;

    enum class VirtualControllerActionKind :
        std::uint8_t
    {
        Button = 0,
        Axis,
        Pov
    };

    enum class SystemActionKind :
        std::uint8_t
    {
        Unknown = 0,

        //
        // Master audio
        //

        MasterVolumeSet,
        MasterVolumeAdjust,

        MasterMuteSet,
        MasterMuteToggle,

        //
        // Application audio
        //

        ApplicationVolumeSet,
        ApplicationVolumeAdjust,

        ApplicationMuteSet,
        ApplicationMuteToggle,

        //
        // Audio endpoint
        //

        EndpointVolumeSet,
        EndpointVolumeAdjust,

        EndpointMuteSet,
        EndpointMuteToggle,

        //
        // Media
        //

        MediaPlayPause,
        MediaPrevious,
        MediaNext,

        //
        // Keyboard
        //

        KeyboardDown,
        KeyboardUp,
        KeyboardPress,

        //
        // Applications
        //

        LaunchApplication,
        TerminateApplication
    };

    enum class HubActionKind :
        std::uint8_t
    {
        Unknown = 0,

        SetLed,
        SetDisplayPage,
        SetBacklight,
        SetHaptic,

        Custom
    };
}