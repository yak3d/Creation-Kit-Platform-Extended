// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <commctrl.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.UICheckBox.h>
#include <CKPE.Common.UIPushButton.h>
#include <CKPE.Graphics.h>

#include <Theme/DarkCheckboxImageList.h>

using namespace CKPE;
using namespace CKPE::Common;

HIMAGELIST CreateDarkCheckboxImageList(HWND hTreeView) noexcept(true)
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
