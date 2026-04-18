// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UIHeader.h>
#include <CKPE.Graphics.h>

#include <Subclass/HeaderSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT CALLBACK HeaderSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    [[maybe_unused]] UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, HeaderSubclass, 0);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_ERASEBKGND)
        return 1;

    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        RECT rcClient = {};
        GetClientRect(hWnd, &rcClient);

        Canvas canvas(ps.hdc);
        canvas.Fill(rcClient, UI::GetThemeSysColor(UI::ThemeColor_Default_Gradient_Start));

        HFONT hFont = static_cast<HFONT>(UI::ThemeData::GetSingleton()->ThemeFont->Handle);
        HGDIOBJ hOldFont = SelectObject(ps.hdc, hFont);
        SetBkMode(ps.hdc, TRANSPARENT);
        SetTextColor(ps.hdc, UI::GetThemeSysColor(UI::ThemeColor_Text_4));

        int count = Header_GetItemCount(hWnd);
        for (int i = 0; i < count; i++)
        {
            RECT rcItem = {};
            if (!Header_GetItemRect(hWnd, i, &rcItem))
                continue;

            UI::Header::Render::DrawBack_Normal(canvas, &rcItem);

            wchar_t text[256] = {};
            HDITEMW hdi = {};
            hdi.mask       = HDI_TEXT | HDI_FORMAT;
            hdi.pszText    = text;
            hdi.cchTextMax = static_cast<int>(ARRAYSIZE(text));
            Header_GetItem(hWnd, i, &hdi);

            RECT rcText = rcItem;
            rcText.left += 6; rcText.right -= 6;
            UINT dtFlags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
            if      (hdi.fmt & HDF_RIGHT)  dtFlags |= DT_RIGHT;
            else if (hdi.fmt & HDF_CENTER) dtFlags |= DT_CENTER;
            else                           dtFlags |= DT_LEFT;
            DrawTextW(ps.hdc, text, -1, &rcText, dtFlags);

            UI::Header::Render::DrawDiv(canvas, &rcItem);
        }

        SelectObject(ps.hdc, hOldFont);
        EndPaint(hWnd, &ps);
        return 0;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
