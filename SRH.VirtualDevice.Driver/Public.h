#pragma once

#include <ntddk.h>

//
// SRH virtual HID identity.
//
// IMPORTANT:
// These VID/PID values are development identifiers for the SRH prototype.
// They are not a registered USB Vendor ID assignment.
// Before public distribution we must decide on production identifiers.
//

#define SRH_HID_VENDOR_ID       0x5348
#define SRH_HID_PRODUCT_ID      0x0001
#define SRH_HID_VERSION_NUMBER  0x0100

//
// {8B816A4E-CA28-4C79-9EA7-29D11276A37A}
//

DEFINE_GUID(
    GUID_DEVINTERFACE_SRH_VIRTUAL_DEVICE,
    0x8b816a4e,
    0xca28,
    0x4c79,
    0x9e, 0xa7, 0x29, 0xd1,
    0x12, 0x76, 0xa3, 0x7a
);

#define SRH_PROTOCOL_VERSION 1

#define SRH_HID_REPORT_ID 1

#define IOCTL_SRH_SUBMIT_REPORT \
    CTL_CODE(                    \
        FILE_DEVICE_UNKNOWN,     \
        0x800,                   \
        METHOD_BUFFERED,         \
        FILE_WRITE_DATA          \
    )

#pragma pack(push, 1)

typedef struct _SRH_INPUT_REPORT_V1
{
    UCHAR ReportId;

    //
    // 128 buttons = 16 bytes.
    //

    UCHAR Buttons[16];

    //
    // 8 signed 16-bit axes.
    //

    SHORT Axes[8];

    //
    // Four POV hats.
    //
    // 0..7 = directions
    // 8    = centered
    //

    UCHAR Pov[4];

} SRH_INPUT_REPORT_V1,
* PSRH_INPUT_REPORT_V1;

typedef struct _SRH_SUBMIT_REPORT_REQUEST_V1
{
    ULONG ProtocolVersion;

    USHORT DeviceId;

    USHORT Reserved;

    SRH_INPUT_REPORT_V1 Report;

} SRH_SUBMIT_REPORT_REQUEST_V1,
* PSRH_SUBMIT_REPORT_REQUEST_V1;

#pragma pack(pop)

C_ASSERT(
    sizeof(SRH_INPUT_REPORT_V1) == 37
);

C_ASSERT(
    sizeof(SRH_SUBMIT_REPORT_REQUEST_V1) == 45
);