#pragma once
#include "global.h"


BOOLEAN MdfmAesEncrypt(IN OUT PUCHAR Buffer, IN OUT ULONG LengthReturned);

NTSTATUS MdfmAesDecrypt(IN OUT PUCHAR Buffer, IN ULONG Length);

