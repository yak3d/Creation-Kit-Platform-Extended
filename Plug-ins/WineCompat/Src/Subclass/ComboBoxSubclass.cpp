// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UIComboBox.h>
#include <CKPE.Common.UIPushButton.h>
#include <CKPE.Graphics.h>

#include <Subclass/ComboBoxSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT CALLBACK ComboBoxSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    [[maybe_unused]] UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if ((uMsg == WM_SETFOCUS) || (uMsg == WM_KILLFOCUS))
        InvalidateRect(hWnd, nullptr, FALSE);

    if (uMsg == WM_PAINT)
    {
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

        COMBOBOXINFO cbi = { sizeof(cbi) };
        if (!GetComboBoxInfo(hWnd, &cbi))
            return result;

        HDC hdc = GetDC(hWnd);
        if (!hdc)
            return result;

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        Canvas canvas(hdc);

        UI::PushButton::Render::DrawPushButton_Normal(canvas, &rcClient);
        UI::ComboBox::Render::DrawArrow_Normal(canvas, &cbi.rcButton);

        LRESULT idx = SendMessage(hWnd, CB_GETCURSEL, 0, 0);
        if (idx != CB_ERR)
        {
            LRESULT cch = SendMessageA(hWnd, CB_GETLBTEXTLEN, static_cast<WPARAM>(idx), 0);
            if (cch != CB_ERR && cch > 0)
            {
                char text[512] = {};
                SendMessageA(hWnd, CB_GETLBTEXT, static_cast<WPARAM>(idx),
                    reinterpret_cast<LPARAM>(text));

                auto f = UI::ThemeData::GetSingleton()->ThemeFont;
                canvas.Font.Assign(*f);
                canvas.TransparentMode = true;
                canvas.ColorText = UI::GetThemeSysColor(UI::ThemeColor_Text_4);

                CRECT rcText = cbi.rcItem;
                rcText.Inflate(-2, -1);
                canvas.TextRect(rcText, text,
                    DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);
                canvas.TransparentMode = false;
            }
        }

        ReleaseDC(hWnd, hdc);
        return result;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
