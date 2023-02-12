#include <stdio.h>
#include "MDFMdll.h"

//初始化句柄
HANDLE g_hPort = INVALID_HANDLE_VALUE;

int InitialCommuicationPort(void)
{
    DWORD hResult = FilterConnectCommunicationPort(MINISPY_PORT_NAME, NULL, NULL, NULL, NULL, &g_hPort);
    printf("进入了通信端口初始化\n");
    if (hResult != S_OK)
    {  
        printf("通信端口初始化不成功\n");
        return hResult;
    }
    printf("通信端口初始化成功\n");
    return 0 ;
}

int  MdfmSendMessage(PVOID InputBuffer)
{
    DWORD bytesReturned = 0;
    DWORD hResult = 0;

    printf("进入发送消息\n");
    hResult = FilterSendMessage(g_hPort, InputBuffer, sizeof(MDFM_MESSAGE), NULL, NULL, &bytesReturned);
    if (hResult != S_OK)
    {
        return hResult;
    }

    return 0 ;
}