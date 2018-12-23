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
	WDF_TIMER_CONFIG timerConfig;
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

			if (NT_SUCCESS(status)) {

				WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, TimerCallback, TIMER_PERIOD);
				status = WdfTimerCreate(&timerConfig, WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->Timer);
			}
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
	WDF_USB_PIPE_INFORMATION UsbPipeInfo;

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
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
		status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
		if (NT_SUCCESS(status)) {
			pDeviceContext->UsbInterface = configParams.Types.SingleInterface.ConfiguredUsbInterface;

			WDF_USB_PIPE_INFORMATION_INIT(&UsbPipeInfo);
			pDeviceContext->BulkReadPipe = WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface, BULK_IN_ENDPOINT_INDEX, &UsbPipeInfo);
			WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkReadPipe);

			WDF_USB_PIPE_INFORMATION_INIT(&UsbPipeInfo);
			pDeviceContext->BulkWritePipe = WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface, BULK_OUT_ENDPOINT_INDEX, &UsbPipeInfo);
			WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkWritePipe);

			WdfTimerStart(pDeviceContext->Timer, WDF_REL_TIMEOUT_IN_MS(5));
		}
	}
	return status;
}

VOID TimerCallback(_In_ WDFTIMER Timer)
{
	UNREFERENCED_PARAMETER(Timer);
}