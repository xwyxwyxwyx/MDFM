#include <fltKernel.h>
#include <dontuse.h>
#include "File.h"


VOID 
MdfmFileCacheClear(
    IN PFILE_OBJECT pFileObject
)

//清除缓存
{
    PFSRTL_COMMON_FCB_HEADER pFcb;
    LARGE_INTEGER liInterval;
    BOOLEAN bNeedReleaseResource = FALSE;
    BOOLEAN bNeedReleasePagingIoResource = FALSE;
    KIRQL irql;

    pFcb = (PFSRTL_COMMON_FCB_HEADER)pFileObject->FsContext;
    if (pFcb == NULL)
        return;

    irql = KeGetCurrentIrql();
    if (irql >= DISPATCH_LEVEL)
    {
        return;
    }

    liInterval.QuadPart = -1 * (LONGLONG)50;

    while (TRUE)
    {
        BOOLEAN bBreak = TRUE;
        BOOLEAN bLockedResource = FALSE;
        BOOLEAN bLockedPagingIoResource = FALSE;
        bNeedReleaseResource = FALSE;
        bNeedReleasePagingIoResource = FALSE;

        // 到fcb中去拿锁。
        if (pFcb->PagingIoResource)
            bLockedPagingIoResource = ExIsResourceAcquiredExclusiveLite(pFcb->PagingIoResource);

        // 总之一定要拿到这个锁。
        if (pFcb->Resource)
        {
            bLockedResource = TRUE;
            if (ExIsResourceAcquiredExclusiveLite(pFcb->Resource) == FALSE)
            {
                bNeedReleaseResource = TRUE;
                if (bLockedPagingIoResource)
                {
                    if (ExAcquireResourceExclusiveLite(pFcb->Resource, FALSE) == FALSE)
                    {
                        bBreak = FALSE;
                        bNeedReleaseResource = FALSE;
                        bLockedResource = FALSE;
                    }
                }
                else
                    ExAcquireResourceExclusiveLite(pFcb->Resource, TRUE);
            }
        }

        if (bLockedPagingIoResource == FALSE)
        {
            if (pFcb->PagingIoResource)
            {
                bLockedPagingIoResource = TRUE;
                bNeedReleasePagingIoResource = TRUE;
                if (bLockedResource)
                {
                    if (ExAcquireResourceExclusiveLite(pFcb->PagingIoResource, FALSE) == FALSE)
                    {
                        bBreak = FALSE;
                        bLockedPagingIoResource = FALSE;
                        bNeedReleasePagingIoResource = FALSE;
                    }
                }
                else
                {
                    ExAcquireResourceExclusiveLite(pFcb->PagingIoResource, TRUE);
                }
            }
        }

        if (bBreak)
        {
            break;
        }

        if (bNeedReleasePagingIoResource)
        {
            ExReleaseResourceLite(pFcb->PagingIoResource);
        }
        if (bNeedReleaseResource)
        {
            ExReleaseResourceLite(pFcb->Resource);
        }

        if (irql == PASSIVE_LEVEL)
        {
            KeDelayExecutionThread(KernelMode, FALSE, &liInterval);
        }
        else
        {
            KEVENT waitEvent;
            KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
            KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, &liInterval);
        }
    }

    if (pFileObject->SectionObjectPointer)
    {
        IO_STATUS_BLOCK ioStatus;
        CcFlushCache(pFileObject->SectionObjectPointer, NULL, 0, &ioStatus);
        if (pFileObject->SectionObjectPointer->ImageSectionObject)
        {
            MmFlushImageSection(pFileObject->SectionObjectPointer, MmFlushForWrite); // MmFlushForDelete
        }
        CcPurgeCacheSection(pFileObject->SectionObjectPointer, NULL, 0, FALSE);
    }

    if (bNeedReleasePagingIoResource)
    {
        ExReleaseResourceLite(pFcb->PagingIoResource);
    }
    if (bNeedReleaseResource)
    {
        ExReleaseResourceLite(pFcb->Resource);
    }
}

ULONG 
MdfmGetFileSize(
    IN PFLT_INSTANCE Instance,
    IN PFILE_OBJECT FileObject
)
// 非重入获取文件大小（向下查询的文件大小会包括文件标识的大小）
{

    FILE_STANDARD_INFORMATION StandardInfo = { 0 };
    ULONG LengthReturned;

    FltQueryInformationFile(Instance, FileObject, &StandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, &LengthReturned);

    return (ULONG)StandardInfo.EndOfFile.QuadPart;
}


NTSTATUS
MdfmFindOrCreateStreamContext(
    IN PFLT_CALLBACK_DATA Data,
    IN PFLT_RELATED_OBJECTS FltObjects,
    IN BOOLEAN CreateIfNotFound,
    IN OUT PSTREAM_CONTEXT* StreamContext,
    IN OUT BOOLEAN* ContextCreated
)
{
    NTSTATUS status;
    PSTREAM_CONTEXT streamContext;
    PSTREAM_CONTEXT oldStreamContext;

    PAGED_CODE();

    *StreamContext = NULL;
    if (ContextCreated != NULL) *ContextCreated = FALSE;

    //  第一次获取文件流上下文
    status = FltGetStreamContext(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, &streamContext);
    if (!NT_SUCCESS(status) &&
        (status == STATUS_NOT_FOUND) &&
        CreateIfNotFound)
    {
        status = MdfmCreateStreamContext(FltObjects, &streamContext);
        if (!NT_SUCCESS(status))
            return status;

        status = FltSetStreamContext(Data->Iopb->TargetInstance,
            Data->Iopb->TargetFileObject,
            FLT_SET_CONTEXT_KEEP_IF_EXISTS,
            streamContext,
            &oldStreamContext);

        if (!NT_SUCCESS(status))
        {
            FltReleaseContext(streamContext);

            if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
            {
                //  FltSetStreamContext failed for a reason other than the context already
                //  existing on the stream. So the object now does not have any context set
                //  on it. So we return failure to the caller.
                return status;
            }
            streamContext = oldStreamContext;
            status = STATUS_SUCCESS;
        }
        else
        {
            if (ContextCreated != NULL) *ContextCreated = TRUE;
        }
    }
    *StreamContext = streamContext;

    return status;
}

NTSTATUS
MdfmCreateStreamContext(
    IN PFLT_RELATED_OBJECTS FltObjects,
    IN OUT PSTREAM_CONTEXT* StreamContext
)
{

    NTSTATUS status;
    PSTREAM_CONTEXT streamContext;

    PAGED_CODE();


    status = FltAllocateContext(FltObjects->Filter,
        FLT_STREAM_CONTEXT,
        sizeof(STREAM_CONTEXT),
        NonPagedPool,
        &streamContext);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //  初始化新的上下文
    RtlZeroMemory(streamContext, sizeof(STREAM_CONTEXT));

    streamContext->Resource = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), RESOURCE_TAG);
    if (streamContext->Resource == NULL)
    {
        FltReleaseContext(streamContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite(streamContext->Resource);

    *StreamContext = streamContext;

    return STATUS_SUCCESS;
}

