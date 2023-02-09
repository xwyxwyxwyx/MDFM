#pragma once
#include "global.h"

VOID MdfmFileCacheClear(IN PFILE_OBJECT pFileObject);

ULONG MdfmGetFileSize(IN PFLT_INSTANCE Instance, IN PFILE_OBJECT FileObject);