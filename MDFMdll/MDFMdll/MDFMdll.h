#ifndef _MINIFILTER_H_
#define _MINIFILTER_H_

#include <windows.h>
#include <stdio.h>
#include <FltUser.h>
#pragma comment(lib, "fltLib.lib")

extern HANDLE g_hPort;
#define MINISPY_PORT_NAME                                L"\\MDFMComPort"  //通信端口名字

#ifdef  MINI_EXPORTS
#define MINI_API _declspec(dllexport)
#else
#define MINI_API _declspec(dllexport)
#endif // NPMINI_EXPORTS

MINI_API int  InitialCommuicationPort(void);
MINI_API int  MdfmSendMessage(PVOID InputBuffer);

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
#endif