#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>

#include "Public.h"


DRIVER_INITIALIZE DriverEntry;


EVT_WDF_DRIVER_DEVICE_ADD
SrhEvtDeviceAdd;


EVT_WDF_OBJECT_CONTEXT_CLEANUP
SrhEvtVirtualDeviceCleanup;


//
// Each logical SRH DeviceId receives its own
// stable VHF instance ID:
//
// SRH_CONTROLLER_0000
// SRH_CONTROLLER_0001
// ...
// SRH_CONTROLLER_FFFF
//

#define SRH_VIRTUAL_DEVICE_INSTANCE_ID_CHARS 32


//
// State of one dynamically created virtual
// HID controller.
//

typedef struct _SRH_VIRTUAL_DEVICE_ENTRY
{
    USHORT DeviceId;

    VHFHANDLE VhfHandle;

    WCHAR InstanceId[
        SRH_VIRTUAL_DEVICE_INSTANCE_ID_CHARS
    ];

} SRH_VIRTUAL_DEVICE_ENTRY,
* PSRH_VIRTUAL_DEVICE_ENTRY;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    SRH_VIRTUAL_DEVICE_ENTRY,
    SrhGetVirtualDeviceEntry
);


//
// Context of the SRH source/control device.
//
// VirtualDevices contains only virtual HID
// controllers that have actually been created.
//
// There is no compile-time controller count.
//

typedef struct _SRH_DEVICE_CONTEXT
{
    WDFCOLLECTION VirtualDevices;

} SRH_DEVICE_CONTEXT,
* PSRH_DEVICE_CONTEXT;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    SRH_DEVICE_CONTEXT,
    SrhGetDeviceContext
);


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SrhEnsureVirtualDevice(
    _In_ WDFDEVICE Device,
    _In_ USHORT DeviceId
);


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SrhSubmitVirtualDeviceReport(
    _In_ WDFDEVICE Device,
    _In_ USHORT DeviceId,
    _In_ PSRH_INPUT_REPORT_V1 Report
);