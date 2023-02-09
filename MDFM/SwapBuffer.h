#pragma once

#include "global.h"


FLT_PREOP_CALLBACK_STATUS
PreWriteSwapBuffers(
    IN OUT PFLT_CALLBACK_DATA* Data,
    IN PCFLT_RELATED_OBJECTS FltObjects,
    OUT PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PostWriteSwapBuffers(
    _Inout_ PFLT_CALLBACK_DATA* Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
PreReadSwapBuffers(
    IN OUT PFLT_CALLBACK_DATA* Data,
    IN PCFLT_RELATED_OBJECTS FltObjects,
    OUT PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PostReadSwapBuffers(
    IN OUT PFLT_CALLBACK_DATA* Data,
    IN PCFLT_RELATED_OBJECTS FltObjects,
    IN PVOID CompletionContext,
    IN FLT_POST_OPERATION_FLAGS Flags
);
