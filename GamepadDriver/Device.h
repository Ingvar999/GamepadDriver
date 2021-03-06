#pragma once
#include "public.h"

EXTERN_C_START

#define BULK_OUT_ENDPOINT_INDEX 1
#define BULK_IN_ENDPOINT_INDEX 0
#define TIMER_PERIOD 1000
#define READ_BUFFER_SIZE 100

typedef struct _DEVICE_CONTEXT
{
    WDFUSBDEVICE UsbDevice;
	WDFUSBINTERFACE UsbInterface;
	WDFUSBPIPE BulkReadPipe;
	WDFUSBPIPE BulkWritePipe;
	WDFTIMER Timer;
	HANDLE LogFileHandle;
	WDFWORKITEM WorkItem;
	WDFREQUEST wdfRequest;
	PVOID readBuffer;
	WDFMEMORY wdfReadMemory;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

NTSTATUS GamepadDriverCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);
EVT_WDF_DEVICE_PREPARE_HARDWARE GamepadDriverEvtDevicePrepareHardware;
EVT_WDF_DEVICE_D0_EXIT  GamepadDriverDeviceD0Exit;

EXTERN_C_END
