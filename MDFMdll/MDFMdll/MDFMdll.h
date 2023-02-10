#include <Windows.h>
#include <stdio.h>
#include <fltUser.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "fltLib.lib")

extern HANDLE g_hPort;

#define MDFM_NAME            L"MDFM"
#define MDFM_PORT_NAME       L"\\MDFMPort"

__declspec(dllexport)	int InitialCommunicationPort(void);
__declspec(dllexport)   int MdfmSendMessage(PVOID InputBuffer);

typedef enum _MDFM_ACCESS {
    MDFM_COMMAND_BLOCK = 0,
    MDFM_COMMAND_READ,
    MDFM_COMMAND_ALL
} MDFM_ACCESS;

typedef struct _MDFM_MESSAGE {
    char           AccessProcess[100];
    UCHAR           Hash[32];
    MDFM_ACCESS 	AccessPrivilege;
} MDFM_MESSAGE, * PMDFM_MESSAGE;
