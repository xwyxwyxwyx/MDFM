#pragma once
#include <fltKernel.h>
#include <dontuse.h>

#define MDFM_MEM_NODE_TAG 'node'
#define MDFM_MEM_STR_TAG 'str'

NTKERNELAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);	// ����ʹ�ô˺�����ȡ��������

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

// ���º������Ի�ý�������
NTSTATUS MdfmCurProcName(PFLT_CALLBACK_DATA Data, PCHAR* ProcessName)
{
    //DbgBreakPoint();

    HANDLE pid = PsGetCurrentProcessId();
    // ��ȡ������
    PEPROCESS processObj = NULL;					// ����һ�����̶���
    PsLookupProcessByProcessId(pid, &processObj);	// ����pid��ȡ���̶���
    if (!processObj) {
        return STATUS_UNSUCCESSFUL;
    }
    ObDereferenceObject(processObj);				// �������
    *ProcessName = PsGetProcessImageFileName(processObj);// ���ݽ��̶����ȡ������
    //DbgPrint("process:%s\n", *ProcessName);

    return STATUS_SUCCESS;
}

// ʶ���Ƿ��ǻ��ܽ���
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

// ׷��һ����Ҫ�ܿصĻ��ܽ���
BOOLEAN MdfmAppendEncryptionProcess(PUNICODE_STRING proc_name)
{

    //((PEPT_PROC)s_mdfm_eptProc_list.Flink)->encryptionProcess;
    //((PEPT_PROC)s_mdfm_eptProc_list.Flink->Flink)->encryptionProcess;
    //DbgBreakPoint();

    // �ȷ���ռ�
    PEPT_PROC node = (PEPT_PROC)ExAllocatePoolWithTag(NonPagedPool, sizeof(EPT_PROC), MDFM_MEM_NODE_TAG);
    if (node == NULL) {
        ASSERT(FALSE);
        return FALSE;
    }

    //�����ַ���
    node->encryptionProcess.Buffer = ExAllocatePoolWithTag(NonPagedPool, 0x1000, MDFM_MEM_STR_TAG);
    node->encryptionProcess.MaximumLength = 0x1000;
    RtlCopyUnicodeString(&node->encryptionProcess, proc_name);

    // ���������ң�����Ѿ����ˣ�����ʧ�ܡ�
    MdfmListLock();

    if (MdfmIsEncryptionProcess(proc_name))
    {
        DbgPrint("MdfmAppendEncryptionProcess: Process already exist\n");
        ExFreePool(node);
        MdfmListUnlock();
        return FALSE;
    }

    // ����Ļ�����������뵽�����
    InsertHeadList(&s_mdfm_eptProc_list, (PLIST_ENTRY)node);
    MdfmListUnlock();

    return TRUE;
}

// ɾ��һ����Ҫ�ܿصĻ��ܽ���
BOOLEAN MdfmRemoveEncryptionProcess(PUNICODE_STRING proc_name) {
    // DbgBreakPoint();

    if (IsListEmpty(&s_mdfm_eptProc_list)) {
        return FALSE;
    }


    PEPT_PROC node = NULL;

    // ���������ң���ȡ��Ӧ�Ľڵ�
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

    // �ҵ��˶�Ӧ�ڵ㣬ɾ����û�ҵ�������ʧ��
    if (isFound) {
        // ������ɾ���ڵ㡣
        RemoveEntryList((PLIST_ENTRY)node);
        ExFreePool(node);
    }
    else {
        return FALSE;
    }

    MdfmListUnlock();

    return TRUE;
}

// ��鵱ǰ���̵�Ȩ��
BOOLEAN MdfmCheckCurProcprivilege(PFLT_CALLBACK_DATA Data) {

    //��ȡ��ǰ������
    PCHAR NAME = NULL;
    MdfmCurProcName(Data, &NAME);

    //խ�ַ�ת���ַ�
    STRING str = { 0 };
    UNICODE_STRING proc_name = { 0 };

    RtlInitString(&str, NAME);
    RtlAnsiStringToUnicodeString(&proc_name, &str, TRUE);


    if (MdfmIsEncryptionProcess(&proc_name)) {
        return TRUE;
    }

    return FALSE;
}




