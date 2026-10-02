#pragma once

#include "Domain/ActionTypes.h"

#include <string>
#include <vector>

namespace srh::engine
{
    struct VirtualControllerState
    {
        VirtualDeviceId deviceId{ 0 };

        //
        // UTF-8 display name.
        //

        std::string name;

        //
        // Disabled controllers remain configured,
        // but actions addressed to them must not
        // be forwarded to the driver.
        //

        bool enabled{ true };

        //
        // Runtime flag.
        //
        // true means the controller has successfully
        // submitted at least one report during the
        // current Engine lifetime.
        //

        bool active{ false };
    };

    struct VirtualControllerSnapshot
    {
        std::vector<VirtualControllerState>
            controllers;
    };
}