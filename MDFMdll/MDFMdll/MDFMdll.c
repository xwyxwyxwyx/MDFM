#include <stdio.h>
#include "MDFMdll.h"

//��ʼ�����
HANDLE g_hPort = INVALID_HANDLE_VALUE;

int InitialCommuicationPort(void)
{
    DWORD hResult = FilterConnectCommunicationPort(MINISPY_PORT_NAME, NULL, NULL, NULL, NULL, &g_hPort);
    printf("������ͨ�Ŷ˿ڳ�ʼ��\n");
    if (hResult != S_OK)
    {  
        printf("ͨ�Ŷ˿ڳ�ʼ�����ɹ�\n");
        return hResult;
    }
    printf("ͨ�Ŷ˿ڳ�ʼ���ɹ�\n");
    return 0 ;
}

int  MdfmSendMessage(PVOID InputBuffer)
{
    DWORD bytesReturned = 0;
    DWORD hResult = 0;

    printf("���뷢����Ϣ\n");
    hResult = FilterSendMessage(g_hPort, InputBuffer, sizeof(MDFM_MESSAGE), NULL, NULL, &bytesReturned);
    if (hResult != S_OK)
    {
        return hResult;
    }

    return 0 ;
}