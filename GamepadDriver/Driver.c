#include "Driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, GamepadDriverEvtDeviceAdd)
#pragma alloc_text (PAGE, GamepadDriverEvtDriverContextCleanup)
#endif


NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = GamepadDriverEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config, GamepadDriverEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);

    return status;
}

NTSTATUS GamepadDriverEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    return GamepadDriverCreateDevice(DeviceInit);
}

VOID GamepadDriverEvtDriverContextCleanup(_In_ WDFOBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE ();
}
