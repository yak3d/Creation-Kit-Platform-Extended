// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UIPushButton.h>
#include <CKPE.Graphics.h>

#include <Subclass/ButtonSubclass.h>

using namespace CKPE;
using namespace CKPE::Common;

struct ButtonState { bool hot = false; bool pressed = false; };

static LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    [[maybe_unused]] UINT_PTR uIdSubclass, DWORD_PTR dwRefData) noexcept(true)
{
    auto* state = reinterpret_cast<ButtonState*>(dwRefData);

    switch (uMsg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, ButtonSubclassProc, 0);
        delete state;
        break;

    case WM_MOUSEMOVE:
        if (!state->hot)
        {
            state->hot = true;
            InvalidateRect(hWnd, nullptr, FALSE);
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
        }
        break;

    case WM_MOUSELEAVE:
        state->hot = false;
        state->pressed = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        break;

    case WM_LBUTTONDOWN:
        state->pressed = true;
        InvalidateRect(hWnd, nullptr, FALSE);
        break;

    case WM_LBUTTONUP:
        state->pressed = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);
        Canvas canvas(ps.hdc);

        DWORD style = static_cast<DWORD>(GetWindowLongW(hWnd, GWL_STYLE));
        bool disabled = (style & WS_DISABLED) != 0;

        if (disabled)
            UI::PushButton::Render::DrawPushButton_Disabled(canvas, &rc);
        else if (state->pressed)
            UI::PushButton::Render::DrawPushButton_Pressed(canvas, &rc);
        else if (state->hot)
            UI::PushButton::Render::DrawPushButton_Hot(canvas, &rc);
        else
            UI::PushButton::Render::DrawPushButton_Normal(canvas, &rc);

        HICON hIcon = reinterpret_cast<HICON>(SendMessage(hWnd, BM_GETIMAGE, IMAGE_ICON, 0));
        if (hIcon)
        {
            ICONINFO ii;
            if (GetIconInfo(hIcon, &ii))
            {
                BITMAP bm = {};
                GetObject(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);
                int x = (rc.right  - rc.left - bm.bmWidth)  / 2;
                int y = (rc.bottom - rc.top  - bm.bmHeight) / 2;
                DrawIconEx(ps.hdc, x, y, hIcon, bm.bmWidth, bm.bmHeight, 0, nullptr, DI_NORMAL);
                if (ii.hbmColor) DeleteObject(ii.hbmColor);
                if (ii.hbmMask)  DeleteObject(ii.hbmMask);
            }
        }
        else
        {
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
                rcText.Inflate(-2, -2);
                canvas.TextRect(rcText, text,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                canvas.TransparentMode = false;
            }
        }

        if (GetFocus() == hWnd)
        {
            RECT rcFocus = rc;
            InflateRect(&rcFocus, -3, -3);
            DrawFocusRect(ps.hdc, &rcFocus);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void ButtonSubclass_Attach(HWND hWnd) noexcept(true)
{
    SetWindowTheme(hWnd, L"", L"");
    SetWindowSubclass(hWnd, ButtonSubclassProc, 0,
        reinterpret_cast<DWORD_PTR>(new ButtonState{}));
}

bool ButtonSubclass_IsAttached(HWND hWnd) noexcept(true)
{
    DWORD_PTR dummy = 0;
    return GetWindowSubclass(hWnd, ButtonSubclassProc, 0, &dummy);
}
