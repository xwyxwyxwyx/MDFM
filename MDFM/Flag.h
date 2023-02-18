#include "global.h"

#define FLAG_TAG 'flag'

#define FILE_GUID_LENGTH 16
#define HASH_SIZE 20
#define KEYSIZE 128

#define FLAG_SIZE sizeof(FILE_FLAG)

// TODO: ���FlagʱҪ����������С
typedef struct _FILE_FLAG {

	/**
	 * ʶ���ļ���ʶ�ı�ʶͷ�����ǿ�����GUID�����������ʶͷ
	 */
	UCHAR    FileFlagHeader[FILE_GUID_LENGTH];

	/**
	 * ��Կ
	 */
	UCHAR    FileKey[KEYSIZE];

	/**
	 * �ļ�����Ч��С
	 */
	LONGLONG FileValidLength;

}FILE_FLAG,*PFILE_FLAG;

NTSTATUS
MdfmWriteFileFlag(
	IN OUT PFLT_CALLBACK_DATA* Data,
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN OUT PFILE_FLAG pFlag
);

NTSTATUS
MdfmReadFileFlag(
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN OUT PFILE_FLAG pFlag
);


VOID
MdfmReadWriteCallbackRoutine(
	IN PFLT_CALLBACK_DATA CallbackData,
	IN PFLT_CONTEXT Context
);

NTSTATUS
MdfmInitNewFlag(
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN OUT PFILE_FLAG pFlag
);