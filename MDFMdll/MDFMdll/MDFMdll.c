#include "MDFMdll.h"

HANDLE g_hPort = INVALID_HANDLE_VALUE;

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitialCommunicationPort();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

int InitialCommunicationPort(void)
{
	DWORD hResult = FilterConnectCommunicationPort(
		MDFM_PORT_NAME,
		0,
		NULL,
		0,
		NULL,
		&g_hPort);

	if (hResult != S_OK) {
		return hResult;
	}
	return 0;
}

int MdfmSendMessage(PVOID InputBuffer)
{
	DWORD bytesReturned = 0;
	DWORD hResult = 0;
	PMDFM_MESSAGE commandMessage = (PMDFM_MESSAGE)InputBuffer;

	hResult = FilterSendMessage(
		g_hPort,
		commandMessage,
		sizeof(MDFM_MESSAGE),
		NULL,
		0,
		&bytesReturned);

	if (hResult != S_OK) {
		return hResult;
	}
	return 0;
}