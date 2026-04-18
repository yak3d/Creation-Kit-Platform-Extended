// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cmath>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UIScrollBar.h>
#include <CKPE.Graphics.h>

#include <ScrollBar/ScrollBarPaint.h>

using namespace CKPE;
using namespace CKPE::Common;

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

void DrawNcScrollBars(HWND hWnd, HDC hdc) noexcept(true)
{
    DWORD style = static_cast<DWORD>(GetWindowLongW(hWnd, GWL_STYLE));
    bool hasV = (style & WS_VSCROLL) != 0;
    bool hasH = (style & WS_HSCROLL) != 0;
    if (!hasV && !hasH) return;

    RECT wndRC = {};
    GetWindowRect(hWnd, &wndRC);
    int wW = wndRC.right  - wndRC.left;
    int wH = wndRC.bottom - wndRC.top;

    POINT clientPt = { 0, 0 };
    ClientToScreen(hWnd, &clientPt);
    int bL = clientPt.x - wndRC.left;
    int bT = clientPt.y - wndRC.top;

    RECT clientRC = {};
    GetClientRect(hWnd, &clientRC);
    int bR = wW - bL - clientRC.right;
    int bB = wH - bT - clientRC.bottom;

    if (bL < 0 || bT < 0 || bR < 0 || bB < 0) return;

    int vSBW = GetSystemMetrics(SM_CXVSCROLL);
    int hSBH = GetSystemMetrics(SM_CYHSCROLL);

    if (hasV && bR < vSBW) return;
    if (hasH && bB < hSBH) return;

    Canvas canvas(hdc);

    auto paintBar = [&](bool vert)
    {
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

        if (vert) UI::ScrollBar::Render::DrawBackgroundVert(canvas, &rcTrack);
        else      UI::ScrollBar::Render::DrawBackgroundHorz(canvas, &rcTrack);

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

    if (hasV && hasH)
    {
        RECT rcCorner = { wW - bR, wH - bB, wW - bR + vSBW, wH };
        canvas.Fill(rcCorner, UI::GetThemeSysColor(UI::ThemeColor_Default));
    }
}

bool TryHandleScrollBarClick(HWND hWnd, WPARAM wParam, LPARAM lParam) noexcept(true)
{
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
            int anchorOff = clickPos - thumbStart;
            int travelPx  = max(1, trackSz - thumbSz);
            int lastPos   = si.nPos;

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

                    SetScrollPos(hWnd, vert ? SB_VERT : SB_HORZ, newPos, FALSE);
                    SendMessage(hWnd, vert ? WM_VSCROLL : WM_HSCROLL,
                        MAKEWPARAM(SB_THUMBTRACK, (WORD)newPos), 0);

                    HDC hdc = GetWindowDC(hWnd);
                    if (hdc) { DrawNcScrollBars(hWnd, hdc); ReleaseDC(hWnd, hdc); }
                    continue;
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
