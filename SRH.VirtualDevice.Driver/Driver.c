#include <initguid.h>

#include "Driver.h"
#include "Queue.h"


//
// SRH Virtual Controller HID Report Descriptor
//
// Report ID 1:
//
// 128 buttons : 16 bytes
// 8 axes      : 16 bytes
// 4 POV hats  : 4 bytes
//
// HID payload : 36 bytes
// Report ID   : 1 byte
//
// Total input report size = 37 bytes.
//

static UCHAR
g_SrhHidReportDescriptor[] =
{
    //
    // Generic Desktop / Game Pad
    //

    0x05, 0x01,             // Usage Page (Generic Desktop)
    0x09, 0x05,             // Usage (Game Pad)
    0xA1, 0x01,             // Collection (Application)

    //
    // Report ID 1
    //

    0x85, SRH_HID_REPORT_ID,

    //
    // 128 buttons
    //

    0x05, 0x09,             // Usage Page (Button)
    0x19, 0x01,             // Usage Minimum (Button 1)
    0x29, 0x80,             // Usage Maximum (Button 128)

    0x15, 0x00,             // Logical Minimum (0)
    0x25, 0x01,             // Logical Maximum (1)

    0x75, 0x01,             // Report Size (1 bit)
    0x95, 0x80,             // Report Count (128)

    0x81, 0x02,             // Input (Data, Variable, Absolute)

    //
    // 8 signed 16-bit axes
    //
    // X
    // Y
    // Z
    // Rx
    // Ry
    // Rz
    // Slider
    // Dial
    //

    0x05, 0x01,             // Usage Page (Generic Desktop)

    0x09, 0x30,             // Usage (X)
    0x09, 0x31,             // Usage (Y)
    0x09, 0x32,             // Usage (Z)
    0x09, 0x33,             // Usage (Rx)
    0x09, 0x34,             // Usage (Ry)
    0x09, 0x35,             // Usage (Rz)
    0x09, 0x36,             // Usage (Slider)
    0x09, 0x37,             // Usage (Dial)

    0x16, 0x00, 0x80,       // Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,       // Logical Maximum (32767)

    0x75, 0x10,             // Report Size (16 bits)
    0x95, 0x08,             // Report Count (8)

    0x81, 0x02,             // Input (Data, Variable, Absolute)

    //
    // 4 POV hats
    //

    0x05, 0x01,             // Usage Page (Generic Desktop)

    0x09, 0x39,             // Usage (Hat switch)
    0x09, 0x39,
    0x09, 0x39,
    0x09, 0x39,

    0x15, 0x00,             // Logical Minimum (0)
    0x25, 0x07,             // Logical Maximum (7)

    0x35, 0x00,             // Physical Minimum (0)
    0x46, 0x3B, 0x01,       // Physical Maximum (315)

    0x65, 0x14,             // Unit (Degrees)

    0x75, 0x08,             // Report Size (8 bits)
    0x95, 0x04,             // Report Count (4)

    0x81, 0x42,             // Input (Data, Variable, Absolute, Null)

    //
    // End Game Pad collection
    //

    0xC0
};


//
// Stable instance-ID format.
//
// IMPORTANT:
//
// sizeof(SRH_INSTANCE_ID_TEMPLATE) includes
// the terminating WCHAR '\0'.
//
// VHF receives InstanceIDLength in bytes.
//

#define SRH_INSTANCE_ID_TEMPLATE \
    L"SRH_CONTROLLER_0000"


static const WCHAR
g_SrhInstanceIdPrefix[] =
L"SRH_CONTROLLER_";


static const WCHAR
g_SrhHexDigits[] =
L"0123456789ABCDEF";


//
// Build:
//
// DeviceId 0:
//     SRH_CONTROLLER_0000
//
// DeviceId 1:
//     SRH_CONTROLLER_0001
//
// DeviceId 0x1234:
//     SRH_CONTROLLER_1234
//
// DeviceId 0xFFFF:
//     SRH_CONTROLLER_FFFF
//

static
VOID
SrhBuildInstanceId(
    _Out_writes_(
        SRH_VIRTUAL_DEVICE_INSTANCE_ID_CHARS
    )
    PWCHAR Buffer,
    _In_ USHORT DeviceId
)
{
    const ULONG prefixLength =
        (ULONG)(
            RTL_NUMBER_OF(
                g_SrhInstanceIdPrefix
            ) - 1
            );

    RtlZeroMemory(
        Buffer,
        sizeof(WCHAR) *
        SRH_VIRTUAL_DEVICE_INSTANCE_ID_CHARS
    );

    RtlCopyMemory(
        Buffer,
        g_SrhInstanceIdPrefix,
        prefixLength *
        sizeof(WCHAR)
    );

    Buffer[prefixLength + 0] =
        g_SrhHexDigits[
            (DeviceId >> 12) &
                0x0F
        ];

    Buffer[prefixLength + 1] =
        g_SrhHexDigits[
            (DeviceId >> 8) &
                0x0F
        ];

    Buffer[prefixLength + 2] =
        g_SrhHexDigits[
            (DeviceId >> 4) &
                0x0F
        ];

    Buffer[prefixLength + 3] =
        g_SrhHexDigits[
            DeviceId &
                0x0F
        ];

    Buffer[prefixLength + 4] =
        L'\0';
}


//
// Find an existing dynamic virtual controller.
//
// Access currently occurs through the sequential
// default IOCTL queue, therefore creation and
// report submission are serialized.
//

static
WDFOBJECT
SrhFindVirtualDeviceObject(
    _In_ PSRH_DEVICE_CONTEXT DeviceContext,
    _In_ USHORT DeviceId
)
{
    ULONG count;
    ULONG index;

    if (
        DeviceContext == NULL ||
        DeviceContext->VirtualDevices == NULL
        )
    {
        return NULL;
    }

    count =
        WdfCollectionGetCount(
            DeviceContext->VirtualDevices
        );

    for (
        index = 0;
        index < count;
        ++index
        )
    {
        WDFOBJECT object;

        PSRH_VIRTUAL_DEVICE_ENTRY
            entry;

        object =
            WdfCollectionGetItem(
                DeviceContext->VirtualDevices,
                index
            );

        entry =
            SrhGetVirtualDeviceEntry(
                object
            );

        if (
            entry != NULL &&
            entry->DeviceId == DeviceId
            )
        {
            return object;
        }
    }

    return NULL;
}


//
// Submit deterministic neutral state immediately
// after creation.
//
// Buttons:
//     all released
//
// Axes:
//     0 = center in our signed HID range
//
// POV:
//     8 = HID Null / centered
//

static
VOID
SrhSubmitInitialNeutralReport(
    _In_ VHFHANDLE VhfHandle
)
{
    SRH_INPUT_REPORT_V1 report;

    HID_XFER_PACKET transferPacket;

    NTSTATUS status;

    if (
        VhfHandle == NULL
        )
    {
        return;
    }

    RtlZeroMemory(
        &report,
        sizeof(report)
    );

    report.ReportId =
        SRH_HID_REPORT_ID;

    report.Pov[0] = 8;
    report.Pov[1] = 8;
    report.Pov[2] = 8;
    report.Pov[3] = 8;

    RtlZeroMemory(
        &transferPacket,
        sizeof(transferPacket)
    );

    transferPacket.reportBuffer =
        (PUCHAR)&report;

    transferPacket.reportBufferLen =
        sizeof(report);

    transferPacket.reportId =
        report.ReportId;

    status =
        VhfReadReportSubmit(
            VhfHandle,
            &transferPacket
        );

    UNREFERENCED_PARAMETER(
        status
    );
}


//
// Cleanup of one dynamically created controller.
//
// Every SRH_VIRTUAL_DEVICE_ENTRY owns exactly
// one VHFHANDLE.
//

VOID
SrhEvtVirtualDeviceCleanup(
    _In_ WDFOBJECT Object
)
{
    PSRH_VIRTUAL_DEVICE_ENTRY
        entry;

    PAGED_CODE();

    entry =
        SrhGetVirtualDeviceEntry(
            Object
        );

    if (
        entry == NULL ||
        entry->VhfHandle == NULL
        )
    {
        return;
    }

    VhfDelete(
        entry->VhfHandle,
        TRUE
    );

    entry->VhfHandle =
        NULL;
}


//
// Ensure one virtual HID controller exists.
//
// Existing DeviceId:
//     return STATUS_SUCCESS
//
// New DeviceId:
//     create WDF entry
//     build unique InstanceID
//     VhfCreate
//     VhfStart
//     add to dynamic collection
//     send neutral report
//

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SrhEnsureVirtualDevice(
    _In_ WDFDEVICE Device,
    _In_ USHORT DeviceId
)
{
    PSRH_DEVICE_CONTEXT
        deviceContext;

    WDF_OBJECT_ATTRIBUTES
        objectAttributes;

    WDFOBJECT
        virtualDeviceObject =
        NULL;

    PSRH_VIRTUAL_DEVICE_ENTRY
        entry;

    VHF_CONFIG
        vhfConfig;

    NTSTATUS status;

    PAGED_CODE();

    deviceContext =
        SrhGetDeviceContext(
            Device
        );

    if (
        deviceContext == NULL ||
        deviceContext->VirtualDevices == NULL
        )
    {
        return STATUS_DEVICE_NOT_READY;
    }

    //
    // Idempotent Ensure().
    //

    if (
        SrhFindVirtualDeviceObject(
            deviceContext,
            DeviceId
        ) != NULL
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // Create dynamically managed WDF object
    // representing one logical controller.
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &objectAttributes,
        SRH_VIRTUAL_DEVICE_ENTRY
    );

    objectAttributes.ParentObject =
        Device;

    objectAttributes.EvtCleanupCallback =
        SrhEvtVirtualDeviceCleanup;

    objectAttributes.ExecutionLevel =
        WdfExecutionLevelPassive;

    status =
        WdfObjectCreate(
            &objectAttributes,
            &virtualDeviceObject
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        return status;
    }

    entry =
        SrhGetVirtualDeviceEntry(
            virtualDeviceObject
        );

    RtlZeroMemory(
        entry,
        sizeof(*entry)
    );

    entry->DeviceId =
        DeviceId;

    entry->VhfHandle =
        NULL;

    SrhBuildInstanceId(
        entry->InstanceId,
        DeviceId
    );

    //
    // Configure one completely independent
    // virtual HID Game Pad.
    //

    VHF_CONFIG_INIT(
        &vhfConfig,
        WdfDeviceWdmGetDeviceObject(
            Device
        ),
        (USHORT)sizeof(
            g_SrhHidReportDescriptor
            ),
        g_SrhHidReportDescriptor
    );

    vhfConfig.VendorID =
        SRH_HID_VENDOR_ID;

    vhfConfig.ProductID =
        SRH_HID_PRODUCT_ID;

    vhfConfig.VersionNumber =
        SRH_HID_VERSION_NUMBER;

    //
    // CRITICAL:
    //
    // InstanceIDLength is expressed in BYTES.
    //
    // Pass the complete backing string buffer,
    // INCLUDING its terminating WCHAR '\0'.
    //
    // Previous version subtracted sizeof(WCHAR),
    // which caused:
    //
    // Device 0 -> SRH_CONTROLLER_000
    // Device 1 -> SRH_CONTROLLER_000
    //
    // and Windows detected duplicate PDOs.
    //

    vhfConfig.InstanceID =
        entry->InstanceId;

    vhfConfig.InstanceIDLength =
        (USHORT)sizeof(
            SRH_INSTANCE_ID_TEMPLATE
            );

    status =
        VhfCreate(
            &vhfConfig,
            &entry->VhfHandle
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        entry->VhfHandle =
            NULL;

        WdfObjectDelete(
            virtualDeviceObject
        );

        return status;
    }

    status =
        VhfStart(
            entry->VhfHandle
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        //
        // WDF object cleanup owns VhfDelete().
        //

        WdfObjectDelete(
            virtualDeviceObject
        );

        return status;
    }

    //
    // Only successfully started VHF devices
    // enter the dynamic registry.
    //

    status =
        WdfCollectionAdd(
            deviceContext->VirtualDevices,
            virtualDeviceObject
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        WdfObjectDelete(
            virtualDeviceObject
        );

        return status;
    }

    //
    // Give Windows a known initial state.
    //

    SrhSubmitInitialNeutralReport(
        entry->VhfHandle
    );

    return STATUS_SUCCESS;
}


//
// Submit one input report to one logical
// virtual controller.
//
// A previously unseen DeviceId is created
// automatically.
//

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SrhSubmitVirtualDeviceReport(
    _In_ WDFDEVICE Device,
    _In_ USHORT DeviceId,
    _In_ PSRH_INPUT_REPORT_V1 Report
)
{
    PSRH_DEVICE_CONTEXT
        deviceContext;

    WDFOBJECT
        virtualDeviceObject;

    PSRH_VIRTUAL_DEVICE_ENTRY
        entry;

    HID_XFER_PACKET
        transferPacket;

    NTSTATUS status;

    PAGED_CODE();

    if (
        Report == NULL
        )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Lazy dynamic creation.
    //

    status =
        SrhEnsureVirtualDevice(
            Device,
            DeviceId
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        return status;
    }

    deviceContext =
        SrhGetDeviceContext(
            Device
        );

    if (
        deviceContext == NULL ||
        deviceContext->VirtualDevices == NULL
        )
    {
        return STATUS_DEVICE_NOT_READY;
    }

    virtualDeviceObject =
        SrhFindVirtualDeviceObject(
            deviceContext,
            DeviceId
        );

    if (
        virtualDeviceObject == NULL
        )
    {
        return STATUS_DEVICE_NOT_READY;
    }

    entry =
        SrhGetVirtualDeviceEntry(
            virtualDeviceObject
        );

    if (
        entry == NULL ||
        entry->VhfHandle == NULL
        )
    {
        return STATUS_DEVICE_NOT_READY;
    }

    RtlZeroMemory(
        &transferPacket,
        sizeof(transferPacket)
    );

    transferPacket.reportBuffer =
        (PUCHAR)Report;

    transferPacket.reportBufferLen =
        sizeof(
            SRH_INPUT_REPORT_V1
            );

    transferPacket.reportId =
        Report->ReportId;

    return
        VhfReadReportSubmit(
            entry->VhfHandle,
            &transferPacket
        );
}


//
// Standard KMDF driver entry point.
//

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;

    WDF_DRIVER_CONFIG_INIT(
        &config,
        SrhEvtDeviceAdd
    );

    return
        WdfDriverCreate(
            DriverObject,
            RegistryPath,
            WDF_NO_OBJECT_ATTRIBUTES,
            &config,
            WDF_NO_HANDLE
        );
}


//
// Create SRH source/control device.
//
// IMPORTANT:
//
// We intentionally create ZERO VHF children
// here.
//
// A virtual HID controller appears only when
// the application first submits a report for
// its DeviceId.
//

NTSTATUS
SrhEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;

    WDFDEVICE device;

    WDF_OBJECT_ATTRIBUTES
        deviceAttributes;

    WDF_OBJECT_ATTRIBUTES
        collectionAttributes;

    PSRH_DEVICE_CONTEXT
        deviceContext;

    UNREFERENCED_PARAMETER(
        Driver
    );

    WdfDeviceInitSetDeviceType(
        DeviceInit,
        FILE_DEVICE_UNKNOWN
    );

    WdfDeviceInitSetExclusive(
        DeviceInit,
        FALSE
    );

    //
    // Dynamic VHF creation and synchronous
    // VHF deletion require passive execution.
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &deviceAttributes,
        SRH_DEVICE_CONTEXT
    );

    deviceAttributes.ExecutionLevel =
        WdfExecutionLevelPassive;

    status =
        WdfDeviceCreate(
            &DeviceInit,
            &deviceAttributes,
            &device
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        return status;
    }

    deviceContext =
        SrhGetDeviceContext(
            device
        );

    deviceContext->VirtualDevices =
        NULL;

    //
    // Dynamic registry of existing virtual
    // controllers.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(
        &collectionAttributes
    );

    collectionAttributes.ParentObject =
        device;

    status =
        WdfCollectionCreate(
            &collectionAttributes,
            &deviceContext->VirtualDevices
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        return status;
    }

    //
    // User-mode control interface.
    //

    status =
        WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_SRH_VIRTUAL_DEVICE,
            NULL
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        return status;
    }

    //
    // IOCTL queue.
    //

    status =
        SrhQueueInitialize(
            device
        );

    if (
        !NT_SUCCESS(status)
        )
    {
        return status;
    }

    //
    // No VhfCreate() here.
    //
    // First report for a DeviceId creates
    // the corresponding controller.
    //

    return STATUS_SUCCESS;
}