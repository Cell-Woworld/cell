// dllmain.cpp : 定義 DLL 應用程式的進入點。
#include "pch.h"
#include <google/protobuf/stubs/common.h>

#ifdef _WIN32
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		google::protobuf::ShutdownProtobufLibrary();
		break;
    }
    return TRUE;
}
#endif