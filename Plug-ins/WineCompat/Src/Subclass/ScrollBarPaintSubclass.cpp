// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <atomic>

#include <CKPE.PluginAPI.PluginAPI.h>
#include <CKPE.Common.UIVarCommon.h>

#include <ScrollBar/ScrollBarPaint.h>
#include <Plugin/WineCompatMessages.h>
#include <Subclass/ScrollBarPaintSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT CALLBACK ScrollBarPaintSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, ScrollBarPaintSubclass, uIdSubclass);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_ERASEBKGND)
    {
        static std::atomic<int> s_tvEraseCount{ 0 };
        int n = ++s_tvEraseCount;
        if (n == 1)
            PluginAPI::_MESSAGE_EX("[WineCompat] TreeView {:08X}: first WM_ERASEBKGND", reinterpret_cast<uintptr_t>(hWnd));
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc = {}; GetClientRect(hWnd, &rc);
        COLORREF bk = TreeView_GetBkColor(hWnd);
        if (bk == CLR_NONE || bk == static_cast<COLORREF>(-1))
            bk = UI::GetThemeSysColor(UI::ThemeColor_TreeView_Color);
        HBRUSH br = ::CreateSolidBrush(bk);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        return 1;
    }

    if (uMsg == WM_VSCROLL || uMsg == WM_HSCROLL ||
        uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEHWHEEL)
    {
        LockWindowUpdate(hWnd);
        LRESULT r = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        LockWindowUpdate(nullptr);
        return r;
    }

    if (uMsg == WM_PAINT || uMsg == WM_NCPAINT ||
        uMsg == WM_NCMOUSEMOVE || uMsg == WM_NCMOUSELEAVE)
    {
        if (uMsg == WM_PAINT)
            DefSubclassProc(hWnd, uMsg, wParam, lParam);
        HDC hdc = GetWindowDC(hWnd);
        if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
        if (uMsg == WM_NCMOUSEMOVE || uMsg == WM_NCMOUSELEAVE)
            PostMessage(hWnd, WM_WINECOMPAT_SCROLLDRAW, 0, 0);
        return 0;
    }

    if (uMsg == WM_NCLBUTTONDOWN)
    {
        if (TryHandleScrollBarClick(hWnd, wParam, lParam))
            return 0;
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        HDC hdc = GetWindowDC(hWnd);
        if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
        PostMessage(hWnd, WM_WINECOMPAT_SCROLLDRAW, 0, 0);
        return result;
    }

    if (uMsg == WM_WINECOMPAT_SCROLLDRAW)
    {
        HDC hdc = GetWindowDC(hWnd);
        if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
        return 0;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
