#pragma once
#include "global.h"


BOOLEAN MdfmIsListInited();

void MdfmListLock();

void MdfmListUnlock();

void MdfmListInit();

NTSTATUS MdfmCurProcName(PFLT_CALLBACK_DATA Data, PCHAR* ProcessName);

BOOLEAN MdfmIsEncryptionProcess(PUNICODE_STRING proc_name);

BOOLEAN MdfmAppendEncryptionProcess(PUNICODE_STRING proc_name);

BOOLEAN MdfmRemoveEncryptionProcess(PUNICODE_STRING proc_name);

BOOLEAN MdfmCheckCurProcprivilege(PFLT_CALLBACK_DATA Data);

