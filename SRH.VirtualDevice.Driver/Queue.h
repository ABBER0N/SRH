#pragma once

#include <ntddk.h>
#include <wdf.h>

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL
SrhEvtIoDeviceControl;

NTSTATUS
SrhQueueInitialize(
    _In_ WDFDEVICE Device
);