#pragma once
#include "global.h"
#include "Context.h"

#define RESOURCE_TAG 'rsTg'

VOID 
MdfmFileCacheClear(
    IN PFILE_OBJECT pFileObject
);

ULONG 
MdfmGetFileSize(
    IN PFLT_INSTANCE Instance, 
    IN PFILE_OBJECT FileObject
);

NTSTATUS
MdfmFindOrCreateStreamContext(
    IN PFLT_CALLBACK_DATA Data,
    IN PFLT_RELATED_OBJECTS FltObjects,
    IN BOOLEAN CreateIfNotFound,
    IN OUT PSTREAM_CONTEXT* StreamContext,
    IN OUT BOOLEAN *ContextCreated
);

NTSTATUS
MdfmCreateStreamContext(
    IN PFLT_RELATED_OBJECTS FltObjects,
    IN OUT PSTREAM_CONTEXT* StreamContext
);


