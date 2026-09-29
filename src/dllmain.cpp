#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "config.h"
static volatile bool g_running=true;
static DWORD WINAPI MainThread(LPVOID){LoadItemHotkeyConfig(L"itemhotkeys.ini");while(g_running)Sleep(1000);return 0;}
BOOL APIENTRY DllMain(HMODULE,DWORD r,LPVOID){if(r==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(GetModuleHandleW(nullptr));CreateThread(nullptr,0,MainThread,nullptr,0,nullptr);}else if(r==DLL_PROCESS_DETACH)g_running=false;return TRUE;}
