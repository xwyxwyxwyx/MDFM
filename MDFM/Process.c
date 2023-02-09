#pragma once
#include <fltKernel.h>
#include <dontuse.h>

#define MDFM_MEM_NODE_TAG 'node'
#define MDFM_MEM_STR_TAG 'str'

NTKERNELAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);	// 声明使用此函数获取进程名称

typedef struct {
    LIST_ENTRY listEntry;
    UNICODE_STRING encryptionProcess;
}EPT_PROC, * PEPT_PROC;

static LIST_ENTRY s_mdfm_eptProc_list;
static BOOLEAN s_mdfm_eptProc_list_inited = FALSE;
static KSPIN_LOCK s_mdfm_list_lock;
static KIRQL s_mdfm_list_lock_irql;

BOOLEAN MdfmIsListInited()
{
    return s_mdfm_eptProc_list_inited;
}

void MdfmListLock()
{
    ASSERT(s_mdfm_eptProc_list_inited);
    KeAcquireSpinLock(&s_mdfm_list_lock, &s_mdfm_list_lock_irql);
}

void MdfmListUnlock()
{
    ASSERT(s_mdfm_eptProc_list_inited);
    KeReleaseSpinLock(&s_mdfm_list_lock, s_mdfm_list_lock_irql);
}

void MdfmListInit()
{
    InitializeListHead(&s_mdfm_eptProc_list);
    KeInitializeSpinLock(&s_mdfm_list_lock);
    s_mdfm_eptProc_list_inited = TRUE;
}

// 以下函数可以获得进程名。
NTSTATUS MdfmCurProcName(PFLT_CALLBACK_DATA Data, PCHAR* ProcessName)
{
    //DbgBreakPoint();

    HANDLE pid = PsGetCurrentProcessId();
    // 获取进程名
    PEPROCESS processObj = NULL;					// 定义一个进程对象
    PsLookupProcessByProcessId(pid, &processObj);	// 根据pid获取进程对象
    if (!processObj) {
        return STATUS_UNSUCCESSFUL;
    }
    ObDereferenceObject(processObj);				// 解除引用
    *ProcessName = PsGetProcessImageFileName(processObj);// 根据进程对象获取进程名
    //DbgPrint("process:%s\n", *ProcessName);

    return STATUS_SUCCESS;
}

// 识别是否是机密进程
BOOLEAN MdfmIsEncryptionProcess(PUNICODE_STRING proc_name)
{
    //DbgBreakPoint();

    ASSERT(s_mdfm_eptProc_list_inited);

    if (proc_name == NULL) {
        DbgPrint("MdfmIsEncryptionProcess:proc_name is NULL\n");
        return FALSE;
    }
    if (IsListEmpty(&s_mdfm_eptProc_list)) {
        DbgPrint("Empty\n");
        return FALSE;
    }

    PLIST_ENTRY p;
    PEPT_PROC node;
    for (p = s_mdfm_eptProc_list.Flink; p != &s_mdfm_eptProc_list; p = p->Flink)
    {
        node = (PEPT_PROC)p;
        if (RtlCompareUnicodeString(&(node->encryptionProcess), proc_name, TRUE) == 0)
        {
            //DbgPrint("MdfmIsEncryptionProcess: process %wZ is encryptionProcess\n", *(proc_name));
            return TRUE;
        }
    }

    //UNICODE_STRING Notepad = { 0 };
    //RtlInitUnicodeString(&Notepad, L"notepad.exe");
    //if (RtlCompareUnicodeString(&Notepad, proc_name, TRUE) == 0) {
    //    return TRUE;
    //}



    return FALSE;
}

// 追加一个需要管控的机密进程
BOOLEAN MdfmAppendEncryptionProcess(PUNICODE_STRING proc_name)
{

    //((PEPT_PROC)s_mdfm_eptProc_list.Flink)->encryptionProcess;
    //((PEPT_PROC)s_mdfm_eptProc_list.Flink->Flink)->encryptionProcess;
    //DbgBreakPoint();

    // 先分配空间
    PEPT_PROC node = (PEPT_PROC)ExAllocatePoolWithTag(NonPagedPool, sizeof(EPT_PROC), MDFM_MEM_NODE_TAG);
    if (node == NULL) {
        ASSERT(FALSE);
        return FALSE;
    }

    //拷贝字符串
    node->encryptionProcess.Buffer = ExAllocatePoolWithTag(NonPagedPool, 0x1000, MDFM_MEM_STR_TAG);
    node->encryptionProcess.MaximumLength = 0x1000;
    RtlCopyUnicodeString(&node->encryptionProcess, proc_name);

    // 加锁并查找，如果已经有了，返回失败。
    MdfmListLock();

    if (MdfmIsEncryptionProcess(proc_name))
    {
        DbgPrint("MdfmAppendEncryptionProcess: Process already exist\n");
        ExFreePool(node);
        MdfmListUnlock();
        return FALSE;
    }

    // 否则的话，在这里插入到链表里。
    InsertHeadList(&s_mdfm_eptProc_list, (PLIST_ENTRY)node);
    MdfmListUnlock();

    return TRUE;
}

// 删除一个需要管控的机密进程
BOOLEAN MdfmRemoveEncryptionProcess(PUNICODE_STRING proc_name) {
    // DbgBreakPoint();

    if (IsListEmpty(&s_mdfm_eptProc_list)) {
        return FALSE;
    }


    PEPT_PROC node = NULL;

    // 加锁并查找，获取对应的节点
    MdfmListLock();
    PLIST_ENTRY p;
    BOOLEAN isFound = FALSE;
    for (p = s_mdfm_eptProc_list.Flink; p != &s_mdfm_eptProc_list; p = p->Flink)
    {
        node = (PEPT_PROC)p;
        if (RtlCompareUnicodeString(&(node->encryptionProcess), proc_name, TRUE) == 0)
        {
            isFound = TRUE;
            break;
        }
    }

    // 找到了对应节点，删除。没找到，返回失败
    if (isFound) {
        // 在这里删除节点。
        RemoveEntryList((PLIST_ENTRY)node);
        ExFreePool(node);
    }
    else {
        return FALSE;
    }

    MdfmListUnlock();

    return TRUE;
}

// 检查当前进程的权限
BOOLEAN MdfmCheckCurProcprivilege(PFLT_CALLBACK_DATA Data) {

    //获取当前进程名
    PCHAR NAME = NULL;
    MdfmCurProcName(Data, &NAME);

    //窄字符转宽字符
    STRING str = { 0 };
    UNICODE_STRING proc_name = { 0 };

    RtlInitString(&str, NAME);
    RtlAnsiStringToUnicodeString(&proc_name, &str, TRUE);


    if (MdfmIsEncryptionProcess(&proc_name)) {
        return TRUE;
    }

    return FALSE;
}




