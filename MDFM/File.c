#include <fltKernel.h>
#include <dontuse.h>

VOID MdfmFileCacheClear(IN PFILE_OBJECT pFileObject)
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

ULONG MdfmGetFileSize(IN PFLT_INSTANCE Instance, IN PFILE_OBJECT FileObject)
// 非重入获取文件大小（向下查询的文件大小会包括文件标识的大小）
{

    FILE_STANDARD_INFORMATION StandardInfo = { 0 };
    ULONG LengthReturned;

    FltQueryInformationFile(Instance, FileObject, &StandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, &LengthReturned);

    return (ULONG)StandardInfo.EndOfFile.QuadPart;
}

PFLT_INSTANCE MdfmGetVolumeInstance(IN PFLT_FILTER pFilter, IN PUNICODE_STRING pVolumeName)
////获得文件所在盘的实例
{
    NTSTATUS		status;
    PFLT_INSTANCE	pInstance = NULL;
    PFLT_VOLUME		pVolumeList[100];
    ULONG			uRet;
    UNICODE_STRING	uniName = { 0 };
    ULONG 			index = 0;
    WCHAR			wszNameBuffer[260] = { 0 };

    status = FltEnumerateVolumes(pFilter,
        NULL,
        0,
        &uRet);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        return NULL;
    }

    status = FltEnumerateVolumes(pFilter,
        pVolumeList,
        uRet,
        &uRet);

    if (!NT_SUCCESS(status))
    {

        return NULL;
    }
    uniName.Buffer = wszNameBuffer;

    if (uniName.Buffer == NULL)
    {
        for (index = 0; index < uRet; index++)
            FltObjectDereference(pVolumeList[index]);

        return NULL;
    }

    uniName.MaximumLength = sizeof(wszNameBuffer);

    for (index = 0; index < uRet; index++)
    {
        uniName.Length = 0;

        status = FltGetVolumeName(pVolumeList[index],
            &uniName,
            NULL);

        if (!NT_SUCCESS(status))
            continue;

        if (RtlCompareUnicodeString(&uniName,
            pVolumeName,
            TRUE) != 0)
            continue;

        status = FltGetVolumeInstanceFromName(pFilter,
            pVolumeList[index],
            NULL,
            &pInstance);

        if (NT_SUCCESS(status))
        {
            FltObjectDereference(pInstance);
            break;
        }
    }

    for (index = 0; index < uRet; index++)
        FltObjectDereference(pVolumeList[index]);
    return pInstance;
}