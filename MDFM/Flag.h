#include "global.h"

#define FLAG_TAG 'flag'

#define FILE_GUID_LENGTH 16
#define HASH_SIZE 20
#define KEYSIZE 128

#define FLAG_SIZE sizeof(FILE_FLAG)

// TODO: 添加Flag时要对其扇区大小
typedef struct _FILE_FLAG {

	/**
	 * 识别文件标识的标识头，我们可以用GUID来设置这个标识头
	 */
	UCHAR    FileFlagHeader[FILE_GUID_LENGTH];

	/**
	 * 密钥
	 */
	UCHAR    FileKey[KEYSIZE];

	/**
	 * 文件的有效大小
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