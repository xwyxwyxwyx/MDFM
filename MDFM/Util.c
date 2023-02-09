#include <fltKernel.h>
#include <dontuse.h>

VOID MdfmFileCacheClear(IN PFILE_OBJECT pFileObject)
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

ULONG MdfmGetFileSize(IN PFLT_INSTANCE Instance, IN PFILE_OBJECT FileObject)
{

    FILE_STANDARD_INFORMATION StandardInfo = { 0 };
    ULONG LengthReturned;

    FltQueryInformationFile(Instance, FileObject, &StandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, &LengthReturned);

    return (ULONG)StandardInfo.EndOfFile.QuadPart;
}