// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <commctrl.h>
#include <atomic>

#include <CKPE.PluginAPI.PluginAPI.h>

#include <ScrollBar/ScrollBarPaint.h>
#include <Subclass/ListViewGridSubclass.h>
#include <Subclass/ScrollBarPaintSubclass.h>
#include <Iat/IatPatch.h>

static decltype(&::SetScrollInfo) g_realSetScrollInfo = ::SetScrollInfo;
static std::atomic<int> g_iatHookCallCount{ 0 };
static std::atomic<int> g_iatHookInterceptCount{ 0 };

// ListView subclass id=1, ScrollBarPaint subclass id=2 — must match StyleDispatcher installs.
static constexpr UINT_PTR kListViewSubclassId     = 1;
static constexpr UINT_PTR kScrollBarPaintSubclassId = 2;

static BOOL WINAPI HookedSetScrollInfo(HWND hwnd, int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw) noexcept(true)
{
    int total = ++g_iatHookCallCount;
    if (total == 1)
        PluginAPI::_MESSAGE("[WineCompat] HookedSetScrollInfo: first call (IAT patch active)");

    if (bRedraw)
    {
        DWORD_PTR dummy = 0;
        if (GetWindowSubclass(hwnd, ScrollBarPaintSubclass, kScrollBarPaintSubclassId, &dummy) ||
            GetWindowSubclass(hwnd, WineListViewGridSubclass, kListViewSubclassId, &dummy))
        {
            int intercepted = ++g_iatHookInterceptCount;
            if (intercepted == 1)
                PluginAPI::_MESSAGE("[WineCompat] HookedSetScrollInfo: first intercept (suppressing fRedraw)");
            BOOL r = g_realSetScrollInfo(hwnd, nBar, lpsi, FALSE);
            HDC hdc = GetWindowDC(hwnd);
            if (hdc) { DrawNcScrollBars(hwnd, hdc); ReleaseDC(hwnd, hdc); }
            return r;
        }
    }
    return g_realSetScrollInfo(hwnd, nBar, lpsi, bRedraw);
}

void PatchComctl32ImportTable() noexcept(true)
{
    HMODULE hComctl32 = GetModuleHandleW(L"comctl32.dll");
    if (!hComctl32) return;

    auto target = reinterpret_cast<ULONG_PTR>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetScrollInfo"));
    if (!target) return;

    auto* pDos = reinterpret_cast<PIMAGE_DOS_HEADER>(hComctl32);
    auto* pNt  = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<BYTE*>(hComctl32) + pDos->e_lfanew);
    auto& impDir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto* pDesc  = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<BYTE*>(hComctl32) + impDir.VirtualAddress);

    for (; pDesc->Name; pDesc++)
    {
        auto* pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            reinterpret_cast<BYTE*>(hComctl32) + pDesc->FirstThunk);
        for (; pThunk->u1.Function; pThunk++)
        {
            if (pThunk->u1.Function != target) continue;
            DWORD oldProt = 0;
            VirtualProtect(&pThunk->u1.Function, sizeof(ULONG_PTR), PAGE_READWRITE, &oldProt);
            g_realSetScrollInfo = reinterpret_cast<decltype(g_realSetScrollInfo)>(target);
            pThunk->u1.Function = reinterpret_cast<ULONG_PTR>(HookedSetScrollInfo);
            VirtualProtect(&pThunk->u1.Function, sizeof(ULONG_PTR), oldProt, &oldProt);
            PluginAPI::_MESSAGE("[WineCompat] Patched comctl32 SetScrollInfo IAT");
            return;
        }
    }
    PluginAPI::_WARNING("[WineCompat] SetScrollInfo IAT entry not found in comctl32");
}
