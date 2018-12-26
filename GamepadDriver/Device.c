#include "driver.h"
#include "FileLogger.h"
#include "PortReader.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GamepadDriverCreateDevice)
#pragma alloc_text (PAGE, GamepadDriverEvtDevicePrepareHardware)
#endif

NTSTATUS GamepadDriverCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES   attributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = GamepadDriverEvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceD0Exit = GamepadDriverDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
	attributes.ExecutionLevel = WdfExecutionLevelDispatch;

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    if (NT_SUCCESS(status)) {

        deviceContext = DeviceGetContext(device);
		status = CreateFile(&deviceContext->LogFileHandle);
		
		if (NT_SUCCESS(status)) {

			status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_GamepadDriver, NULL);

			if (NT_SUCCESS(status)) {

				status = GamepadDriverQueueInitialize(device);
			}
			if (!NT_SUCCESS(status)) {

				WriteLog(deviceContext->LogFileHandle, L"Close File in add device", status);
				CloseFile(deviceContext->LogFileHandle);
			}
		}
    }

    return status;
}

NTSTATUS GamepadDriverEvtDevicePrepareHardware(
	_In_ WDFDEVICE Device,
	_In_ WDFCMRESLIST ResourceList,
	_In_ WDFCMRESLIST ResourceListTranslated)
{
	NTSTATUS status;
	PDEVICE_CONTEXT deviceContext;
	WDF_USB_DEVICE_CREATE_CONFIG createParams;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
	WDF_USB_PIPE_INFORMATION UsbPipeInfo;
	WDF_OBJECT_ATTRIBUTES attributes;


	UNREFERENCED_PARAMETER(ResourceList);
	UNREFERENCED_PARAMETER(ResourceListTranslated);

	PAGED_CODE();

	status = STATUS_SUCCESS;
	deviceContext = DeviceGetContext(Device);

	if (deviceContext->UsbDevice == NULL) {

		WDF_USB_DEVICE_CREATE_CONFIG_INIT(&createParams, USBD_CLIENT_CONTRACT_VERSION_602);
		status = WdfUsbTargetDeviceCreateWithParameters(Device, &createParams, WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->UsbDevice);
	}
	if (NT_SUCCESS(status)) {

		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
		status = WdfUsbTargetDeviceSelectConfig(deviceContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);

		if (NT_SUCCESS(status)) {

			deviceContext->UsbInterface = configParams.Types.SingleInterface.ConfiguredUsbInterface;

			WDF_USB_PIPE_INFORMATION_INIT(&UsbPipeInfo);
			deviceContext->BulkReadPipe = WdfUsbInterfaceGetConfiguredPipe(deviceContext->UsbInterface, BULK_IN_ENDPOINT_INDEX, &UsbPipeInfo);
			WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(deviceContext->BulkReadPipe);

			WDF_USB_PIPE_INFORMATION_INIT(&UsbPipeInfo);
			deviceContext->BulkWritePipe = WdfUsbInterfaceGetConfiguredPipe(deviceContext->UsbInterface, BULK_OUT_ENDPOINT_INDEX, &UsbPipeInfo);
			WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(deviceContext->BulkWritePipe);

			status = ReaderSart(Device);
		}
	}
	if (!NT_SUCCESS(status)) {

		WriteLog(deviceContext->LogFileHandle, L"Close file in prepare hardware", status);
		CloseFile(deviceContext->LogFileHandle);
	}
	return status;
}

NTSTATUS GamepadDriverDeviceD0Exit(WDFDEVICE  Device, WDF_POWER_DEVICE_STATE  TargetState) {
	NTSTATUS status;
	PDEVICE_CONTEXT deviceContext;

	UNREFERENCED_PARAMETER(TargetState);

	deviceContext = DeviceGetContext(Device);
	WriteLog(deviceContext->LogFileHandle, L"Close file in exit", 0);
	status = CloseFile(deviceContext->LogFileHandle);
	return status;
}