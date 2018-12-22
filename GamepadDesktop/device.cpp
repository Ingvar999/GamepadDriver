#include "pch.h"

#include <cfgmgr32.h>
#include "../GamepadDriver/Public.h"

using namespace std;

HRESULT RetrieveDevicePath(
	_Out_bytecap_(BufLen) LPTSTR DevicePath,
	_In_                  ULONG  BufLen,
	_Out_opt_             PBOOL  FailureDeviceNotFound
);

HRESULT OpenDevice(
	_Out_     PDEVICE_DATA DeviceData,
	_Out_opt_ PBOOL        FailureDeviceNotFound)
{
	HRESULT hr = S_OK;

	DeviceData->HandlesOpen = FALSE;

	hr = RetrieveDevicePath(DeviceData->DevicePath, sizeof(DeviceData->DevicePath), FailureDeviceNotFound);

	if (FAILED(hr)) {
		return hr;
	}

	DeviceData->DeviceHandle = CreateFile(DeviceData->DevicePath,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE == DeviceData->DeviceHandle) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DeviceData->HandlesOpen = TRUE;
	return hr;
}

VOID CloseDevice(_Inout_ PDEVICE_DATA DeviceData)
{
	if (!DeviceData->HandlesOpen) {
		return;
	}

	CloseHandle(DeviceData->DeviceHandle);
	DeviceData->HandlesOpen = FALSE;
}

HRESULT RetrieveDevicePath(
	_Out_bytecap_(BufLen) LPTSTR DeviceInterfaceList,
	_In_ ULONG  BufLen,
	_Out_opt_ PBOOL  FailureDeviceNotFound
)
{
	CONFIGRET cr = CR_SUCCESS;
	HRESULT hr = S_OK;
	ULONG DeviceInterfaceListLength = 0;

	if (NULL != FailureDeviceNotFound) {
		*FailureDeviceNotFound = FALSE;
	}

	cr = CM_Get_Device_Interface_List_Size(&DeviceInterfaceListLength,
		(LPGUID)&GUID_DEVINTERFACE_GamepadDriver,
		NULL,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

	if (cr != CR_SUCCESS) {
		return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
	}

	ZeroMemory(DeviceInterfaceList, BufLen);
	cr = CM_Get_Device_Interface_List((LPGUID)&GUID_DEVINTERFACE_GamepadDriver,
		NULL,
		DeviceInterfaceList,
		DeviceInterfaceListLength,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

	if (cr != CR_SUCCESS) {
		return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
	}

	if (*DeviceInterfaceList == TEXT('\0')) {
		if (NULL != FailureDeviceNotFound) {
			*FailureDeviceNotFound = TRUE;
		}
		return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}

	return hr;
}

