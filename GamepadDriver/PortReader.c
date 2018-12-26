#include "Driver.h"
#include "PortReader.h"
#include "FileLogger.h"

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

NTSTATUS ReaderSart(_In_ WDFDEVICE Device) {
	NTSTATUS status;
	PDEVICE_CONTEXT deviceContext;
	WDF_TIMER_CONFIG timerConfig;
	WDF_WORKITEM_CONFIG WIConfig;
	WDF_OBJECT_ATTRIBUTES attributes;

	deviceContext = DeviceGetContext(Device);

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

VOID ReadCompletionRoutine(WDFREQUEST Request, WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS Params, WDFCONTEXT Context)
{
	PDEVICE_CONTEXT deviceContext;
	WDF_REQUEST_REUSE_PARAMS reuseParams;

	deviceContext = (PDEVICE_CONTEXT)Context;
	WriteLogByWorkItem(deviceContext, L"Read completion", KeGetCurrentIrql());

	WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
	WdfRequestReuse(Request, &reuseParams);
}