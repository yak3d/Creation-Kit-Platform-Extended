// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <uxtheme.h>

#include <CKPE.PluginAPI.PluginAPI.h>
#include <CKPE.Common.UIListView.h>
#include <CKPE.Common.UITreeView.h>
#include <CKPE.Common.UIVarCommon.h>

#include <Plugin/WineCompatMessages.h>
#include <Plugin/StyleDispatcher.h>
#include <Subclass/ButtonSubclass.h>
#include <Subclass/CheckBoxSubclass.h>
#include <Subclass/ComboBoxSubclass.h>
#include <Subclass/EditSubclass.h>
#include <Subclass/HeaderSubclass.h>
#include <Subclass/ListViewGridSubclass.h>
#include <Subclass/ScrollBarPaintSubclass.h>
#include <Subclass/DeferredRestyleSubclass.h>
#include <Theme/RichEditTheme.h>
#include <Theme/DarkCheckboxImageList.h>

using namespace CKPE;
using namespace CKPE::Common;

void StyleChildControls(HWND parent) noexcept(true)
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
            ListView_SetExtendedListViewStyleEx(child, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
            DWORD dbCheck = ListView_GetExtendedListViewStyle(child);
            PluginAPI::_MESSAGE_EX("[WineCompat] ListView {:08X}: exStyle after doublebuffer={:08X} doublebuffer_bit={}",
                reinterpret_cast<uintptr_t>(child), dbCheck, !!(dbCheck & LVS_EX_DOUBLEBUFFER));
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
            TreeView_SetExtendedStyle(child, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            DWORD tvDbCheck = TreeView_GetExtendedStyle(child);
            PluginAPI::_MESSAGE_EX("[WineCompat] TreeView {:08X}: exStyle after doublebuffer={:08X} doublebuffer_bit={}",
                reinterpret_cast<uintptr_t>(child), tvDbCheck, !!(tvDbCheck & TVS_EX_DOUBLEBUFFER));
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
                if (ButtonSubclass_IsAttached(child))
                    return TRUE;
                ButtonSubclass_Attach(child);
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

void OnInitDialog(HWND dlg, [[maybe_unused]] void* userdata) noexcept(true)
{
    StyleChildControls(dlg);
    SetWindowSubclass(dlg, DeferredRestyleSubclass, kDeferredSubclassId, 0);
    PostMessage(dlg, WM_WINECOMPAT_RESTYLE, 0, 0);
}
