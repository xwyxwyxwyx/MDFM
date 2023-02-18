#include "Context.h"
#include "Encrypt.h"
#include "Flag.h"

extern NPAGED_LOOKASIDE_LIST Pre2PostContextList;          // ���ڷ��������ĵ������б�


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
        //  ���������ͼд0�ֽڣ���ôʲô�����������ǲ���Ҫһ��������ص���
        //

        if (writeLen == 0) {

            leave;
        }

        //
        //  ��ȡ�������ģ��Ա��ڵ����������ʾ������
        //

        status = FltGetVolumeContext(FltObjects->Filter,
            FltObjects->Volume,
            &volCtx);

        if (!NT_SUCCESS(status)) {

            DbgPrint("SwapPreWriteBuffers: Get VolumeContext Failed\n");

            leave;
        }

        //
        //  �������һ���ǻ����I/O��������Ҫ�������������뵽����豸��������С��
        //  ���Ǳ�������������Ϊ�ļ�ϵͳ��������������Ҫȷ�����ǵĻ�������������������һ����
        //

        if (FlagOn(IRP_NOCACHE, iopb->IrpFlags)) {

            writeLen = (ULONG)ROUND_TO_SIZE(writeLen, volCtx->SectorSize);
        }

        //
        //  ΪҪ�����Ļ������������ķ�page�ڴ档��ʵ����ֻ�Էǻ���IO�б�Ҫ������������Ϊ�˼򻯶���������
        //  ��������޷�����ڴ棬�Ͳ�Ҫ����������н�����������
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
        //  ����ֻ��ҪΪIRP��������һ��MDL������FASTIO���������ǲ���Ҫ����������Ϊ�����˷�ʱ�䣬
        //  ��ΪFASTIO�ӿ�û�н�MDL���ݸ��ļ�ϵͳ�Ĳ�����
        //

        if (FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

            //
            //  Ϊ�·�����ڴ����һ��MDL�����MDL����ʧ�ܣ��򲻻�Ϊ�ò�������������
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
            //  Ϊ�ղŷ���ķǷ�ҳ������MDL
            //

            MmBuildMdlForNonPagedPool(newMdl);
        }

        //
        //  ����û���ԭʼ��������һ��MDL�����ȡһ��ϵͳ��ַ��
        //

        if (iopb->Parameters.Write.MdlAddress != NULL) {

            //
            //  ��Ӧ����һ���򵥵�MDL�����ǲ�������ʽmdl�ڶ�ջ����ô�ߵ�λ��
            //

            FLT_ASSERT(((PMDL)iopb->Parameters.Write.MdlAddress)->Next == NULL);

            origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Write.MdlAddress,
                NormalPagePriority | MdlMappingNoExecute);

            if (origBuf == NULL) {

                DbgPrint("SwapPreWriteBuffers: Get origBuf failed\n");

                //
                //  �������Ϊ�û���������ȡϵͳ��ַ����˲�����ʧ�ܡ�
                //

                (*Data)->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                (*Data)->IoStatus.Information = 0;
                retValue = FLT_PREOP_COMPLETE;
                leave;
            }

        }
        else {

            //
            //  û�ж���MDL��ʹ�ø����Ļ�������ַ��
            //

            origBuf = iopb->Parameters.Write.WriteBuffer;
        }

        //
        // �����ڴ棬���Ǳ�����try/expect��ִ�д˲�������Ϊ���ǿ��ܻ�ʹ���û���������ַ
        //

        try {

            RtlCopyMemory(newBuf,
                origBuf,
                writeLen);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            //  ����ʧ�ܣ����ش��󣬲���ʧ��
            //

            (*Data)->IoStatus.Status = GetExceptionCode();
            (*Data)->IoStatus.Information = 0;
            retValue = FLT_PREOP_COMPLETE;

            DbgPrint("SwapPreWriteBuffers: Invalid user buffer");

            leave;
        }

        //
        //  ����׼������������������һ��pre2Post�����Ľṹ��������Ҫ�����������ĺͷ����ڴ滺�������ݸ�������ص���
        //

        p2pCtx = ExAllocateFromNPagedLookasideList(&Pre2PostContextList);

        if (p2pCtx == NULL) {

            DbgPrint("SwapPreWriteBuffers: Allocate p2pCtx failed\n");

            leave;
        }

        //
        //  �滻������֮ǰ�����ݼ���
        //

        //MdfmAesEncrypt(newBuf, writeLen);

        //
        //  �����µĻ�����
        //

        iopb->Parameters.Write.WriteBuffer = newBuf;
        iopb->Parameters.Write.MdlAddress = newMdl;
        //iopb->Parameters.Write.ByteOffset.QuadPart += ROUND_TO_SIZE(FLAG_SIZE, volCtx->SectorSize);
        FltSetCallbackDataDirty((*Data));

        //
        //  ��״̬���ݸ����ǵĲ�����ص���
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  ����������Ҫ�Ĳ�����ص�
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    }
    finally {

        //
        //  ������ǲ���Ҫһ��������ص�����ô�ͷŻ�������MDL(������ѷ���)��
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
    //  �ͷ��ڴ�غ;�������
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
        //  ���������ͼ��ȡ0�ֽڣ���ôʲô�����������ǲ���Ҫһ��������ص���
        //

        if (readLen == 0) {

            leave;
        }

        //
        // ��ȡ�������ģ����ǽ����õ����汣���������С
        //

        status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &volCtx);
        

        if (!NT_SUCCESS(status)) {

            DbgPrint("[MdfmReadFileFlag]->FltGetVolumeFromInstance failed. Status = %x\n", status);
            leave;
        }
        


        //
        //  �������һ���ǻ����I/O��������Ҫ�������������뵽����豸��
        //  ������С�����Ǳ�������������Ϊ�ļ�ϵͳ��������������Ҫȷ����
        //  �ǵĻ�������������������һ����
        //

        if (FlagOn(IRP_NOCACHE, iopb->IrpFlags)) {

            readLen = (ULONG)ROUND_TO_SIZE(readLen, volCtx->SectorSize);
        }

        //
        //  ΪҪ�����Ļ������������ķ�page�ڴ档��ʵ����ֻ�Էǻ���IO�б�Ҫ��
        //  ����������Ϊ�˼򻯶�����������������޷�����ڴ棬�Ͳ�Ҫ����������н�����������
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
        //  ����ֻ��ҪΪIRP��������һ��MDL������FASTIO���������ǲ���Ҫ����������ΪFASTIO�ӿ�û�н�MDL���ݸ��ļ�ϵͳ�Ĳ�����
        //

        //
        //  ������newBuf����newMdl������ָ���ͬһ����ַ���������ǿ�����һ��Ψһ�ĵ�ַ�����滺����
        //  �����ַ����ᱣ�浽p2pCtx->SwappedBuffer
        //

        if (FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

            //
            //  Ϊ�·�����ڴ����һ��MDL�����MDL����ʧ�ܣ��򲻻�Ϊ�ò�������������
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
            //  Ϊ�ղŷ���ķǷ�ҳ������MDL
            //

            MmBuildMdlForNonPagedPool(newMdl);
        }

        //
        //  ����׼���������������õ�һ��pre2Post�����Ľṹ��������Ҫ�����������ĺͷ����ڴ滺�������ݸ�������ص���
        //

        p2pCtx = ExAllocateFromNPagedLookasideList(&Pre2PostContextList);

        if (p2pCtx == NULL) {

            DbgPrint("SwapPreReadBuffers: Allocate p2pCtx failed\n");

            leave;
        }


        //
        //  ���»�����ָ���MDL��ַ����������Ѿ��ı���һЩ������
        //

        iopb->Parameters.Read.ReadBuffer = newBuf;
        iopb->Parameters.Read.MdlAddress = newMdl;
        iopb->Parameters.Read.ByteOffset.QuadPart += ROUND_TO_SIZE(FLAG_SIZE, volCtx->SectorSize);
        FltSetCallbackDataDirty((*Data));

        //
        //  ��״̬���ݸ����ǵĺ�����ص���
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  ����������Ҫ������ص�
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    }
    finally {

        //
        //  ������ǲ���Ҫ������ص�����ô����״̬��
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
        //  �������ʧ�ܻ����Ϊ�㣬��û������Ҫ���ƣ�������ھͷ��ء�
        //

        if (!NT_SUCCESS((*Data)->IoStatus.Status) ||
            ((*Data)->IoStatus.Information == 0)) {
               
            DbgPrint("PostReadSwapBuffers: ����ʧ�ܣ����踴������\n");

            leave;
        }

        //
        //  ������Ҫ����ȡ�����ݸ��ƻ��û���������ע�⣬���������Ĳ�����ԭʼ�����������������ǽ����Ļ�������
        //

        if (iopb->Parameters.Read.MdlAddress != NULL) {

            //
            //  ��Ӧ����һ���򵥵�MDL�����ǲ�������ʽmdl�ڶ�ջ����ô�ߵ�λ��
            //

            FLT_ASSERT(((PMDL)iopb->Parameters.Read.MdlAddress)->Next == NULL);

            //
            //  ��Ϊ��һ��Ϊԭʼ�����������MDL��Ϊ����ȡһ��ϵͳ��ַ������
            //  ���ǾͿ��Խ����ݸ��ƻ��������Ǳ�������������Ϊ���ǲ�֪������
            //  ��ʲô�߳��������С�
            //

            origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
                NormalPagePriority | MdlMappingNoExecute);

            if (origBuf == NULL) {

                DbgPrint("PostReadSwapBuffers: Get origBuf Failed\n");

                //
                //  ���δ�ܻ��ϵͳ��ַ�����Ƕ�ȡʧ�ܲ����ء�
                //

                (*Data)->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                (*Data)->IoStatus.Information = 0;
                leave;
            }

        }
        else if (FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
            FlagOn((*Data)->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) {

            //
            //  �������һ��ϵͳ��������ֻ��ʹ�ø����ĵ�ַ����Ϊ���������߳��������ж���Ч��
            //  �������һ��FASTIO���������ǿ���ֻʹ�û�����(��try/except��)����Ϊ����֪����
            //  ������ȷ���߳���������(�㲻�ܹ���FASTIO)��
            //

            origBuf = iopb->Parameters.Read.ReadBuffer;

        }
        else {

            //
            //  ����û��MDL���ⲻ��ϵͳ��������Ҳ����fastio�������������������û���������
            //  ���ǲ�����DPC������д������Գ��Ի��һ����ȫ��IRQL���������ǾͿ��Խ��д���
            //

            if (FltDoCompletionProcessingWhenSafe((*Data),
                FltObjects,
                CompletionContext,
                Flags,
                PostReadSwapBuffersWhenSafe,
                &retValue)) {

                //
                //  ��������Ѿ���ת�Ƶ�һ����ȫ��IRQL�У������õ����̽�ִ��(���Ѿ�ִ��)�ͷţ����Բ�Ҫ�����ǵ�������ִ�С�
                //

                cleanupAllocatedBuffer = FALSE;

            }
            else {

                //
                //  ���Ǵ�������һ��״̬:�����޷����һ����ȫ��IRQL������Ҳû��һ��MDL�������޷���ȫ�ؽ����ݸ��ƻ��û���������
                //  ����ʧ�ܲ����ء����������Ӧ�÷�������Ϊ�ڷ�������ȫ������£�����Ӧ����һ��MDL��
                //

                DbgPrint("SwapPostReadBuffers: Unable to get a safe IRQL\n");

                (*Data)->IoStatus.Status = STATUS_UNSUCCESSFUL;
                (*Data)->IoStatus.Information = 0;
            }

            leave;
        }

        //
        //  ����Ҫô��һ��ϵͳ��������Ҫô����һ��fastio�����������������ʵ����������С��������ݡ�
        //

        try {

            RtlCopyMemory(origBuf,
                p2pCtx->SwappedBuffer,
                iopb->Parameters.Read.Length);

            //
            //  �滻������֮�����ݽ���
            //
            
            //MdfmAesDecrypt(origBuf, iopb->Parameters.Read.Length);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            //  ����ʧ�ܣ����ش��󣬲���ʧ�ܡ�
            //

            (*Data)->IoStatus.Status = GetExceptionCode();
            (*Data)->IoStatus.Information = 0;

            DbgPrint("SwapPostReadBuffers: Copy failed\n");
        }

    }
    finally {

        //
        //  �����Ҫ�Ļ������������ڴ沢�ͷž������ġ�MDL���ͷ�(����еĻ�)��FltMgr����
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
    //  ���ǽ���һ����ȫ��IRQL�����������û���������UserBuffer����Ȼ������
    //

    status = FltLockUserBuffer(Data);

    if (!NT_SUCCESS(status)) {

        DbgPrint("PostReadSwapBuffersWhenSafe: Can not lock UserBuffer\n");

        //
        //  ������������������������ʧ��
        //

        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    }
    else {

        //
        //  ��ȡ�˻�������ϵͳ��ַ��
        //

        origBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
            NormalPagePriority | MdlMappingNoExecute);

        if (origBuf == NULL) {

            DbgPrint("PostReadSwapBuffersWhenSafe: Failed to get System address for MDL\n");

            //
            //  ����޷����ϵͳ��������ַ�������ʧ��
            //

            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        }
        else {

            //
            //  �����ݸ��ƻ�ԭʼ������
            //
            //  NOTE:  ����FASTFAT�е�һ������������Ϣ�ֶ��з��ش���ĳ���(���ܶ�)���������ǻḴ��ԭʼ�Ļ��������ȡ�
            //

            RtlCopyMemory(origBuf,
                p2pCtx->SwappedBuffer,
                /*Data->IoStatus.Information*/
                iopb->Parameters.DirectoryControl.QueryDirectory.Length);
        }
    }

    //
    //  �ͷ����Ƿ�����ڴ沢����
    //

    DbgPrint("PostReadSwapBuffersWhenSafe: Freeing\n");

    ExFreePool(p2pCtx->SwappedBuffer);
    FltReleaseContext(p2pCtx->VolCtx);

    ExFreeToNPagedLookasideList(&Pre2PostContextList,
        p2pCtx);

    return FLT_POSTOP_FINISHED_PROCESSING;
}