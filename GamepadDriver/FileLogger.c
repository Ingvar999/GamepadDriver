#include <stdio.h>
#include "FileLogger.h"

typedef struct _LOG_TIME {
	DWORD32 microseconds, miliseconds, seconds, minutes, hours;

} LOG_TIME, *PLOG_TIME;

void InitLogTime(_Out_ PLOG_TIME t) {
	TIME systemTime, localTime;

	KeQuerySystemTimePrecise(&systemTime);
	ExSystemTimeToLocalTime(&systemTime, &localTime);
	t->microseconds = (localTime.LowPart % 10000) / 10;
	t->miliseconds = (localTime.LowPart % 10000000) / 10000;
	t->seconds = (localTime.QuadPart % 600000000) / 10000000;
	t->minutes = (localTime.QuadPart % 36000000000) / 600000000;
	t->hours = (localTime.QuadPart % 864000000000) / 36000000000;
}

NTSTATUS CreateFile(_Out_ PHANDLE file) {
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK statusBlock;
	LARGE_INTEGER size;
	UNICODE_STRING uString;

	RtlInitUnicodeString(&uString, L"\\DosDevices\\C:\\GamepadDriverLog.txt");
	InitializeObjectAttributes(&attributes, &uString, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	return ZwCreateFile(file, GENERIC_WRITE, &attributes, &statusBlock,
			&size, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE | FILE_SHARE_READ,
			FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
}

NTSTATUS WriteLog(_In_ HANDLE file, _In_ LPWCH message, _In_ NTSTATUS status) {
	LOG_TIME t;
	INT size;
	WCHAR buffer[100];
	IO_STATUS_BLOCK statusBlock;

	InitLogTime(&t);
	size = swprintf(buffer, L"[%d:%d:%d:%d:%d] %s %u\r",
		t.hours, t.minutes, t.seconds, t.miliseconds, t.microseconds, message, status);
	return ZwWriteFile(file, NULL, NULL, NULL, &statusBlock, buffer, size * sizeof(WCHAR), NULL, NULL);
}

NTSTATUS CloseFile(_In_ HANDLE file) {
	return ZwClose(file);
}
