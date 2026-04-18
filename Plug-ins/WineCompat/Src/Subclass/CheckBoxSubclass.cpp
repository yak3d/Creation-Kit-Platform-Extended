// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UICheckBox.h>
#include <CKPE.Common.UIPushButton.h>
#include <CKPE.Graphics.h>

#include <Subclass/CheckBoxSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT CALLBACK CheckBoxSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    [[maybe_unused]] UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, CheckBoxSubclass, 0);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);
        Canvas canvas(ps.hdc);

        canvas.Fill(rc, UI::GetThemeSysColor(UI::ThemeColor_Default));

        DWORD style    = static_cast<DWORD>(GetWindowLongW(hWnd, GWL_STYLE));
        bool  disabled = (style & WS_DISABLED) != 0;

        int boxW = GetSystemMetrics(SM_CXMENUCHECK);
        int boxH = GetSystemMetrics(SM_CYMENUCHECK);
        int boxY = ((rc.bottom - rc.top) - boxH) / 2;
        RECT rcBox = { rc.left, boxY, rc.left + boxW, boxY + boxH };

        if (disabled)
            UI::PushButton::Render::DrawPushButton_Disabled(canvas, &rcBox);
        else
            UI::PushButton::Render::DrawPushButton_Normal(canvas, &rcBox);

        LRESULT checkState = SendMessage(hWnd, BM_GETCHECK, 0, 0);
        if (checkState == BST_CHECKED)
        {
            if (disabled) UI::CheckBox::Render::DrawCheck_Disabled(canvas, &rcBox);
            else          UI::CheckBox::Render::DrawCheck_Normal(canvas, &rcBox);
        }
        else if (checkState == BST_INDETERMINATE)
        {
            if (disabled) UI::CheckBox::Render::DrawIdeterminate_Disabled(canvas, &rcBox);
            else          UI::CheckBox::Render::DrawIdeterminate_Normal(canvas, &rcBox);
        }

        char text[256] = {};
        GetWindowTextA(hWnd, text, ARRAYSIZE(text));
        if (text[0])
        {
            auto f = UI::ThemeData::GetSingleton()->ThemeFont;
            canvas.Font.Assign(*f);
            canvas.TransparentMode = true;
            canvas.ColorText = disabled
                ? UI::GetThemeSysColor(UI::ThemeColor_Text_1)
                : UI::GetThemeSysColor(UI::ThemeColor_Text_4);
            CRECT rcText = rc;
            rcText.Left += boxW + 4;
            canvas.TextRect(rcText, text,
                DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            canvas.TransparentMode = false;
        }

        if (GetFocus() == hWnd)
        {
            RECT rcFocus = { rc.left + boxW + 2, rc.top + 1, rc.right - 1, rc.bottom - 1 };
            DrawFocusRect(ps.hdc, &rcFocus);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
