#pragma once

typedef struct _DEVICE_DATA {

	BOOL                    HandlesOpen;
	HANDLE                  DeviceHandle;
	TCHAR                   DevicePath[MAX_PATH];

} DEVICE_DATA, *PDEVICE_DATA;

HRESULT OpenDevice(
	_Out_     PDEVICE_DATA DeviceData,
	_Out_opt_ PBOOL        FailureDeviceNotFound
);

VOID CloseDevice(_Inout_ PDEVICE_DATA DeviceData);
