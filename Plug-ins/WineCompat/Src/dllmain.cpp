// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
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
#include <CKPE.Common.UIPushButton.h>
#include <CKPE.Graphics.h>

using namespace CKPE;
using namespace CKPE::Common;

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

    if (uMsg == WM_NCPAINT)
    {
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

            ReleaseDC(hWnd, hdc);
        }
        return result;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static HIMAGELIST CreateDarkCheckboxImageList(HWND hTreeView) noexcept(true);

// ---------------------------------------------------------------------------
// OnInitDialog — walk child controls and apply Wine-specific overrides
// ---------------------------------------------------------------------------

static void OnInitDialog(HWND dlg, [[maybe_unused]] void* userdata) noexcept(true)
{
    EnumChildWindows(dlg, [](HWND child, LPARAM) -> BOOL
    {
        wchar_t cls[64] = {};
        GetClassNameW(child, cls, ARRAYSIZE(cls));

        if (_wcsicmp(cls, WC_LISTVIEWW) == 0)
        {
            UI::ListView::Initialize(child);
            auto f = UI::ThemeData::GetSingleton()->ThemeFont;
            PostMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(f->Handle), TRUE);
        }
        else if (_wcsicmp(cls, WC_TREEVIEWW) == 0)
        {
            UI::TreeView::Initialize(child);
            // Initialize sets SetWindowTheme("","") under Wine for text colors, but that
            // also breaks the +/- expand glyphs (classic renderer uses COLOR_WINDOW → white).
            // Restore visual styles here; text/selection colors are handled via NM_CUSTOMDRAW.
            SetWindowTheme(child, nullptr, nullptr);
            auto f = UI::ThemeData::GetSingleton()->ThemeFont;
            PostMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(f->Handle), TRUE);

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
            SetWindowTheme(child, L"", L"");
            SetWindowSubclass(child, EditSubclass, 0, 0);
        }
        else if (_wcsicmp(cls, L"ComboBox") == 0)
        {
            SetWindowTheme(child, L"", L"");
            SetWindowSubclass(child, ComboBoxSubclass, 0, 0);
        }
        else if (_wcsicmp(cls, L"Button") == 0)
        {
            DWORD btnType = static_cast<DWORD>(GetWindowLongW(child, GWL_STYLE)) & BS_TYPEMASK;
            if (btnType == BS_PUSHBUTTON || btnType == BS_DEFPUSHBUTTON)
            {
                SetWindowTheme(child, L"", L"");
                SetWindowSubclass(child, ButtonSubclass, 0,
                    reinterpret_cast<DWORD_PTR>(new ButtonState{}));
            }
        }
        else if (_wcsicmp(cls, MSFTEDIT_CLASS) == 0 ||
                 _wcsicmp(cls, L"RichEdit20A") == 0  ||
                 _wcsicmp(cls, L"RichEdit20W") == 0)
        {
            ApplyDarkThemeToRichEdit(child);
        }

        return TRUE;
    }, 0);
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
