#include "pch.h"

#include <stdio.h>

LONG __cdecl _tmain(LONG Argc, LPTSTR *Argv)
{
    DEVICE_DATA           deviceData;
    HRESULT               hr;
    USB_DEVICE_DESCRIPTOR deviceDesc;
    BOOL                  bResult;
    BOOL                  noDevice;
    ULONG                 lengthReceived;

    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Argv);

    hr = OpenDevice(&deviceData, &noDevice);

    if (FAILED(hr)) {

        if (noDevice) {

            wprintf(L"Device not connected or driver not installed\n");

        } else {

            wprintf(L"Failed looking for device, HRESULT 0x%x\n", hr);
        }

        return 0;
    }
    CloseDevice(&deviceData);
    return 0;
}
