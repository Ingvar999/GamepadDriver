#pragma once
#include "Driver.h"

NTSTATUS CreateFile(_Out_ PHANDLE file);
NTSTATUS WriteLog(_In_ HANDLE file, _In_ LPWCH message,_In_ NTSTATUS status);
NTSTATUS CloseFile(_In_ HANDLE file);