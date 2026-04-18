// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Graphics.h>

#include <ScrollBar/ScrollBarPaint.h>
#include <Plugin/WineCompatMessages.h>
#include <Subclass/EditSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT CALLBACK EditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    [[maybe_unused]] UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, EditSubclass, 0);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_SETFOCUS || uMsg == WM_KILLFOCUS)
    {
        LRESULT r = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        return r;
    }

    if (uMsg == WM_NCPAINT ||
        uMsg == WM_NCMOUSEMOVE || uMsg == WM_NCMOUSELEAVE)
    {
        HDC hdc = GetWindowDC(hWnd);
        if (hdc)
        {
            if (uMsg == WM_NCPAINT)
            {
                RECT rc = {};
                GetWindowRect(hWnd, &rc);
                OffsetRect(&rc, -rc.left, -rc.top);

                CRECT crc = rc;
                Canvas canvas(hdc);

                if (GetFocus() == hWnd)
                    canvas.Frame(crc, UI::GetThemeSysColor(UI::ThemeColor_Divider_Highlighter_Pressed));
                else
                    canvas.Frame(crc, UI::GetThemeSysColor(UI::ThemeColor_Divider_Highlighter_Gradient_End));

                crc.Inflate(-1, -1);
                canvas.Frame(crc, UI::GetThemeSysColor(UI::ThemeColor_Divider_Color));
            }

            DrawNcScrollBars(hWnd, hdc);
            ReleaseDC(hWnd, hdc);
        }
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
        if (hdc)
        {
            RECT rc = {};
            GetWindowRect(hWnd, &rc);
            OffsetRect(&rc, -rc.left, -rc.top);
            CRECT crc = rc;
            Canvas canvas(hdc);
            if (GetFocus() == hWnd)
                canvas.Frame(crc, UI::GetThemeSysColor(UI::ThemeColor_Divider_Highlighter_Pressed));
            else
                canvas.Frame(crc, UI::GetThemeSysColor(UI::ThemeColor_Divider_Highlighter_Gradient_End));
            crc.Inflate(-1, -1);
            canvas.Frame(crc, UI::GetThemeSysColor(UI::ThemeColor_Divider_Color));
            DrawNcScrollBars(hWnd, hdc);
            ReleaseDC(hWnd, hdc);
        }
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
        HDC hdc = GetWindowDC(hWnd);
        if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
        return result;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
