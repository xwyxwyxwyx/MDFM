#include <fltKernel.h>
#include <dontuse.h>

BOOLEAN MdfmAesEncrypt(IN OUT PUCHAR Buffer, IN OUT ULONG LengthReturned) {
	if (NULL == Buffer)
	{
		DbgPrint("MdfmAesEncrypt->Buffer is NULL.\n");
		return FALSE;
	}
	for (UCHAR i = 0; i < (UCHAR)LengthReturned; i++) {
		*(Buffer + i) ^= 1;
	}
	return TRUE;
}

NTSTATUS MdfmAesDecrypt(IN OUT PUCHAR Buffer, IN ULONG Length) {
	if (NULL == Buffer)
	{
		DbgPrint("MdfmAesEncrypt->Buffer is NULL.\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (0 == Length)
	{
		DbgPrint("MdfmAesEncrypt->Length is NULL.\n");
		return STATUS_UNSUCCESSFUL;
	}

	for (UCHAR i = 0; i < (UCHAR)Length; i++) {
		*(Buffer + i) ^= 1;
	}

	return STATUS_SUCCESS;
}

