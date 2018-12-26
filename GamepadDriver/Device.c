#include "driver.h"
#include "FileLogger.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GamepadDriverCreateDevice)
#pragma alloc_text (PAGE, GamepadDriverEvtDevicePrepareHardware)
#endif


VOID WriteLogByWorkItem(PDEVICE_CONTEXT deviceContext, LPWCH msg, NTSTATUS status) {
	PWORK_ITEM_CONTEXT context;
	DWORD32 i = 0;

	context = WorkItemGetContext(deviceContext->WorkItem);
	context->status = status;
	while (context->message[i++] = *(msg++));

	WdfWorkItemEnqueue(deviceContext->WorkItem);
}

VOID WorkItemRoutine(WDFWORKITEM WorkItem) {
	NTSTATUS status;
	PDEVICE_CONTEXT deviceContext;
	PWORK_ITEM_CONTEXT context;

	deviceContext = DeviceGetContext(WdfWorkItemGetParentObject(WorkItem));
	context = WorkItemGetContext(WorkItem);
	WriteLog(deviceContext->LogFileHandle, context->message, context->status);
}

NTSTATUS GamepadDriverCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES   attributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    //PAGED_CODE();

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
	_In_ WDFCMRESLIST ResourceListTranslated
)
{
	NTSTATUS status;
	PDEVICE_CONTEXT deviceContext;
	WDF_USB_DEVICE_CREATE_CONFIG createParams;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
	WDF_USB_PIPE_INFORMATION UsbPipeInfo;
	WDF_TIMER_CONFIG timerConfig;
	WDF_WORKITEM_CONFIG WIConfig;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDFMEMORY wdfMemory;
	PVOID readBuffer;

	UNREFERENCED_PARAMETER(ResourceList);
	UNREFERENCED_PARAMETER(ResourceListTranslated);

	//PAGED_CODE();

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

			if (NT_SUCCESS(status)) {

				WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
				attributes.ParentObject = Device;
				status = WdfRequestCreate(&attributes, NULL, &deviceContext->wdfRequest);
				WriteLog(deviceContext->LogFileHandle, L"Create request", status);

				if (NT_SUCCESS(status)) {

					WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
					attributes.ParentObject = deviceContext->wdfRequest;
					status = WdfMemoryCreate(&attributes, NonPagedPool, 0, READ_BUFFER_SIZE, &deviceContext->wdfReadMemory, &deviceContext->readBuffer);
					WriteLog(deviceContext->LogFileHandle, L"Allocate read buffer", status);

					if (NT_SUCCESS(status)) {

						WDF_WORKITEM_CONFIG_INIT(&WIConfig, WorkItemRoutine);
						WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
						WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WORK_ITEM_CONTEXT);
						attributes.ParentObject = Device;
						status = WdfWorkItemCreate(&WIConfig, &attributes, &deviceContext->WorkItem);
						WriteLog(deviceContext->LogFileHandle, L"Create work item", status);

						if (NT_SUCCESS(status)) {

							WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, TimerCallback, TIMER_PERIOD);
							timerConfig.AutomaticSerialization = TRUE;
							WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
							attributes.ParentObject = Device;
							status = WdfTimerCreate(&timerConfig, &attributes, &deviceContext->Timer);
							WriteLog(deviceContext->LogFileHandle, L"Create Timer", status);

							if (NT_SUCCESS(status)) {

								WdfTimerStart(deviceContext->Timer, WDF_REL_TIMEOUT_IN_MS(5));
								WriteLog(deviceContext->LogFileHandle, L"Start timer", status);
							}
						}
					}

				}
			}
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

VOID TimerCallback(_In_ WDFTIMER Timer)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT deviceContext;
	WDF_REQUEST_SEND_OPTIONS sendOptions;
	WDFREQUEST request;

	deviceContext = DeviceGetContext(WdfTimerGetParentObject(Timer));
	request = deviceContext->wdfRequest;

	//WriteLogByWorkItem(deviceContext, L"Reuse request", status);

	if (NT_SUCCESS(status)) {

		WdfRequestSetCompletionRoutine(request, ReadCompletionRoutine, deviceContext);
		status = WdfUsbTargetPipeFormatRequestForRead(deviceContext->BulkReadPipe, request, deviceContext->wdfReadMemory, NULL);
		//WriteLogByWorkItem(deviceContext, L"Config request", status);

		if (NT_SUCCESS(status)) {

			WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
			WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, WDF_REL_TIMEOUT_IN_MS(1));

			if (!WdfRequestSend(request, WdfUsbTargetPipeGetIoTarget(deviceContext->BulkReadPipe), &sendOptions)) {
				status = WdfRequestGetStatus(request);
			}
			//WriteLogByWorkItem(deviceContext, L"Send request", status);
		}
	}
}

VOID ReadCompletionRoutine(WDFREQUEST Request, WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT Context)
{
	PDEVICE_CONTEXT deviceContext;
	WDF_REQUEST_REUSE_PARAMS reuseParams;

	deviceContext = (PDEVICE_CONTEXT)Context;
	WriteLogByWorkItem(deviceContext, L"Read completion", KeGetCurrentIrql());
	
	WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
	WdfRequestReuse(Request, &reuseParams);
}