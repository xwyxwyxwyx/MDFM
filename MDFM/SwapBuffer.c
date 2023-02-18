#include "Context.h"
#include "Encrypt.h"
#include "Flag.h"

extern NPAGED_LOOKASIDE_LIST Pre2PostContextList;          // 用于分配上下文的旁视列表


FLT_POSTOP_CALLBACK_STATUS PostReadSwapBuffersWhenSafe(IN PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN PVOID CompletionContext, IN FLT_POST_OPERATION_FLAGS Flags);

FLT_PREOP_CALLBACK_STATUS
PreWriteSwapBuffers(
    IN OUT PFLT_CALLBACK_DATA* Data, 
    IN PCFLT_RELATED_OBJECTS FltObjects,
    OUT PVOID* CompletionContext
)
{
    
    PFLT_IO_PARAMETER_BLOCK iopb = (*Data)->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    PVOID origBuf;
    NTSTATUS status;
    ULONG writeLen = iopb->Parameters.Write.Length;

    try {

        //
        //  如果他们试图写0字节，那么什么都不做，我们不需要一个后操作回调。
        //

        if (writeLen == 0) {

            leave;
        }

        //
        //  获取卷上下文，以便在调试输出中显示卷名。
        //

        status = FltGetVolumeContext(FltObjects->Filter,
            FltObjects->Volume,
            &volCtx);

        if (!NT_SUCCESS(status)) {

            DbgPrint("SwapPreWriteBuffers: Get VolumeContext Failed\n");

            leave;
        }

        //
        //  如果这是一个非缓存的I/O，我们需要将长度四舍五入到这个设备的扇区大小。
        //  我们必须这样做，因为文件系统这样做，我们需要确保我们的缓冲区与他们所期望的一样大。
        //

        if (FlagOn(IRP_NOCACHE, iopb->IrpFlags)) {

            writeLen = (ULONG)ROUND_TO_SIZE(writeLen, volCtx->SectorSize);
        }

        //
        //  为要交换的缓冲区分配对齐的非page内存。这实际上只对非缓存IO有必要，但我们总是为了简化而这样做。
        //  如果我们无法获得内存，就不要在这个操作中交换缓冲区。
        //

        newBuf = FltAllocatePoolAlignedWithTag(FltObjects->Instance,
            NonPagedPool,
            (SIZE_T)writeLen,
            BUFFER_SWAP_TAG);

        if (newBuf == NULL) {

            DbgPrint("SwapPreWriteBuffers: Allocate newBuf Failed\n");

            leave;
        }

        //
        //  我们只需要为IRP操作构建一个MDL。对于FASTIO操作，我们不需要这样做，因为这是浪费时间，
        //  因为FASTIO接口没有将MDL传递给文件系统的参数。
        //

        if (FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

            //
            //  为新分配的内存分配一个MDL。如果MDL分配失败，则不会为该操作交换缓冲区
            //

            newMdl = IoAllocateMdl(newBuf,
                writeLen,
                FALSE,
                FALSE,
                NULL);

            if (newMdl == NULL) {

                DbgPrint("SwapPreWriteBuffers: Get newMdl failed\n");

                leave;
            }

            //
            //  为刚才分配的非分页池设置MDL
            //

            MmBuildMdlForNonPagedPool(newMdl);
        }

        //
        //  如果用户的原始缓冲区有一个MDL，则获取一个系统地址。
        //

        if (iopb->Parameters.Write.MdlAddress != NULL) {

            //
            //  这应该是一个简单的MDL。我们不期望链式mdl在堆栈的这么高的位置
            //

            FLT_ASSERT(((PMDL)iopb->Parameters.Write.MdlAddress)->Next == NULL);

            origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Write.MdlAddress,
                NormalPagePriority | MdlMappingNoExecute);

            if (origBuf == NULL) {

                DbgPrint("SwapPreWriteBuffers: Get origBuf failed\n");

                //
                //  如果不能为用户缓冲区获取系统地址，则此操作将失败。
                //

                (*Data)->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                (*Data)->IoStatus.Information = 0;
                retValue = FLT_PREOP_COMPLETE;
                leave;
            }

        }
        else {

            //
            //  没有定义MDL，使用给定的缓冲区地址。
            //

            origBuf = iopb->Parameters.Write.WriteBuffer;
        }

        //
        // 复制内存，我们必须在try/expect中执行此操作，因为我们可能会使用用户缓冲区地址
        //

        try {

            RtlCopyMemory(newBuf,
                origBuf,
                writeLen);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            //  复制失败，返回错误，操作失败
            //

            (*Data)->IoStatus.Status = GetExceptionCode();
            (*Data)->IoStatus.Information = 0;
            retValue = FLT_PREOP_COMPLETE;

            DbgPrint("SwapPreWriteBuffers: Invalid user buffer");

            leave;
        }

        //
        //  我们准备交换缓冲区，申请一个pre2Post上下文结构。我们需要它将卷上下文和分配内存缓冲区传递给后操作回调。
        //

        p2pCtx = ExAllocateFromNPagedLookasideList(&Pre2PostContextList);

        if (p2pCtx == NULL) {

            DbgPrint("SwapPreWriteBuffers: Allocate p2pCtx failed\n");

            leave;
        }

        //
        //  替换缓冲区之前，数据加密
        //

        //MdfmAesEncrypt(newBuf, writeLen);

        //
        //  设置新的缓冲区
        //

        iopb->Parameters.Write.WriteBuffer = newBuf;
        iopb->Parameters.Write.MdlAddress = newMdl;
        //iopb->Parameters.Write.ByteOffset.QuadPart += ROUND_TO_SIZE(FLAG_SIZE, volCtx->SectorSize);
        FltSetCallbackDataDirty((*Data));

        //
        //  将状态传递给我们的操作后回调。
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  返回我们需要的操作后回调
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    }
    finally {

        //
        //  如果我们不想要一个操作后回调，那么释放缓冲区或MDL(如果它已分配)。
        //

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                FltFreePoolAlignedWithTag(FltObjects->Instance,
                    newBuf,
                    BUFFER_SWAP_TAG);

            }

            if (newMdl != NULL) {

                IoFreeMdl(newMdl);
            }

            if (volCtx != NULL) {

                FltReleaseContext(volCtx);
            }
        }
    }

    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
PostWriteSwapBuffers(
    _Inout_ PFLT_CALLBACK_DATA *Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    DbgPrint("SwapPostWriteBuffers: Freeing Context\n");

    //
    //  释放内存池和卷上下文
    //

    FltFreePoolAlignedWithTag(FltObjects->Instance,
        p2pCtx->SwappedBuffer,
        BUFFER_SWAP_TAG);

    FltReleaseContext(p2pCtx->VolCtx);

    ExFreeToNPagedLookasideList(&Pre2PostContextList,
        p2pCtx);

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
PreReadSwapBuffers(
    IN OUT PFLT_CALLBACK_DATA* Data, 
    IN PCFLT_RELATED_OBJECTS FltObjects,
    OUT PVOID* CompletionContext
)
{
    PFLT_IO_PARAMETER_BLOCK iopb = (*Data)->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    NTSTATUS status;

    ULONG readLen = iopb->Parameters.Read.Length;

    PVOLUME_CONTEXT volCtx = NULL;
     

    try {

        //
        //  如果它们试图读取0字节，那么什么都不做，我们不需要一个后操作回调。
        //

        if (readLen == 0) {

            leave;
        }

        //
        // 获取卷上下文，我们将会用到里面保存的扇区大小
        //

        status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &volCtx);
        

        if (!NT_SUCCESS(status)) {

            DbgPrint("[MdfmReadFileFlag]->FltGetVolumeFromInstance failed. Status = %x\n", status);
            leave;
        }
        


        //
        //  如果这是一个非缓存的I/O，我们需要将长度四舍五入到这个设备的
        //  扇区大小。我们必须这样做，因为文件系统这样做，我们需要确保我
        //  们的缓冲区与他们所期望的一样大。
        //

        if (FlagOn(IRP_NOCACHE, iopb->IrpFlags)) {

            readLen = (ULONG)ROUND_TO_SIZE(readLen, volCtx->SectorSize);
        }

        //
        //  为要交换的缓冲区分配对齐的非page内存。这实际上只对非缓存IO有必要，
        //  但我们总是为了简化而这样做。如果我们无法获得内存，就不要在这个操作中交换缓冲区。
        //

        newBuf = FltAllocatePoolAlignedWithTag(FltObjects->Instance,
            NonPagedPool,
            (SIZE_T)readLen,
            BUFFER_SWAP_TAG);

        if (newBuf == NULL) {

            DbgPrint("SwapPreReadBuffers: Get newBuf failed\n");

            leave;
        }

        //
        //  我们只需要为IRP操作构建一个MDL。对于FASTIO操作，我们不需要这样做，因为FASTIO接口没有将MDL传递给文件系统的参数。
        //

        //
        //  无论是newBuf还是newMdl，都是指向的同一个地址，这样我们可以用一个唯一的地址来代替缓冲区
        //  这个地址后面会保存到p2pCtx->SwappedBuffer
        //

        if (FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

            //
            //  为新分配的内存分配一个MDL。如果MDL分配失败，则不会为该操作交换缓冲区
            //

            newMdl = IoAllocateMdl(newBuf,
                readLen,
                FALSE,
                FALSE,
                NULL);

            if (newMdl == NULL) {

                DbgPrint("SwapPreReadBuffers: Allocate Mdl failed\n");

                leave;
            }

            //
            //  为刚才分配的非分页池设置MDL
            //

            MmBuildMdlForNonPagedPool(newMdl);
        }

        //
        //  我们准备交换缓冲区，得到一个pre2Post上下文结构。我们需要它将卷上下文和分配内存缓冲区传递给后操作回调。
        //

        p2pCtx = ExAllocateFromNPagedLookasideList(&Pre2PostContextList);

        if (p2pCtx == NULL) {

            DbgPrint("SwapPreReadBuffers: Allocate p2pCtx failed\n");

            leave;
        }


        //
        //  更新缓冲区指针和MDL地址，标记我们已经改变了一些东西。
        //

        iopb->Parameters.Read.ReadBuffer = newBuf;
        iopb->Parameters.Read.MdlAddress = newMdl;
        iopb->Parameters.Read.ByteOffset.QuadPart += ROUND_TO_SIZE(FLAG_SIZE, volCtx->SectorSize);
        FltSetCallbackDataDirty((*Data));

        //
        //  将状态传递给我们的后操作回调。
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  返回我们需要后操作回调
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    }
    finally {

        //
        //  如果我们不想要操作后回调，那么清理状态。
        //

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                FltFreePoolAlignedWithTag(FltObjects->Instance,
                    newBuf,
                    BUFFER_SWAP_TAG);
            }

            if (newMdl != NULL) {

                IoFreeMdl(newMdl);
            }
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
PostReadSwapBuffers(
    IN OUT PFLT_CALLBACK_DATA* Data, 
    IN PCFLT_RELATED_OBJECTS FltObjects, 
    IN PVOID CompletionContext, 
    IN FLT_POST_OPERATION_FLAGS Flags
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PVOID origBuf;
    PFLT_IO_PARAMETER_BLOCK iopb = (*Data)->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = (PPRE_2_POST_CONTEXT)CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    PVOLUME_CONTEXT volCtx = NULL;


    //
    //  This system won't draining an operation with swapped buffers, verify
    //  the draining flag is not set.
    //

    FLT_ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    try {

        //
        //  如果操作失败或计数为零，则没有数据要复制，因此现在就返回。
        //

        if (!NT_SUCCESS((*Data)->IoStatus.Status) ||
            ((*Data)->IoStatus.Information == 0)) {
               
            DbgPrint("PostReadSwapBuffers: 操作失败，无需复制数据\n");

            leave;
        }

        //
        //  我们需要将读取的数据复制回用户缓冲区。注意，后操作传入的参数是原始缓冲区，而不是我们交换的缓冲区。
        //

        if (iopb->Parameters.Read.MdlAddress != NULL) {

            //
            //  这应该是一个简单的MDL。我们不期望链式mdl在堆栈的这么高的位置
            //

            FLT_ASSERT(((PMDL)iopb->Parameters.Read.MdlAddress)->Next == NULL);

            //
            //  因为有一个为原始缓冲区定义的MDL，为它获取一个系统地址，这样
            //  我们就可以将数据复制回它。我们必须这样做，因为我们不知道我们
            //  在什么线程上下文中。
            //

            origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
                NormalPagePriority | MdlMappingNoExecute);

            if (origBuf == NULL) {

                DbgPrint("PostReadSwapBuffers: Get origBuf Failed\n");

                //
                //  如果未能获得系统地址，则标记读取失败并返回。
                //

                (*Data)->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                (*Data)->IoStatus.Information = 0;
                leave;
            }

        }
        else if (FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
            FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) {

            //
            //  如果这是一个系统缓冲区，只需使用给定的地址，因为它在所有线程上下文中都有效。
            //  如果这是一个FASTIO操作，我们可以只使用缓冲区(在try/except中)，因为我们知道我
            //  们在正确的线程上下文中(你不能挂起FASTIO)。
            //

            origBuf = iopb->Parameters.Read.ReadBuffer;

        }
        else {

            //
            //  它们没有MDL，这不是系统缓冲区，也不是fastio，所以这可能是任意的用户缓冲区。
            //  我们不能在DPC级别进行处理，所以尝试获得一个安全的IRQL，这样我们就可以进行处理。
            //

            if (FltDoCompletionProcessingWhenSafe((*Data),
                FltObjects,
                CompletionContext,
                Flags,
                PostReadSwapBuffersWhenSafe,
                &retValue)) {

                //
                //  这个操作已经被转移到一个安全的IRQL中，被调用的例程将执行(或已经执行)释放，所以不要在我们的例程中执行。
                //

                cleanupAllocatedBuffer = FALSE;

            }
            else {

                //
                //  我们处于这样一种状态:我们无法获得一个安全的IRQL，我们也没有一个MDL。我们无法安全地将数据复制回用户缓冲区，
                //  操作失败并返回。这种情况不应该发生，因为在发布不安全的情况下，我们应该有一个MDL。
                //

                DbgPrint("SwapPostReadBuffers: Unable to get a safe IRQL\n");

                (*Data)->IoStatus.Status = STATUS_UNSUCCESSFUL;
                (*Data)->IoStatus.Information = 0;
            }

            leave;
        }

        //
        //  我们要么有一个系统缓冲区，要么这是一个fastio操作，所以我们在适当的上下文中。复制数据。
        //

        try {

            RtlCopyMemory(origBuf,
                p2pCtx->SwappedBuffer,
                iopb->Parameters.Read.Length);

            //
            //  替换缓冲区之后，数据解密
            //
            
            //MdfmAesDecrypt(origBuf, iopb->Parameters.Read.Length);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            //  复制失败，返回错误，操作失败。
            //

            (*Data)->IoStatus.Status = GetExceptionCode();
            (*Data)->IoStatus.Information = 0;

            DbgPrint("SwapPostReadBuffers: Copy failed\n");
        }

    }
    finally {

        //
        //  如果需要的话，清理分配的内存并释放卷上下文。MDL的释放(如果有的话)由FltMgr处理。
        //

        if (cleanupAllocatedBuffer) {

            DbgPrint("SwapPostReadBuffers: Freeing Context\n");

            FltFreePoolAlignedWithTag(FltObjects->Instance,
                p2pCtx->SwappedBuffer,
                BUFFER_SWAP_TAG);

            FltReleaseContext(p2pCtx->VolCtx);

            ExFreeToNPagedLookasideList(&Pre2PostContextList,
                p2pCtx);
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS 
PostReadSwapBuffersWhenSafe(
    IN PFLT_CALLBACK_DATA Data, 
    IN PCFLT_RELATED_OBJECTS FltObjects, 
    IN PVOID CompletionContext,
    IN FLT_POST_OPERATION_FLAGS Flags
)
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    FLT_ASSERT(Data->IoStatus.Information != 0);

    //
    //  我们将在一个安全的IRQL锁定并访问用户缓冲区（UserBuffer），然后复制它
    //

    status = FltLockUserBuffer(Data);

    if (!NT_SUCCESS(status)) {

        DbgPrint("PostReadSwapBuffersWhenSafe: Can not lock UserBuffer\n");

        //
        //  如果不能锁定缓冲区，则操作失败
        //

        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    }
    else {

        //
        //  获取此缓冲区的系统地址。
        //

        origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
            NormalPagePriority | MdlMappingNoExecute);

        if (origBuf == NULL) {

            DbgPrint("PostReadSwapBuffersWhenSafe: Failed to get System address for MDL\n");

            //
            //  如果无法获得系统缓冲区地址，则操作失败
            //

            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        }
        else {

            //
            //  将数据复制回原始缓冲区
            //
            //  NOTE:  由于FASTFAT中的一个错误，它在信息字段中返回错误的长度(它很短)，我们总是会复制原始的缓冲区长度。
            //

            RtlCopyMemory(origBuf,
                p2pCtx->SwappedBuffer,
                /*Data->IoStatus.Information*/
                iopb->Parameters.DirectoryControl.QueryDirectory.Length);
        }
    }

    //
    //  释放我们分配的内存并返回
    //

    DbgPrint("PostReadSwapBuffersWhenSafe: Freeing\n");

    ExFreePool(p2pCtx->SwappedBuffer);
    FltReleaseContext(p2pCtx->VolCtx);

    ExFreeToNPagedLookasideList(&Pre2PostContextList,
        p2pCtx);

    return FLT_POSTOP_FINISHED_PROCESSING;
}