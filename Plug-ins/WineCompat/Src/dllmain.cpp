// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <uxtheme.h>

#include <CKPE.PathUtils.h>
#include <CKPE.PluginAPI.PluginAPI.h>
#include <CKPE.Common.UIListView.h>
#include <CKPE.Common.UITreeView.h>
#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UIComboBox.h>
#include <CKPE.Common.UICheckBox.h>
#include <CKPE.Common.UIHeader.h>
#include <CKPE.Common.UIPushButton.h>
#include <CKPE.Common.UIScrollBar.h>
#include <CKPE.Graphics.h>

using namespace CKPE;
using namespace CKPE::Common;

static void DrawNcScrollBars(HWND hWnd, HDC hdc) noexcept(true);
static bool TryHandleScrollBarClick(HWND hWnd, WPARAM wParam, LPARAM lParam) noexcept(true);

static constexpr UINT     WM_WINECOMPAT_RESTYLE    = WM_APP + 0x201;
static constexpr UINT     WM_WINECOMPAT_SCROLLDRAW = WM_APP + 0x202;

// Set to the HWND being thumb-dragged. WM_NCPAINT skips DefSubclassProc while
// this is set, preventing Wine's SetScrollInfo(redraw=TRUE) from firing a
// system-colored scrollbar draw mid-drag.
static HWND s_thumbDragHwnd = nullptr;

// ---------------------------------------------------------------------------
// Button subclass — overdraw the face with the dark theme
// ---------------------------------------------------------------------------

struct ButtonState { bool hot = false; bool pressed = false; };

static LRESULT CALLBACK ButtonSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    [[maybe_unused]] UINT_PTR uIdSubclass, DWORD_PTR dwRefData) noexcept(true)
{
    auto* state = reinterpret_cast<ButtonState*>(dwRefData);

    switch (uMsg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, ButtonSubclass, 0);
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

        // Draw icon if button has one
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
            // Draw button text
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

        // Draw focus rect if button has keyboard focus
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

// ---------------------------------------------------------------------------
// CheckBox subclass — full WM_PAINT override with dark theme box + label
// ---------------------------------------------------------------------------

static LRESULT CALLBACK CheckBoxSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
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

// ---------------------------------------------------------------------------
// ComboBox subclass — overdraw the face with the dark theme after DefSubclass
// ---------------------------------------------------------------------------

static LRESULT CALLBACK ComboBoxSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
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

// ---------------------------------------------------------------------------
// RichEdit dark theme — set background + text color via messages
// ---------------------------------------------------------------------------

static void ApplyDarkThemeToRichEdit(HWND hWnd) noexcept(true)
{
    SendMessageA(hWnd, EM_SETBKGNDCOLOR, FALSE,
        static_cast<LPARAM>(UI::GetThemeSysColor(UI::ThemeColor_Edit_Color)));

    CHARFORMAT2A fmt = {};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_COLOR | CFM_CHARSET;
    fmt.crTextColor = UI::GetThemeSysColor(UI::ThemeColor_Text_4);
    fmt.bCharSet    = DEFAULT_CHARSET;
    SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_ALL,   reinterpret_cast<LPARAM>(&fmt));
    SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&fmt));
}

// ---------------------------------------------------------------------------
// Edit subclass — overdraw the non-client border with dark theme colors.
// SetWindowTheme("","") fixes the client background (WM_CTLCOLOREDIT works)
// but leaves a classic 3D white frame in the non-client area under Wine.
// ---------------------------------------------------------------------------

static LRESULT CALLBACK EditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
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
        if (uMsg != WM_NCPAINT || hWnd != s_thumbDragHwnd)
            DefSubclassProc(hWnd, uMsg, wParam, lParam);

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

// ---------------------------------------------------------------------------
// WineListViewGridSubclass (id=1) — strips LVS_EX_GRIDLINES (Wine draws it
// with COLOR_3DFACE → wrong color) and redraws grid lines in dark theme.
// ---------------------------------------------------------------------------

static LRESULT CALLBACK WineListViewGridSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
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

    if (uMsg == WM_NCPAINT ||
        uMsg == WM_NCMOUSEMOVE || uMsg == WM_NCMOUSELEAVE)
    {
        if (uMsg != WM_NCPAINT || hWnd != s_thumbDragHwnd)
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

    if (uMsg == WM_PAINT)
    {
        LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

        // Overdraw scrollbars via window DC after all painting finishes.
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

            // Horizontal row separators — find top of first visible item so we
            // don't draw into the header area.
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

            // Vertical column separators — positions come from the header.
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

// ---------------------------------------------------------------------------
// Header subclass — full WM_PAINT override for ListView column headers.
// Under Wine the uxtheme DrawThemeBackground hook fails, leaving the header
// in the system light-grey style.
// ---------------------------------------------------------------------------

static LRESULT CALLBACK HeaderSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
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

static HIMAGELIST CreateDarkCheckboxImageList(HWND hTreeView) noexcept(true);

// ---------------------------------------------------------------------------
// TryHandleScrollBarClick — intercept arrow-button and track clicks on
// embedded scrollbars so Wine's tracking loop (which calls DrawScrollBar
// directly, bypassing our subclass) is never entered for those regions.
// Thumb-drag clicks return false and fall through to DefSubclassProc.
// ---------------------------------------------------------------------------

static bool TryHandleScrollBarClick(HWND hWnd, WPARAM wParam, LPARAM lParam) noexcept(true)
{
    // wParam in WM_NCLBUTTONDOWN is the hit-test code from DefWindowProc.
    bool vert = (wParam == HTVSCROLL);
    bool horz = (wParam == HTHSCROLL);
    if (!vert && !horz) return false;

    RECT wndRC = {}; GetWindowRect(hWnd, &wndRC);
    int wW = wndRC.right - wndRC.left, wH = wndRC.bottom - wndRC.top;
    POINT cp = { 0, 0 }; ClientToScreen(hWnd, &cp);
    RECT clientRC = {}; GetClientRect(hWnd, &clientRC);
    int bL = cp.x - wndRC.left, bT = cp.y - wndRC.top;
    int bR = wW - bL - clientRC.right, bB = wH - bT - clientRC.bottom;

    int vSBW   = GetSystemMetrics(SM_CXVSCROLL);
    int arrowV = GetSystemMetrics(SM_CYVSCROLL);
    int arrowH = GetSystemMetrics(SM_CXHSCROLL);
    int arrowLen = vert ? arrowV : arrowH;

    RECT rcSB = vert
        ? RECT{ wW - bR, bT, wW - bR + vSBW, wH - bB }
        : RECT{ bL, wH - bB, wW - bR, wH - bB + GetSystemMetrics(SM_CYHSCROLL) };

    int trackStart = vert ? rcSB.top    + arrowLen : rcSB.left  + arrowLen;
    int trackEnd   = vert ? rcSB.bottom - arrowLen : rcSB.right - arrowLen;
    int clickPos   = vert
        ? (GET_Y_LPARAM(lParam) - wndRC.top)
        : (GET_X_LPARAM(lParam) - wndRC.left);

    bool inBtnA  = (clickPos < trackStart);
    bool inBtnB  = (clickPos >= trackEnd);
    bool inTrack = (!inBtnA && !inBtnB);

    if (inTrack)
    {
        // Determine thumb bounds to decide page-up/page-down vs drag.
        SCROLLINFO si = { sizeof(si), SIF_ALL };
        if (!GetScrollInfo(hWnd, vert ? SB_VERT : SB_HORZ, &si)) return false;

        int trackSz  = trackEnd - trackStart;
        int minThumb = vert ? GetSystemMetrics(SM_CYVTHUMB) : GetSystemMetrics(SM_CXHTHUMB);
        int range    = si.nMax - si.nMin + 1;
        int page     = (int)si.nPage;
        int thumbSz  = (range > 0 && page > 0 && page < range)
                       ? max(minThumb, trackSz * page / range) : minThumb;
        thumbSz = min(thumbSz, trackSz);
        int travel   = max(1, range - page);
        int thumbOff = (travel > 0)
                       ? (int)((long long)(si.nPos - si.nMin) * (trackSz - thumbSz) / travel) : 0;
        int thumbStart = trackStart + thumbOff;
        int thumbEnd   = thumbStart + thumbSz;

        if (clickPos >= thumbStart && clickPos < thumbEnd)
        {
            // Thumb drag: run our own tracking loop so Wine's DrawScrollBar
            // is never called during the drag.
            int anchorOff = clickPos - thumbStart;
            int travelPx  = max(1, trackSz - thumbSz);
            int lastPos   = si.nPos;

            s_thumbDragHwnd = hWnd;
            SetCapture(hWnd);

            MSG msg = {};
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                if (msg.message == WM_LBUTTONUP || msg.message == WM_CANCELMODE)
                {
                    SendMessage(hWnd, vert ? WM_VSCROLL : WM_HSCROLL,
                        MAKEWPARAM(SB_THUMBPOSITION, (WORD)lastPos), 0);
                    SendMessage(hWnd, vert ? WM_VSCROLL : WM_HSCROLL,
                        MAKEWPARAM(SB_ENDSCROLL, 0), 0);
                    break;
                }
                if (msg.message == WM_MOUSEMOVE)
                {
                    POINT cur = {}; GetCursorPos(&cur);
                    RECT wr = {}; GetWindowRect(hWnd, &wr);
                    int curPos  = vert ? (cur.y - wr.top) : (cur.x - wr.left);
                    int newOff  = max(0, min(trackSz - thumbSz, curPos - anchorOff - trackStart));
                    int newPos  = si.nMin + (int)((long long)newOff * travel / travelPx);
                    newPos = max(si.nMin, min(si.nMax - (int)si.nPage + 1, newPos));
                    lastPos = newPos;

                    // SetScrollPos with bRedraw=FALSE updates the internal
                    // position (so GetScrollInfo returns the right value for
                    // DrawNcScrollBars) without triggering WM_NCPAINT, which
                    // is what causes the Wine light-draw → dark-overdraw flicker.
                    SetScrollPos(hWnd, vert ? SB_VERT : SB_HORZ, newPos, FALSE);
                    SendMessage(hWnd, vert ? WM_VSCROLL : WM_HSCROLL,
                        MAKEWPARAM(SB_THUMBTRACK, (WORD)newPos), 0);

                    HDC hdc = GetWindowDC(hWnd);
                    if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
                    continue; // consume — don't dispatch
                }
                if (msg.message == WM_QUIT)
                {
                    PostQuitMessage((int)msg.wParam);
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (GetCapture() == hWnd)
                ReleaseCapture();
            s_thumbDragHwnd = nullptr;

            HDC hdc = GetWindowDC(hWnd);
            if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
            return true;
        }

        WPARAM code = vert
            ? (WPARAM)(clickPos < thumbStart ? SB_PAGEUP   : SB_PAGEDOWN)
            : (WPARAM)(clickPos < thumbStart ? SB_PAGELEFT : SB_PAGERIGHT);
        SendMessage(hWnd, vert ? WM_VSCROLL : WM_HSCROLL, MAKEWPARAM(code, 0), 0);
    }
    else
    {
        WPARAM code = vert
            ? (WPARAM)(inBtnA ? SB_LINEUP   : SB_LINEDOWN)
            : (WPARAM)(inBtnA ? SB_LINELEFT : SB_LINERIGHT);
        SendMessage(hWnd, vert ? WM_VSCROLL : WM_HSCROLL, MAKEWPARAM(code, 0), 0);
    }

    HDC hdc = GetWindowDC(hWnd);
    if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
    return true;
}

// ---------------------------------------------------------------------------
// DrawScrollArrow — duplicate of the SBP_ARROWBTN PolyPolyline logic in
// ModernTheme's Comctl32DrawThemeBackground, adapted for direct HDC use.
// ---------------------------------------------------------------------------

static void DrawScrollArrow(HDC hdc, RECT rc, bool vert, bool isNeg) noexcept(true)
{
    const int aw = (int)std::ceil(std::abs(rc.right - rc.left) * 0.4f);
    const int ah = (int)std::ceil(std::abs(rc.bottom - rc.top) * 0.35f);

    DWORD counts[6] = { 2, 2, 2, 2, 2, 2 };
    POINT verts[12] =
    {
        { 0,    0 },  { aw/2+1, -ah+2 },
        { 0,   -1 },  { aw/2+1, -ah+1 },
        { 0,   -2 },  { aw/2+1, -ah   },
        { aw-1, 0 },  { aw/2,   -ah+2 },
        { aw-1,-1 },  { aw/2,   -ah+1 },
        { aw-1,-2 },  { aw/2,   -ah+1 },
    };

    for (auto& v : verts)
    {
        if (vert && isNeg)        // UP
        {
            v.x += rc.left + ah - 1;
            v.y += rc.bottom - ah;
        }
        else if (vert)            // DOWN
        {
            v.x += rc.left + ah - 1;
            v.y  = -v.y + rc.top + ah - 1;
        }
        else if (isNeg)           // LEFT
        {
            std::swap(v.x, v.y);
            v.x += rc.right - ah;
            v.y += rc.top + ah - 1;
        }
        else                      // RIGHT
        {
            std::swap(v.x, v.y);
            v.x  = -v.x + rc.left + ah - 1;
            v.y += rc.top + ah - 1;
        }
    }

    HPEN hShadow = CreatePen(PS_SOLID, 1, UI::GetThemeSysColor(UI::ThemeColor_Shape_Shadow));
    HGDIOBJ hPrev = SelectObject(hdc, hShadow);
    PolyPolyline(hdc, verts, counts, 6);
    HPEN hMain = CreatePen(PS_SOLID, 1, UI::GetThemeSysColor(UI::ThemeColor_Shape));
    DeleteObject(SelectObject(hdc, hMain));
    for (auto& v : verts) v.y++;
    PolyPolyline(hdc, verts, counts, 6);
    DeleteObject(SelectObject(hdc, hPrev));
}

// ---------------------------------------------------------------------------
// DrawNcScrollBars — overpaint non-client scrollbars with dark theme after
// DefSubclassProc has drawn the system-colored version.
//
// Does NOT use GetScrollBarInfo: under Wine it always returns
// STATE_SYSTEM_INVISIBLE for embedded scrollbars. Instead, derives the
// scrollbar rectangle from GetWindowRect / GetClientRect / ClientToScreen,
// which gives the exact non-client geometry without accessibility APIs.
// ---------------------------------------------------------------------------

static void DrawNcScrollBars(HWND hWnd, HDC hdc) noexcept(true)
{
    DWORD style = static_cast<DWORD>(GetWindowLongW(hWnd, GWL_STYLE));
    bool hasV = (style & WS_VSCROLL) != 0;
    bool hasH = (style & WS_HSCROLL) != 0;
    if (!hasV && !hasH) return;

    RECT wndRC = {};
    GetWindowRect(hWnd, &wndRC);
    int wW = wndRC.right  - wndRC.left;
    int wH = wndRC.bottom - wndRC.top;

    // Compute non-client border sizes from the client origin in screen space.
    POINT clientPt = { 0, 0 };
    ClientToScreen(hWnd, &clientPt);
    int bL = clientPt.x - wndRC.left;
    int bT = clientPt.y - wndRC.top;

    RECT clientRC = {};
    GetClientRect(hWnd, &clientRC);
    int bR = wW - bL - clientRC.right;
    int bB = wH - bT - clientRC.bottom;

    // Sanity — if geometry is degenerate skip.
    if (bL < 0 || bT < 0 || bR < 0 || bB < 0) return;

    int vSBW = GetSystemMetrics(SM_CXVSCROLL);
    int hSBH = GetSystemMetrics(SM_CYHSCROLL);

    // bR should contain the scrollbar width; bail if it doesn't.
    if (hasV && bR < vSBW) return;
    if (hasH && bB < hSBH) return;

    Canvas canvas(hdc);

    auto paintBar = [&](bool vert)
    {
        // Vertical SB: rightmost vSBW pixels of the right NC strip.
        // Horizontal SB: topmost hSBH pixels of the bottom NC strip.
        RECT rcSB;
        if (vert)
            rcSB = { wW - bR, bT, wW - bR + vSBW, wH - bB };
        else
            rcSB = { bL, wH - bB, wW - bR, wH - bB + hSBH };

        if (rcSB.right <= rcSB.left || rcSB.bottom <= rcSB.top) return;

        int arrowLen = vert ? GetSystemMetrics(SM_CYVSCROLL)
                            : GetSystemMetrics(SM_CXHSCROLL);
        RECT rcBtnA, rcBtnB, rcTrack;
        if (vert)
        {
            rcBtnA  = { rcSB.left, rcSB.top,              rcSB.right, rcSB.top  + arrowLen };
            rcBtnB  = { rcSB.left, rcSB.bottom - arrowLen, rcSB.right, rcSB.bottom };
            rcTrack = { rcSB.left, rcSB.top  + arrowLen,  rcSB.right, rcSB.bottom - arrowLen };
        }
        else
        {
            rcBtnA  = { rcSB.left,             rcSB.top, rcSB.left + arrowLen, rcSB.bottom };
            rcBtnB  = { rcSB.right - arrowLen, rcSB.top, rcSB.right,           rcSB.bottom };
            rcTrack = { rcSB.left + arrowLen,  rcSB.top, rcSB.right - arrowLen, rcSB.bottom };
        }

        // Track background
        if (vert) UI::ScrollBar::Render::DrawBackgroundVert(canvas, &rcTrack);
        else      UI::ScrollBar::Render::DrawBackgroundHorz(canvas, &rcTrack);

        // Thumb — position from GetScrollInfo (works under Wine)
        SCROLLINFO si = { sizeof(si), SIF_ALL };
        if (GetScrollInfo(hWnd, vert ? SB_VERT : SB_HORZ, &si))
        {
            int range   = si.nMax - si.nMin + 1;
            int page    = static_cast<int>(si.nPage);
            int trackSz = vert ? (rcTrack.bottom - rcTrack.top)
                               : (rcTrack.right  - rcTrack.left);
            int minSz   = vert ? GetSystemMetrics(SM_CYVTHUMB)
                               : GetSystemMetrics(SM_CXHTHUMB);
            if (range > 0 && page > 0 && page < range && trackSz > 0)
            {
                int thumbSz  = max(minSz, trackSz * page / range);
                thumbSz      = min(thumbSz, trackSz);
                int travel   = max(1, range - page);
                int off      = static_cast<int>(
                    static_cast<long long>(si.nPos - si.nMin) * (trackSz - thumbSz) / travel);
                RECT rcThumb;
                if (vert)
                    rcThumb = { rcTrack.left, rcTrack.top + off,
                                rcTrack.right, rcTrack.top + off + thumbSz };
                else
                    rcThumb = { rcTrack.left + off, rcTrack.top,
                                rcTrack.left + off + thumbSz, rcTrack.bottom };
                if (vert) UI::ScrollBar::Render::DrawSliderVert_Normal(canvas, &rcThumb);
                else      UI::ScrollBar::Render::DrawSliderHorz_Normal(canvas, &rcThumb);
            }
        }

        // Arrow buttons
        if (vert)
        {
            UI::ScrollBar::Render::DrawBackgroundVert(canvas, &rcBtnA);
            DrawScrollArrow(hdc, rcBtnA, true, true);
            UI::ScrollBar::Render::DrawBackgroundVert(canvas, &rcBtnB);
            DrawScrollArrow(hdc, rcBtnB, true, false);
        }
        else
        {
            UI::ScrollBar::Render::DrawBackgroundHorz(canvas, &rcBtnA);
            DrawScrollArrow(hdc, rcBtnA, false, true);
            UI::ScrollBar::Render::DrawBackgroundHorz(canvas, &rcBtnB);
            DrawScrollArrow(hdc, rcBtnB, false, false);
        }
    };

    if (hasV) paintBar(true);
    if (hasH) paintBar(false);

    // Corner grip when both bars are visible
    if (hasV && hasH)
    {
        RECT rcCorner = { wW - bR, wH - bB, wW - bR + vSBW, wH };
        canvas.Fill(rcCorner, UI::GetThemeSysColor(UI::ThemeColor_Default));
    }
}

// ---------------------------------------------------------------------------
// ScrollBarPaintSubclass — WM_PAINT post-pass that calls DrawNcScrollBars via
// GetWindowDC. WM_NCPAINT alone is insufficient under Wine: the native
// scrollbar code repaints with system colors after WM_NCPAINT returns. Using
// WM_PAINT + GetWindowDC fires last, matching the pattern CKPE.Common uses
// for border overdraw in UIListView / UITreeView.
// ---------------------------------------------------------------------------

static LRESULT CALLBACK ScrollBarPaintSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, ScrollBarPaintSubclass, uIdSubclass);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_PAINT || uMsg == WM_NCPAINT ||
        uMsg == WM_NCMOUSEMOVE || uMsg == WM_NCMOUSELEAVE)
    {
        // During thumb drag, skip DefSubclassProc for WM_NCPAINT — Wine's
        // SetScrollInfo(redraw=TRUE) inside SB_THUMBTRACK fires WM_NCPAINT
        // which would draw system-colored scrollbars causing flicker.
        if (uMsg != WM_NCPAINT || hWnd != s_thumbDragHwnd)
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

// ---------------------------------------------------------------------------
// StyleChildControls — apply Wine overrides to all current children of parent.
// Called both immediately from OnInitDialog and deferred (post-game-init).
// Guards against double-installation so repeated calls are safe.
// ---------------------------------------------------------------------------

static void StyleChildControls(HWND parent) noexcept(true)
{
    EnumChildWindows(parent, [](HWND child, LPARAM) -> BOOL
    {
        wchar_t cls[64] = {};
        GetClassNameW(child, cls, ARRAYSIZE(cls));

        if (_wcsicmp(cls, WC_LISTVIEWW) == 0)
        {
            DWORD_PTR existing = 0;
            if (GetWindowSubclass(child, WineListViewGridSubclass, 1, &existing))
                return TRUE;

            UI::ListView::Initialize(child);
            auto f = UI::ThemeData::GetSingleton()->ThemeFont;
            PostMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(f->Handle), TRUE);

            DWORD exStyle = ListView_GetExtendedListViewStyle(child);
            if (exStyle & LVS_EX_GRIDLINES)
                ListView_SetExtendedListViewStyle(child, exStyle & ~LVS_EX_GRIDLINES);
            SetWindowSubclass(child, WineListViewGridSubclass, 1, 0);

            HWND hHeader = ListView_GetHeader(child);
            if (hHeader)
            {
                SetWindowTheme(hHeader, L"", L"");
                SetWindowSubclass(hHeader, HeaderSubclass, 0, 0);
            }
        }
        else if (_wcsicmp(cls, WC_TREEVIEWW) == 0)
        {
            DWORD_PTR existing = 0;
            if (GetWindowSubclass(child, CKPE::Common::UI::TreeView::TreeViewSubclass, 0, &existing))
                return TRUE;

            UI::TreeView::Initialize(child);
            SetWindowTheme(child, nullptr, nullptr);
            auto f = UI::ThemeData::GetSingleton()->ThemeFont;
            PostMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(f->Handle), TRUE);
            SetWindowSubclass(child, ScrollBarPaintSubclass, 2, 0);

            if (GetWindowLongW(child, GWL_STYLE) & TVS_CHECKBOXES)
            {
                HIMAGELIST hIml = CreateDarkCheckboxImageList(child);
                if (hIml)
                {
                    HIMAGELIST hOld = TreeView_SetImageList(child, hIml, TVSIL_STATE);
                    if (hOld) ImageList_Destroy(hOld);
                }
            }
        }
        else if (_wcsicmp(cls, L"Edit") == 0)
        {
            DWORD_PTR existing = 0;
            if (GetWindowSubclass(child, EditSubclass, 0, &existing))
                return TRUE;
            SetWindowTheme(child, L"", L"");
            SetWindowSubclass(child, EditSubclass, 0, 0);
        }
        else if (_wcsicmp(cls, L"ComboBox") == 0)
        {
            DWORD_PTR existing = 0;
            if (GetWindowSubclass(child, ComboBoxSubclass, 0, &existing))
                return TRUE;
            SetWindowTheme(child, L"", L"");
            SetWindowSubclass(child, ComboBoxSubclass, 0, 0);
        }
        else if (_wcsicmp(cls, L"Button") == 0)
        {
            DWORD btnType = static_cast<DWORD>(GetWindowLongW(child, GWL_STYLE)) & BS_TYPEMASK;
            if (btnType == BS_PUSHBUTTON || btnType == BS_DEFPUSHBUTTON)
            {
                DWORD_PTR existing = 0;
                if (GetWindowSubclass(child, ButtonSubclass, 0, &existing))
                    return TRUE;
                SetWindowTheme(child, L"", L"");
                SetWindowSubclass(child, ButtonSubclass, 0,
                    reinterpret_cast<DWORD_PTR>(new ButtonState{}));
            }
            else if (btnType == BS_CHECKBOX    || btnType == BS_AUTOCHECKBOX ||
                     btnType == BS_3STATE      || btnType == BS_AUTO3STATE)
            {
                DWORD_PTR existing = 0;
                if (GetWindowSubclass(child, CheckBoxSubclass, 0, &existing))
                    return TRUE;
                SetWindowTheme(child, L"", L"");
                SetWindowSubclass(child, CheckBoxSubclass, 0, 0);
            }
        }
        else if (_wcsicmp(cls, L"ListBox") == 0)
        {
            DWORD_PTR existing = 0;
            if (GetWindowSubclass(child, ScrollBarPaintSubclass, 2, &existing))
                return TRUE;
            SetWindowTheme(child, L"", L"");
            SetWindowSubclass(child, ScrollBarPaintSubclass, 2, 0);
        }
        else if (_wcsicmp(cls, MSFTEDIT_CLASS) == 0 ||
                 _wcsicmp(cls, L"RichEdit20A") == 0  ||
                 _wcsicmp(cls, L"RichEdit20W") == 0)
        {
            ApplyDarkThemeToRichEdit(child);
            DWORD_PTR existing = 0;
            if (!GetWindowSubclass(child, ScrollBarPaintSubclass, 2, &existing))
                SetWindowSubclass(child, ScrollBarPaintSubclass, 2, 0);
        }

        return TRUE;
    }, 0);
}

// ---------------------------------------------------------------------------
// DeferredRestyleSubclass — installed permanently on every dialog.
//
// Two responsibilities:
//   1. WM_WINECOMPAT_RESTYLE (posted): runs StyleChildControls, catching
//      controls created dynamically after WM_INITDIALOG.
//   2. WM_NOTIFY / TCN_SELCHANGE: tab switched inside this dialog —
//      posts WM_WINECOMPAT_RESTYLE so we restyle after the game builds
//      the new tab's controls.
//
// Stays installed until WM_NCDESTROY so repeated tab switches all work.
// ---------------------------------------------------------------------------

static constexpr UINT_PTR kDeferredSubclassId = 0xBC01;

static LRESULT CALLBACK DeferredRestyleSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
{
    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hWnd, DeferredRestyleSubclass, uIdSubclass);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_WINECOMPAT_RESTYLE)
    {
        StyleChildControls(hWnd);
        return 0;
    }
    if (uMsg == WM_NOTIFY)
    {
        auto* hdr = reinterpret_cast<NMHDR*>(lParam);
        if (hdr && hdr->code == TCN_SELCHANGE)
            PostMessage(hWnd, WM_WINECOMPAT_RESTYLE, 0, 0);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// OnInitDialog — walk child controls and apply Wine-specific overrides.
// First pass runs immediately (before game's dialog proc); second pass is
// posted so it runs after the game finishes creating dynamic controls.
// ---------------------------------------------------------------------------

static void OnInitDialog(HWND dlg, [[maybe_unused]] void* userdata) noexcept(true)
{
    StyleChildControls(dlg);
    SetWindowSubclass(dlg, DeferredRestyleSubclass, kDeferredSubclassId, 0);
    PostMessage(dlg, WM_WINECOMPAT_RESTYLE, 0, 0);
}

// ---------------------------------------------------------------------------
// Dark checkbox state image list for TreeViews with TVS_CHECKBOXES.
// Wine draws the default state images using GetSysColor(COLOR_WINDOW) → white.
// We replace them with properly themed bitmaps after Initialize.
// ---------------------------------------------------------------------------

static HIMAGELIST CreateDarkCheckboxImageList(HWND hTreeView) noexcept(true)
{
    int cx = 16, cy = 16;
    HIMAGELIST hExisting = TreeView_GetImageList(hTreeView, TVSIL_STATE);
    if (hExisting)
        ImageList_GetIconSize(hExisting, &cx, &cy);

    HIMAGELIST hIml = ImageList_Create(cx, cy, ILC_COLOR32, 3, 0);
    if (!hIml) return nullptr;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    COLORREF bkColor = UI::GetThemeSysColor(UI::ThemeColor_TreeView_Color);
    RECT rc = { 0, 0, cx, cy };

    auto addImage = [&](bool drawBox, bool drawMark)
    {
        HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, cx, cy);
        HGDIOBJ hOld = SelectObject(hdcMem, hBmp);

        HBRUSH hbr = CreateSolidBrush(bkColor);
        FillRect(hdcMem, &rc, hbr);
        DeleteObject(hbr);

        if (drawBox)
        {
            Canvas canvas(hdcMem);
            UI::PushButton::Render::DrawPushButton_Normal(canvas, &rc);
            if (drawMark)
                UI::CheckBox::Render::DrawCheck_Normal(canvas, &rc);
        }

        SelectObject(hdcMem, hOld);
        ImageList_Add(hIml, hBmp, nullptr);
        DeleteObject(hBmp);
    };

    addImage(false, false);  // index 0: blank (state 0 = no checkbox)
    addImage(true,  false);  // index 1: unchecked box
    addImage(true,  true);   // index 2: checked box

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    return hIml;
}

// ---------------------------------------------------------------------------
// TreeView CustomDraw — handle text and selection colors via CDDS_ITEMPOSTPAINT.
// SetWindowTheme("","") is NOT used (it breaks +/- glyphs under Wine).
// Visual styles draw the glyphs correctly; we overdraw text+background after.
// ---------------------------------------------------------------------------

static LRESULT OnTreeViewCustomDraw(HWND wnd,
    LPNMLVCUSTOMDRAW lpcd,
    bool* handled,
    [[maybe_unused]] void* userdata) noexcept(true)
{
    switch (lpcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *handled = true;
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
        *handled = true;
        return CDRF_NOTIFYPOSTPAINT;

    case CDDS_ITEMPOSTPAINT:
    {
        *handled = true;

        HTREEITEM hItem = reinterpret_cast<HTREEITEM>(lpcd->nmcd.dwItemSpec);
        RECT rcLabel = {};
        *reinterpret_cast<HTREEITEM*>(&rcLabel) = hItem;
        if (!SendMessage(wnd, TVM_GETITEMRECT, TRUE, reinterpret_cast<LPARAM>(&rcLabel)))
            return CDRF_DODEFAULT;

        bool selected = (lpcd->nmcd.uItemState & CDIS_SELECTED) != 0;
        COLORREF bkColor = UI::GetThemeSysColor(selected
            ? UI::ThemeColor_SelectedItem_Back
            : UI::ThemeColor_TreeView_Color);
        COLORREF txColor = UI::GetThemeSysColor(selected
            ? UI::ThemeColor_SelectedItem_Text
            : UI::ThemeColor_Text_4);

        HBRUSH hbr = CreateSolidBrush(bkColor);
        FillRect(lpcd->nmcd.hdc, &rcLabel, hbr);
        DeleteObject(hbr);

        wchar_t text[512] = {};
        TVITEMW tvi = {};
        tvi.mask = TVIF_TEXT;
        tvi.hItem = hItem;
        tvi.pszText = text;
        tvi.cchTextMax = static_cast<int>(ARRAYSIZE(text));
        SendMessageW(wnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&tvi));

        HFONT hFont = static_cast<HFONT>(UI::ThemeData::GetSingleton()->ThemeFont->Handle);
        HGDIOBJ hOldFont = SelectObject(lpcd->nmcd.hdc, hFont);
        SetBkMode(lpcd->nmcd.hdc, TRANSPARENT);
        SetTextColor(lpcd->nmcd.hdc, txColor);

        RECT rcText = rcLabel;
        rcText.left += 2;
        DrawTextW(lpcd->nmcd.hdc, text, -1, &rcText,
            DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

        SelectObject(lpcd->nmcd.hdc, hOldFont);
        return CDRF_DODEFAULT;
    }
    }

    *handled = false;
    return CDRF_DODEFAULT;
}

// ---------------------------------------------------------------------------
// Plugin entry points
// ---------------------------------------------------------------------------

extern "C"
{
    __declspec(dllexport) PluginAPI::CKPEPluginVersionData CKPEPlugin_Version =
    {
        CKPE::PluginAPI::CKPEPluginVersionData::kVersion,
        1,
        "WineCompat",
        "CKPE Contributors",
        static_cast<std::uint8_t>(
            CKPE::PluginAPI::CKPEPluginVersionData::kGameSkyrimSE |
            CKPE::PluginAPI::CKPEPluginVersionData::kGameFallout4  |
            CKPE::PluginAPI::CKPEPluginVersionData::kGameStarfield),
        0,
        { 0 },
        CKPE::PluginAPI::CKPEPluginVersionData::kAnyGames |
        CKPE::PluginAPI::CKPEPluginVersionData::kNoLinkedVersionGame
    };

    __declspec(dllexport) bool CKPEPlugin_HasDependencies() noexcept(true)
    {
        return false;
    }

    __declspec(dllexport) std::uint32_t CKPEPlugin_GetDependCount() noexcept(true)
    {
        return 0;
    }

    __declspec(dllexport) const char* CKPEPlugin_GetDependAt([[maybe_unused]] std::uint32_t id) noexcept(true)
    {
        return nullptr;
    }

    __declspec(dllexport) std::uint32_t CKPEPlugin_Load(
        const PluginAPI::CKPEPluginInterface* intf) noexcept(true)
    {
        auto sfname = PathUtils::GetCKPELogsPluginPath() + L"WineCompat.log";
        PathUtils::CreateFolder(PathUtils::ExtractFilePath(PathUtils::Normalize(sfname)));
        PluginAPI::UserPluginLogger.Open(sfname);

        auto* rt = static_cast<PluginAPI::CKPERuntimeInterface*>(
            intf->QueryInterface(PluginAPI::kInterface_Runtime));
        if (!rt || rt->InterfaceVersion != PluginAPI::CKPERuntimeInterface::kInterfaceVersion)
        {
            PluginAPI::_ERROR("WineCompat: CKPERuntimeInterface unavailable");
            return false;
        }

        if (!rt->IsUserUsingWine())
        {
            PluginAPI::_MESSAGE("WineCompat: not running under Wine, unloading");
            return true;
        }

        PluginAPI::_MESSAGE("WineCompat: Wine detected, installing overrides");

        auto* mh = static_cast<PluginAPI::CKPEMessageHookInterface*>(
            intf->QueryInterface(PluginAPI::kInterface_MessageHook));
        if (!mh || mh->InterfaceVersion != PluginAPI::CKPEMessageHookInterface::kInterfaceVersion)
        {
            PluginAPI::_ERROR("WineCompat: CKPEMessageHookInterface unavailable");
            return false;
        }

        auto* to = static_cast<PluginAPI::CKPEThemeOverrideInterface*>(
            intf->QueryInterface(PluginAPI::kInterface_ThemeOverride));
        if (!to || to->InterfaceVersion != PluginAPI::CKPEThemeOverrideInterface::kInterfaceVersion)
        {
            PluginAPI::_ERROR("WineCompat: CKPEThemeOverrideInterface unavailable");
            return false;
        }

        mh->RegisterInitDialogHook(OnInitDialog, nullptr);
        to->SetTreeViewCustomDraw(OnTreeViewCustomDraw, nullptr);

        PluginAPI::_MESSAGE("WineCompat: hooks registered");
        return true;
    }
}

BOOL APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      [[maybe_unused]] LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
