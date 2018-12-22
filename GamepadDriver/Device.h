#pragma once
#include "public.h"

EXTERN_C_START

typedef struct _DEVICE_CONTEXT
{
    WDFUSBDEVICE UsbDevice;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

NTSTATUS GamepadDriverCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);
EVT_WDF_DEVICE_PREPARE_HARDWARE GamepadDriverEvtDevicePrepareHardware;

EXTERN_C_END
