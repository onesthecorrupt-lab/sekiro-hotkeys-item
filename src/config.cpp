#include "config.h"
#include <cwchar>
ItemSlot g_itemSlots[48]{};
bool LoadItemHotkeyConfig(const wchar_t* path){wchar_t b[64];for(int i=0;i<48;i++){wchar_t k[32];swprintf_s(k,L"Slot%02dItemId",i+1);GetPrivateProfileStringW(L"Items",k,L"0",b,64,path);g_itemSlots[i].itemId=_wtoi(b);swprintf_s(k,L"Slot%02dHotkey",i+1);GetPrivateProfileStringW(L"Hotkeys",k,L"0",b,64,path);g_itemSlots[i].hotkey=(UINT)_wtoi(b);g_itemSlots[i].enabled=g_itemSlots[i].itemId!=0&&g_itemSlots[i].hotkey!=0;}return true;}
