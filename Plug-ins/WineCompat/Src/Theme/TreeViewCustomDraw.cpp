// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <commctrl.h>

#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Graphics.h>

#include <Theme/TreeViewCustomDraw.h>

using namespace CKPE;
using namespace CKPE::Common;

LRESULT OnTreeViewCustomDraw(HWND wnd,
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
