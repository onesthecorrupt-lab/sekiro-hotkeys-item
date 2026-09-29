#pragma once
#include <windows.h>
struct ItemSlot { int itemId=0; UINT hotkey=0; bool enabled=false; };
extern ItemSlot g_itemSlots[48];
bool LoadItemHotkeyConfig(const wchar_t* path);
