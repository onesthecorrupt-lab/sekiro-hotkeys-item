#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include "config.h"

static volatile bool g_running = true;
static HANDLE g_thread = nullptr;
static wchar_t g_logPath[MAX_PATH]{};

static void Log(const wchar_t* fmt, ...)
{
    wchar_t line[1024]{};
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf_s(line, (sizeof(line) / sizeof(line[0])), _TRUNCATE, fmt, ap);
    va_end(ap);

    OutputDebugStringW(line);

    FILE* f = nullptr;
    if (_wfopen_s(&f, g_logPath, L"a, ccs=UTF-8") == 0 && f)
    {
        fwprintf(f, L"%s\n", line);
        fclose(f);
    }
}

static uintptr_t ReadPtr(uintptr_t address)
{
    __try
    {
        return *reinterpret_cast<uintptr_t*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

static uint32_t ReadU32(uintptr_t address)
{
    __try
    {
        return *reinterpret_cast<uint32_t*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

static bool LooksLikeGoodsId(uint32_t v)
{
    return (v & 0xFF000000u) == 0xB0000000u;
}

static void ScanEquipmentRegion(uintptr_t playerGameData)
{
    // Sekiro 1.6:
    // PlayerGameData + 0x518 = EquipGameData
    // PlayerGameData + 0x5B0 = EquipInventoryData
    //
    // We inspect the 0x98-byte region immediately before EquipInventoryData.
    // This is where the undocumented EquipGameData/equipped-item state lives.
    const uintptr_t equipGameData = playerGameData + 0x518;
    const uintptr_t begin = equipGameData;
    const uintptr_t end = playerGameData + 0x5B0;

    Log(L"[ItemHotkeys] EquipGameData=%p  scan=[%p,%p)",
        reinterpret_cast<void*>(equipGameData),
        reinterpret_cast<void*>(begin),
        reinterpret_cast<void*>(end));

    for (uintptr_t p = begin; p < end; p += 4)
    {
        const uint32_t v = ReadU32(p);
        if (LooksLikeGoodsId(v))
        {
            Log(L"[ItemHotkeys] Goods candidate +0x%03X = 0x%08X",
                static_cast<unsigned>(p - playerGameData), v);
        }
    }
}

static void WatchEquipmentRegion(uintptr_t playerGameData)
{
    constexpr size_t WORDS = 0x98 / 4;
    uint32_t previous[WORDS]{};

    const uintptr_t begin = playerGameData + 0x518;

    for (size_t i = 0; i < WORDS; ++i)
        previous[i] = ReadU32(begin + i * 4);

    Log(L"[ItemHotkeys] Watching EquipGameData. Press X/C in-game to switch Quick Item.");

    while (g_running)
    {
        Sleep(100);

        const uintptr_t pgd = ReadPtr(0x143D5AAC0);
        if (!pgd)
            continue;

        const uintptr_t currentBegin = pgd + 0x518;

        for (size_t i = 0; i < WORDS; ++i)
        {
            const uint32_t v = ReadU32(currentBegin + i * 4);
            if (v != previous[i])
            {
                // Log all changes in the equipment block. Goods-looking values
                // are especially useful for identifying the quick-item field.
                Log(L"[ItemHotkeys] CHANGE +0x%03X: 0x%08X -> 0x%08X%s",
                    static_cast<unsigned>(0x518 + i * 4),
                    previous[i],
                    v,
                    LooksLikeGoodsId(v) ? L"  <Goods-like>" : L"");

                previous[i] = v;
            }
        }
    }
}

static DWORD WINAPI MainThread(LPVOID)
{
    wchar_t dllPath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, dllPath, MAX_PATH);

    wchar_t* slash = wcsrchr(dllPath, L'\\');
    if (slash)
        *(slash + 1) = L'\\0';

    wcscpy_s(g_logPath, dllPath);
    wcscat_s(g_logPath, L"itemhotkeys_scan.log");

    Log(L"[ItemHotkeys] Scanner build started.");

    LoadItemHotkeyConfig(L"itemhotkeys.ini");

    uintptr_t pgd = 0;
    for (int i = 0; i < 120 && g_running; ++i)
    {
        pgd = ReadPtr(0x143D5AAC0);
        if (pgd)
            break;
        Sleep(250);
    }

    if (!pgd)
    {
        Log(L"[ItemHotkeys] Could not resolve PlayerGameData.");
        return 0;
    }

    Log(L"[ItemHotkeys] PlayerGameData=%p", reinterpret_cast<void*>(pgd));
    ScanEquipmentRegion(pgd);
    WatchEquipmentRegion(pgd);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        g_running = true;
        g_thread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        g_running = false;
    }

    return TRUE;
}
