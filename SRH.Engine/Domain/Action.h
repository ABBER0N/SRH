#pragma once

#include "Domain/ActionTypes.h"
#include "Domain/DeviceState.h"
#include "Domain/InputTypes.h"

#include <cstdint>
#include <string>
#include <variant>

namespace srh::engine
{
    //
    // Concrete action for a virtual controller.
    //
    // value:
    //
    // Button:
    //     0 = released
    //     non-zero = pressed
    //
    // Axis:
    //     final axis value
    //
    // Pov:
    //     final POV value
    //

    struct VirtualControllerAction
    {
        VirtualControllerActionKind kind{
            VirtualControllerActionKind::Button
        };

        VirtualDeviceId deviceId{ 0 };

        ControlId controlId{ 0 };

        std::int32_t value{ 0 };
    };

    //
    // Concrete action that must be executed
    // by the operating system.
    //
    // targetId:
    //     application/session/endpoint identity
    //     where required.
    //
    // value:
    //     volume or relative adjustment.
    //
    // state:
    //     explicit boolean state such as mute.
    //
    // code:
    //     keyboard key code or other numeric
    //     system parameter.
    //
    // argument:
    //     executable path, command argument,
    //     or other textual parameter.
    //
    // Strings in the Engine domain are UTF-8.
    // Windows-specific conversion belongs to
    // the Windows execution implementation.
    //

    struct SystemAction
    {
        SystemActionKind kind{
            SystemActionKind::Unknown
        };

        std::string targetId;

        float value{ 0.0f };

        bool state{ false };

        std::uint32_t code{ 0 };

        std::string argument;
    };

    //
    // Concrete action that must be sent
    // from PC back to Hub / Node.
    //
    // targetNodeId:
    //     0 may later be reserved for Hub itself.
    //
    // channel:
    //     LED/display/haptic/etc. logical channel.
    //
    // value:
    //     numeric action value.
    //
    // payload:
    //     optional higher-level data.
    //

    struct HubAction
    {
        HubActionKind kind{
            HubActionKind::Unknown
        };

        NodeId targetNodeId{ 0 };

        std::uint32_t channel{ 0 };

        std::int32_t value{ 0 };

        std::string payload;
    };

    //
    // Single action type used by the Engine.
    //

    using Action =
        std::variant<
        VirtualControllerAction,
        SystemAction,
        HubAction
        >;
}