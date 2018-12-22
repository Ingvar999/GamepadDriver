#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GamepadDriverQueueInitialize)
#endif

NTSTATUS GamepadDriverQueueInitialize(_In_ WDFDEVICE Device)
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = GamepadDriverEvtIoDeviceControl;
    status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);

    return status;
}

VOID  GamepadDriverEvtIoDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode)
{
	WdfRequestComplete(Request, STATUS_SUCCESS);
}