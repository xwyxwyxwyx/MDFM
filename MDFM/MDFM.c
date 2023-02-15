#pragma once
#include "MDFM.h"
#include "Process.h"
#include "Encrypt.h"
#include "SwapBuffer.h"
#include "Context.h"
#include "File.h"
#include "Flag.h"

PFLT_FILTER gFilterHandle;                          // 保存过滤器的句柄
NPAGED_LOOKASIDE_LIST Pre2PostContextList;          // 用于分配上下文的旁视列表
ULONG_PTR OperationStatusCtx = 1;
ULONG gTraceFlags = 0;
UCHAR gFileFlagHeader[FILE_GUID_LENGTH] = {
    0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
    0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77
};



//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MDFMUnload)
#pragma alloc_text(PAGE, MDFMInstanceQueryTeardown)
#pragma alloc_text(PAGE, MDFMInstanceSetup)
#pragma alloc_text(PAGE, MDFMInstanceTeardownStart)
#pragma alloc_text(PAGE, MDFMInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

     { FLT_VOLUME_CONTEXT,
       0,
       ContextCleanUp,
       sizeof(VOLUME_CONTEXT),
       CONTEXT_TAG },

     { FLT_CONTEXT_END }
};

CONST FLT_OPERATION_REGISTRATION Callbacks[5] = {

#if 0 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_CLOSE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_READ,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_WRITE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_SET_EA,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      MDFMPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_QUERY_SECURITY,
        0,
        MDFMPreOperation,
        MDFMPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_PNP,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      MDFMPreOperation,
      MDFMPostOperation },

#endif // TODO
    { IRP_MJ_CREATE,
      0,
      MDFMPreCreate,
      MDFMPostCreate },
    
    { IRP_MJ_READ,
      0,
      MDFMPreRead,
      MDFMPostRead },

    { IRP_MJ_WRITE,
      0,
      MDFMPreWrite,
      MDFMPostWrite },

    { IRP_MJ_CLEANUP,
      0,
      MDFMPreCleanUp,
      MDFMPostCleanUp },

    //{ IRP_MJ_CLOSE,
    //  0,
    //  MDFMPreClose,
    //  MDFMPostClose },

    { IRP_MJ_OPERATION_END }
};


//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextNotifications,               //  Context
    Callbacks,                          //  Operation callbacks

    MDFMUnload,                           //  MiniFilterUnload

    MDFMInstanceSetup,                    //  InstanceSetup
    MDFMInstanceQueryTeardown,            //  InstanceQueryTeardown
    MDFMInstanceTeardownStart,            //  InstanceTeardownStart
    MDFMInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
MDFMInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    PDEVICE_OBJECT devObj = NULL;
    PVOLUME_CONTEXT ctx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG retLen;
    PUNICODE_STRING workingName;
    USHORT size;
    UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512];
    PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    try {

        //
        //  创建一个卷上下文
        //

        status = FltAllocateContext(FltObjects->Filter,
            FLT_VOLUME_CONTEXT,
            sizeof(VOLUME_CONTEXT),
            NonPagedPool,
            &ctx);

        if (!NT_SUCCESS(status)) {


            //
            // 不能创建上下文，退出
            //

            DbgPrint("MDFMInstanceSetup: FltAllocateContext Failed\n");

            leave;
        }

        //
        //  总是获取卷的属性，这样能够获得扇区大小
        //

        status = FltGetVolumeProperties(FltObjects->Volume,
            volProp,
            sizeof(volPropBuffer),
            &retLen);

        if (!NT_SUCCESS(status)) {

            leave;
        }
        
        
        //
        //  在上下文中报错卷名，后续将用来获取卷的示例
        //

        ctx->Name.Buffer = ExAllocatePoolWithTag(NonPagedPool, 0x30, NAME_TAG);
        ctx->Name.MaximumLength = 0x30;
        RtlCopyUnicodeString(&ctx->Name, &volProp->RealDeviceName);
        


        //
        //  在上下文中保存扇区大小以供以后使用。
        //

        //FLT_ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= MIN_SECTOR_SIZE));

        ctx->SectorSize =volProp->SectorSize;


        //
        //  初始化缓冲区字段(稍后可能会分配)。
        //

        ctx->DosName.Buffer = NULL;

        //
        //  获取需要命名的存储设备对象。
        //

        status = FltGetDiskDeviceObject(FltObjects->Volume, &devObj);

        if (NT_SUCCESS(status)) {

            //
            //  尝试获取DOS名称。如果成功，我们将有一个分配的名称缓冲区。如果不是，它将是NULL
            //

            status = IoVolumeDeviceToDosName(devObj, &ctx->DosName);
        }

        //
        //  如果我们无法获得DOS名称，则获得NT名称。
        //

        if (!NT_SUCCESS(status)) {

            FLT_ASSERT(ctx->DosName.Buffer == NULL);

            //
            //  从属性中找出要使用的名称
            //

            if (volProp->RealDeviceName.Length > 0) {

                workingName = &volProp->RealDeviceName;

            }
            else if (volProp->FileSystemDeviceName.Length > 0) {

                workingName = &volProp->FileSystemDeviceName;

            }
            else {

                //
                //  没有名称，不保存上下文
                //
                DbgPrint("MDFMInstanceSetup: Get name failed\n");
                status = STATUS_FLT_DO_NOT_ATTACH;
                leave;
            }

            //
            //  获取要分配的缓冲区大小。这是字符串的长度加上后面冒号的空间。
            //

            size = workingName->Length + sizeof(WCHAR);

            //
            //  现在分配一个缓冲区来保存这个名称
            //

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "ctx->Name.Buffer will not be leaked because it is freed in CleanupVolumeContext")

            ctx->DosName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                size,
                NAME_TAG);

            if (ctx->DosName.Buffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;

                DbgPrint("MDFMInstanceSetup: Allocate Name.buffer failed\n");

                leave;
            }

            //
            //  初始化其余的字段
            //

            ctx->DosName.Length = 0;
            ctx->DosName.MaximumLength = size;

            //
            //  复制名称
            //

            RtlCopyUnicodeString(&ctx->DosName,
                workingName);

            //
            //  在后面加一个冒号可以使显示效果更好
            //

            //RtlAppendUnicodeToString(&ctx->Name,
            //    L":");
        }

        //
        //  设置上下文
        //

        status = FltSetVolumeContext(FltObjects->Volume,
            FLT_SET_CONTEXT_KEEP_IF_EXISTS,
            ctx,
            NULL);



        //
        //  已经定义了上下文就可以了。
        //

        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

            status = STATUS_SUCCESS;
        }

    }
    finally {

        //
        // 总是释放上下文。如果设置失败，它将释放上下文。如果没有，它将删除
        // 设置的引用。请注意，ctx中的名称缓冲区将由上下文清理例程释放。
        //

        if (ctx) {

            FltReleaseContext(ctx);
        }

        //
        //  删除FltGetDiskDeviceObject添加到设备对象的引用。
        //

        if (devObj) {

            ObDereferenceObject(devObj);
        }
    }

    return status;
}


NTSTATUS
MDFMInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DbgPrint("MDFMInstanceQueryTeardown: Entered\n");

    return STATUS_SUCCESS;
}


VOID
MDFMInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DbgPrint("MDFMInstanceTeardownStart: Entered\n");
}


VOID
MDFMInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DbgPrint("MDFMInstanceTeardownComplete: Entered\n");
}


VOID
ContextCleanUp(
    _In_ PFLT_CONTEXT context,
    _In_ FLT_CONTEXT_TYPE ContextType
)
{
    PAGED_CODE();

    switch (ContextType)
    {
        case FLT_VOLUME_CONTEXT:
        {
            PVOLUME_CONTEXT ctx = context;

            if (ctx->DosName.Buffer != NULL) {

                ExFreePool(ctx->DosName.Buffer);
                ExFreePool(ctx->Name.Buffer);
                ctx->DosName.Buffer = NULL;
                ctx->Name.Buffer = NULL;

            }

            break;
        }

        default:break;
    }
}

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    //DbgBreakPoint();

    // 初始化进程链表
    MdfmListInit();

    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( RegistryPath );

    DbgPrint("MDFM!DriverEntry: Entered\n");

    //
    //  初始化旁视列表，用于分配上下文结构，用于将信息从preOperation回调传递给postOperation回调。
    //

    ExInitializeNPagedLookasideList(&Pre2PostContextList,
        NULL,
        NULL,
        0,
        sizeof(PRE_2_POST_CONTEXT),
        PRE_2_POST_TAG,
        0);


    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );
   
    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }
    


    // 测试：识别机密进程
    if (MdfmIsListInited()) {
        UNICODE_STRING ProcName = { 0 };
        RtlInitUnicodeString(&ProcName, L"notepad.exe");
        MdfmAppendEncryptionProcess(&ProcName);
        
        /*UNICODE_STRING ProcName1 = { 0 };
        RtlInitUnicodeString(&ProcName1, L"notepad.exe");*/
        //MdfmAppendEncryptionProcess(&ProcName);


        //MdfmRemoveEncryptionProcess(&ProcName);
       // DbgBreakPoint();
    }
    
    return status;
}

NTSTATUS
MDFMUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DbgPrint("MDFM!MDFMUnload: Entered\n");

    //
    //  注销过滤器
    //

    FltUnregisterFilter( gFilterHandle );

    //
    //  销毁旁视列表
    //

    ExDeleteNPagedLookasideList(&Pre2PostContextList);

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
MDFMPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    //check

    if (!MdfmCheckCurProcprivilege(Data)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    UNICODE_STRING TXT = { 0 };

    RtlInitUnicodeString(&TXT, L"\\Users\\xxx\\Desktop\\test.txt");

    if (RtlCompareUnicodeString(&TXT, &FltObjects->FileObject->FileName, TRUE) != 0) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
    
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MDFMPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    DbgPrint("MDFMPostCreate: Entered\n");

    NTSTATUS status = STATUS_SUCCESS;
    
    PVOLUME_CONTEXT volCtx = NULL;
    PFLT_INSTANCE Instance = NULL;
    ULONG fileSize;


    try {

        /*
        *   获取文件大小
        */

        Instance = FltObjects->Instance;

        fileSize = MdfmGetFileSize(Instance, FltObjects->FileObject);

        DbgPrint("FileSize is %ld\n", fileSize);

        PFILE_FLAG pFlag = ExAllocatePoolWithTag( NonPagedPool, FLAG_SIZE, FLAG_TAG);
        RtlZeroMemory(pFlag, FLAG_SIZE);
        memcpy(pFlag->FileFlagHeader, &gFileFlagHeader, 16);
        for (UCHAR i = 0; i < (UCHAR)128; i++) {
            pFlag->FileKey[i] = i;
        }
        pFlag->FileValidLength = fileSize;


        MdfmWriteFileFlag(&Data, FltObjects, pFlag);

        MdfmReadFileFlag(FltObjects, pFlag);
        if (pFlag != NULL && strncmp(pFlag->FileFlagHeader, gFileFlagHeader, FILE_GUID_LENGTH) == 0) {
            DbgPrint("ValidLength: %ld\n", pFlag->FileValidLength);
        }

        ExFreePool(pFlag);

        fileSize = MdfmGetFileSize(Instance, FltObjects->FileObject);

        DbgPrint("FileSize is %ld\n", fileSize);


    }
    finally {
        
    } 


    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
MDFMPreRead(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{

    //check

    if (!MdfmCheckCurProcprivilege(Data)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }


    UNICODE_STRING TXT = { 0 };

    RtlInitUnicodeString(&TXT, L"\\Users\\xxx\\Desktop\\test.txt");

    if (RtlCompareUnicodeString(&TXT, &FltObjects->FileObject->FileName, TRUE) != 0) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (!FlagOn(Data->Iopb->IrpFlags, (IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE)))
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    DbgPrint("MDFMPreRead: Entered\n");

    // Get Flag

    PFILE_FLAG pFlag = ExAllocatePoolWithTag(NonPagedPool, FLAG_SIZE, FLAG_TAG);

    MdfmReadFileFlag(FltObjects, pFlag);

    // PreRead

    PreReadSwapBuffers(&Data, FltObjects, CompletionContext);

    FltSetCallbackDataDirty(Data);

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MDFMPostRead(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    DbgPrint("MDFMPostRead: Entered\n");

    PostReadSwapBuffers(&Data, FltObjects, CompletionContext,Flags);

    return FLT_POSTOP_FINISHED_PROCESSING;
}



FLT_PREOP_CALLBACK_STATUS
MDFMPreWrite(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
) 
{
    //check


    if (!MdfmCheckCurProcprivilege(Data)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    UNICODE_STRING TXT = { 0 };

    RtlInitUnicodeString(&TXT, L"\\Users\\xxx\\Desktop\\test.txt");

    if (RtlCompareUnicodeString(&TXT, &FltObjects->FileObject->FileName, TRUE) != 0) {
        DbgPrint("%wZ\n", FltObjects->FileObject->FileName);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (!FlagOn(Data->Iopb->IrpFlags, (IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE)))
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    DbgPrint("MDFMPreWrite:Entered\n");

    PreWriteSwapBuffers(&Data, FltObjects, CompletionContext);

    FltSetCallbackDataDirty(Data);

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MDFMPostWrite(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
) 
{

    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    DbgPrint("MDFMPostWrite: Entered\n");

    PostWriteSwapBuffers(&Data, FltObjects, CompletionContext, Flags);

    return FLT_POSTOP_FINISHED_PROCESSING;
}






FLT_PREOP_CALLBACK_STATUS
MDFMPreCleanUp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
) 
{
    //check

    if (!MdfmCheckCurProcprivilege(Data)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    UNICODE_STRING TXT = { 0 };

    RtlInitUnicodeString(&TXT, L"\\Users\\xxx\\Desktop\\test.txt");

    if (RtlCompareUnicodeString(&TXT, &FltObjects->FileObject->FileName, TRUE) != 0) {
        DbgPrint("%wZ\n", FltObjects->FileObject->FileName);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    DbgPrint("MDFMPreCleanUp:Entered\n");

    MdfmFileCacheClear(FltObjects->FileObject);

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MDFMPostCleanUp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
) 
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    DbgPrint("MDFMPostCleanUp: Entered\n");

    return FLT_POSTOP_FINISHED_PROCESSING;
}

//FLT_PREOP_CALLBACK_STATUS
//MDFMPreClose(
//    _Inout_ PFLT_CALLBACK_DATA Data,
//    _In_ PCFLT_RELATED_OBJECTS FltObjects,
//    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
//)
//{
//
//    //check
//
//    if (!MdfmCheckCurProcprivilege(Data)) {
//        return FLT_PREOP_SUCCESS_NO_CALLBACK;
//    }
//
//    UNICODE_STRING TXT = { 0 };
//
//    RtlInitUnicodeString(&TXT, L"\\Users\\xxx\\Desktop\\test.txt");
//
//    if (RtlCompareUnicodeString(&TXT, &FltObjects->FileObject->FileName, TRUE) != 0) {
//        DbgPrint("%wZ\n", FltObjects->FileObject->FileName);
//        return FLT_PREOP_SUCCESS_NO_CALLBACK;
//    }
//
//    DbgPrint("MDFMPreClose:Entered\n");
//
//    //MdfmFileCacheClear(FltObjects->FileObject);
//
//    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
//}
//
//FLT_POSTOP_CALLBACK_STATUS
//MDFMPostClose(
//    _Inout_ PFLT_CALLBACK_DATA Data,
//    _In_ PCFLT_RELATED_OBJECTS FltObjects,
//    _In_opt_ PVOID CompletionContext,
//    _In_ FLT_POST_OPERATION_FLAGS Flags
//) {
//    UNREFERENCED_PARAMETER(Data);
//    UNREFERENCED_PARAMETER(FltObjects);
//    UNREFERENCED_PARAMETER(CompletionContext);
//    UNREFERENCED_PARAMETER(Flags);
//
//    PAGED_CODE();
//
//    DbgPrint("MDFMPostClose: Entered\n");
//
//    return FLT_POSTOP_FINISHED_PROCESSING;
//}