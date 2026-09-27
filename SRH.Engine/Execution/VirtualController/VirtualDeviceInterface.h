#pragma once

#include <guiddef.h>

namespace srh::engine::virtual_controller
{
    //
    // Must exactly match:
    //
    // GUID_DEVINTERFACE_SRH_VIRTUAL_DEVICE
    //
    // in SRH.VirtualDevice.Driver/Public.h
    //
    // {8B816A4E-CA28-4C79-9EA7-29D11276A37A}
    //

    inline constexpr GUID
        DeviceInterfaceGuid
    {
        0x8b816a4e,
        0xca28,
        0x4c79,
        {
            0x9e,
            0xa7,
            0x29,
            0xd1,
            0x12,
            0x76,
            0xa3,
            0x7a
        }
    };
}