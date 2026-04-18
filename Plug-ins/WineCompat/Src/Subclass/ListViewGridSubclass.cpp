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
#include <Subclass/ListViewGridSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT CALLBACK WineListViewGridSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, WineListViewGridSubclass, uIdSubclass);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == LVM_SETEXTENDEDLISTVIEWSTYLE)
    {
        wParam &= ~static_cast<WPARAM>(LVS_EX_GRIDLINES);
        lParam &= ~static_cast<LPARAM>(LVS_EX_GRIDLINES);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_ERASEBKGND)
    {
        static std::atomic<int> s_lvEraseCount{ 0 };
        int n = ++s_lvEraseCount;
        if (n == 1)
            PluginAPI::_MESSAGE_EX("[WineCompat] ListView {:08X}: first WM_ERASEBKGND", reinterpret_cast<uintptr_t>(hWnd));
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc = {}; GetClientRect(hWnd, &rc);
        COLORREF bk = ListView_GetBkColor(hWnd);
        if (bk == CLR_NONE || bk == static_cast<COLORREF>(-1))
            bk = UI::GetThemeSysColor(UI::ThemeColor_ListView_Color);
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

    if (uMsg == WM_NCPAINT ||
        uMsg == WM_NCMOUSEMOVE || uMsg == WM_NCMOUSELEAVE)
    {
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

    if (uMsg == WM_PAINT)
    {
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

        HDC hdcWnd = GetWindowDC(hWnd);
        if (hdcWnd)
        {
            DrawNcScrollBars(hWnd, hdcWnd);
            ReleaseDC(hWnd, hdcWnd);
        }

        HDC hdc = GetDC(hWnd);
        if (hdc)
        {
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            COLORREF lineColor = UI::GetThemeSysColor(UI::ThemeColor_Default);
            HPEN hPen = CreatePen(PS_SOLID, 1, lineColor);
            HGDIOBJ hOldPen = SelectObject(hdc, hPen);

            int count  = ListView_GetItemCount(hWnd);
            int first  = ListView_GetTopIndex(hWnd);
            int perPg  = ListView_GetCountPerPage(hWnd);

            int bodyTop = rcClient.top;
            if (count > 0 && first < count)
            {
                RECT rcFirst;
                if (ListView_GetItemRect(hWnd, first, &rcFirst, LVIR_BOUNDS))
                    bodyTop = rcFirst.top;
            }

            for (int i = first; i <= min(count - 1, first + perPg); i++)
            {
                RECT rcItem;
                if (ListView_GetItemRect(hWnd, i, &rcItem, LVIR_BOUNDS))
                {
                    MoveToEx(hdc, rcClient.left, rcItem.bottom - 1, nullptr);
                    LineTo(hdc, rcClient.right, rcItem.bottom - 1);
                }
            }

            HWND hHeader = ListView_GetHeader(hWnd);
            if (hHeader)
            {
                int colCount = Header_GetItemCount(hHeader);
                for (int i = 0; i < colCount - 1; i++)
                {
                    RECT rcCol;
                    if (Header_GetItemRect(hHeader, i, &rcCol))
                    {
                        MoveToEx(hdc, rcCol.right - 1, bodyTop, nullptr);
                        LineTo(hdc, rcCol.right - 1, rcClient.bottom);
                    }
                }
            }

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            ReleaseDC(hWnd, hdc);
        }
        return result;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
