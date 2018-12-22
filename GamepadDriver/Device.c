#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GamepadDriverCreateDevice)
#pragma alloc_text (PAGE, GamepadDriverEvtDevicePrepareHardware)
#endif


NTSTATUS GamepadDriverCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = GamepadDriverEvtDevicePrepareHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        deviceContext = DeviceGetContext(device);
        status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_GamepadDriver, NULL);

        if (NT_SUCCESS(status)) {
            status = GamepadDriverQueueInitialize(device);
        }
    }

    return status;
}

NTSTATUS GamepadDriverEvtDevicePrepareHardware(
	_In_ WDFDEVICE Device,
	_In_ WDFCMRESLIST ResourceList,
	_In_ WDFCMRESLIST ResourceListTranslated
)
{
	NTSTATUS status;
	PDEVICE_CONTEXT pDeviceContext;
	WDF_USB_DEVICE_CREATE_CONFIG createParams;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;

	UNREFERENCED_PARAMETER(ResourceList);
	UNREFERENCED_PARAMETER(ResourceListTranslated);

	PAGED_CODE();

	status = STATUS_SUCCESS;
	pDeviceContext = DeviceGetContext(Device);
	if (pDeviceContext->UsbDevice == NULL) {

		WDF_USB_DEVICE_CREATE_CONFIG_INIT(&createParams, USBD_CLIENT_CONTRACT_VERSION_602);
		status = WdfUsbTargetDeviceCreateWithParameters(Device, &createParams, WDF_NO_OBJECT_ATTRIBUTES, &pDeviceContext->UsbDevice);
	}
	if (NT_SUCCESS(status)) {
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&configParams, 0, NULL);
		status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
	}
	return status;
}
