#include "Queue.h"

#include "Driver.h"
#include "Public.h"


NTSTATUS
SrhQueueInitialize(
    _In_ WDFDEVICE Device
)
{
    WDF_IO_QUEUE_CONFIG
        queueConfig;

    WDF_OBJECT_ATTRIBUTES
        queueAttributes;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoDeviceControl =
        SrhEvtIoDeviceControl;

    //
    // First report for a previously unseen
    // DeviceId can invoke VhfCreate().
    //
    // Therefore this queue executes at
    // PASSIVE_LEVEL.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(
        &queueAttributes
    );

    queueAttributes.ExecutionLevel =
        WdfExecutionLevelPassive;

    return
        WdfIoQueueCreate(
            Device,
            &queueConfig,
            &queueAttributes,
            WDF_NO_HANDLE
        );
}


VOID
SrhEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status =
        STATUS_INVALID_DEVICE_REQUEST;

    size_t information =
        0;

    UNREFERENCED_PARAMETER(
        OutputBufferLength
    );

    switch (
        IoControlCode
        )
    {
    case IOCTL_SRH_SUBMIT_REPORT:
    {
        WDFDEVICE device;

        PSRH_SUBMIT_REPORT_REQUEST_V1
            request = NULL;

        size_t bufferLength =
            0;

        //
        // Validate ABI request size.
        //

        if (
            InputBufferLength <
            sizeof(
                SRH_SUBMIT_REPORT_REQUEST_V1
                )
            )
        {
            status =
                STATUS_BUFFER_TOO_SMALL;

            break;
        }

        status =
            WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(
                    SRH_SUBMIT_REPORT_REQUEST_V1
                    ),
                (PVOID*)&request,
                &bufferLength
            );

        if (
            !NT_SUCCESS(status)
            )
        {
            break;
        }

        //
        // Validate protocol version.
        //

        if (
            request->ProtocolVersion !=
            SRH_PROTOCOL_VERSION
            )
        {
            status =
                STATUS_REVISION_MISMATCH;

            break;
        }

        //
        // Validate HID report ID.
        //

        if (
            request->Report.ReportId !=
            SRH_HID_REPORT_ID
            )
        {
            status =
                STATUS_INVALID_PARAMETER;

            break;
        }

        //
        // DeviceId is dynamic.
        //
        // There is no DeviceId == 0 restriction.
        //

        device =
            WdfIoQueueGetDevice(
                Queue
            );

        //
        // SrhSubmitVirtualDeviceReport():
        //
        // unseen DeviceId:
        //     create VHF device
        //     start VHF device
        //     submit neutral state
        //     submit requested state
        //
        // existing DeviceId:
        //     submit requested state
        //

        status =
            SrhSubmitVirtualDeviceReport(
                device,
                request->DeviceId,
                &request->Report
            );

        break;
    }

    default:
        status =
            STATUS_INVALID_DEVICE_REQUEST;

        break;
    }

    WdfRequestCompleteWithInformation(
        Request,
        status,
        information
    );
}