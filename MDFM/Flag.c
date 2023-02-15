#include "Flag.h"
#include "Context.h"


NTSTATUS
MdfmWriteFileFlag(
	IN OUT PFLT_CALLBACK_DATA* Data,
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN OUT PFILE_FLAG pFlag
) 
{
	NTSTATUS status = STATUS_SUCCESS;

	FILE_STANDARD_INFORMATION StandardInfo = { 0 };
	ULONG Length, LengthReturned;

	PVOLUME_CONTEXT Volume;
	FLT_VOLUME_PROPERTIES VolumeProps;

	PVOID Buffer;

	KEVENT Event;
	LARGE_INTEGER ByteOffset;

	//
	//	��ѯ�ļ���С
	//

	status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &StandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, &LengthReturned);

	if (!NT_SUCCESS(status) || status == STATUS_VOLUME_DISMOUNTED)
	{
		DbgPrint("[MdfmWriteFileFlag]->FltQueryInformationFile failed. Status = %x\n", status);
		return status;
	}

	//
	//	�Ӿ������������ҵ�������С
	//
	status = FltGetVolumeFromInstance(FltObjects->Instance, &Volume);

	if (!NT_SUCCESS(status)) {

		DbgPrint("[MdfmWriteFileFlag]->FltGetVolumeFromInstance failed. Status = %x\n", status);
		return status;
	}

	status = FltGetVolumeProperties(Volume, &VolumeProps, sizeof(VolumeProps), &Length);

	if (NULL != Volume)
	{
		FltObjectDereference(Volume);
		Volume = NULL;
	}
	//
	// д��Flag�Ĵ�С������������С��������
	//

	Length = sizeof(FILE_FLAG);

	Length = ROUND_TO_SIZE(Length, VolumeProps.SectorSize);


	// 
	//	����ռ�
	//

	Buffer = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, Length, FLAG_TAG);

	if (!Buffer) {

		DbgPrint("MdfmWriteFileFlag->ExAllocatePoolWithTag Buffer failed.\n");
		return status;
	}

	RtlZeroMemory(Buffer, Length);

	//
	// �������
	//
	memcpy(Buffer, pFlag, FLAG_SIZE);

	//
	// д��Flag
	//

	KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

	ByteOffset.QuadPart = 0;
	status = FltWriteFile(FltObjects->Instance, FltObjects->FileObject, &ByteOffset, Length, Buffer,
		FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, NULL, MdfmReadWriteCallbackRoutine, &Event);

	KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);

	if (!NT_SUCCESS(status)) {

		DbgPrint("MdfmWriteFileFlag->NULL FltWriteFile failed. Status = %x\n", status);
		if (NULL != Buffer)
		{
			FltFreePoolAlignedWithTag(FltObjects->Instance, Buffer, FLAG_TAG);
			Buffer = NULL;
		}
		return status;
	}

	if (NULL != Buffer)
	{
		FltFreePoolAlignedWithTag(FltObjects->Instance, Buffer, FLAG_TAG);
		Buffer = NULL;
	}

	DbgPrint(" FltWriteFile success.\n");

	return status;
}


NTSTATUS
MdfmReadFileFlag(
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN OUT PFILE_FLAG pFlag
) 
{
	NTSTATUS status = STATUS_SUCCESS;

	FILE_STANDARD_INFORMATION StandardInfo = { 0 };

	PVOLUME_CONTEXT Volume = NULL;
	FLT_VOLUME_PROPERTIES VolumeProps;

	ULONG Length,LengthReturned;

	PVOID ReadBuffer = NULL;

	KEVENT Event;
	LARGE_INTEGER ByteOffset = { 0 };

	try {

		//
		//	��ѯ�ļ���С�����̫С˵��û��Flag
		//

		status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &StandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, &LengthReturned);

		if (!NT_SUCCESS(status) || status == STATUS_VOLUME_DISMOUNTED)
		{
			DbgPrint("[MdfmReadFileFlag]->FltQueryInformationFile failed. Status = %x\n", status);
			pFlag = NULL;
			leave;
		}

		if (StandardInfo.EndOfFile.QuadPart < FLAG_SIZE) {
			leave;
		}

		//
		// �Ӿ������������ҵ�������С
		//

		status = FltGetVolumeFromInstance(FltObjects->Instance, &Volume);

		if (!NT_SUCCESS(status)) {

			DbgPrint("[MdfmReadFileFlag]->FltGetVolumeFromInstance failed. Status = %x\n", status);
			leave;
		}

		status = FltGetVolumeProperties(Volume, &VolumeProps, sizeof(VolumeProps), &Length);

		if (NULL != Volume)
		{
			FltObjectDereference(Volume);
			Volume = NULL;
		}

		//
		// ��ȡFlag�Ĵ�С������������С��������
		//

		Length = sizeof(FILE_FLAG);

		Length = ROUND_TO_SIZE(Length, VolumeProps.SectorSize);


		//
		// �����ڴ�
		//

		ReadBuffer = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, Length, FLAG_TAG);

		if (!ReadBuffer)
		{
			DbgPrint("MdfmReadFileFlag->FltAllocatePoolAlignedWithTag ReadBuffer failed.\n");
			leave;
		}

		RtlZeroMemory(ReadBuffer, Length);

		//
		// ��ȡFlag
		//

		KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

		ByteOffset.QuadPart = 0;
		status = FltReadFile(FltObjects->Instance, FltObjects->FileObject, &ByteOffset, Length, ReadBuffer,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, NULL, MdfmReadWriteCallbackRoutine, &Event);

		KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);

		if (!NT_SUCCESS(status)) {
			DbgPrint("MdfmReadFileFlag->FltReadFile Filed\n");
			leave;
		}

		//
		//	���ݽ���
		//

		if (ReadBuffer != NULL) {
			memcpy(pFlag, ReadBuffer, FLAG_SIZE);
		}
		
		DbgPrint("MdfmReadFileFlag: Read Flag Success\n");

	}
	finally {
		if (NULL != Volume)
		{
			FltObjectDereference(Volume);
			Volume = NULL;
		}

		if (NULL != ReadBuffer)
		{
			FltFreePoolAlignedWithTag(FltObjects->Instance, ReadBuffer, 'itRB');
			ReadBuffer = NULL;
		}
	}

	return status;
}


VOID 
MdfmReadWriteCallbackRoutine(
	IN PFLT_CALLBACK_DATA CallbackData, 
	IN PFLT_CONTEXT Context
)
{
	UNREFERENCED_PARAMETER(CallbackData);
	KeSetEvent((PRKEVENT)Context, IO_NO_INCREMENT, FALSE);
}

