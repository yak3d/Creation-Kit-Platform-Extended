// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <richedit.h>

#include <CKPE.Common.UIVarCommon.h>

#include <Theme/RichEditTheme.h>

using namespace CKPE;
using namespace CKPE::Common;

void ApplyDarkThemeToRichEdit(HWND hWnd) noexcept(true)
{
    SendMessageA(hWnd, EM_SETBKGNDCOLOR, FALSE,
        static_cast<LPARAM>(UI::GetThemeSysColor(UI::ThemeColor_Edit_Color)));

    CHARFORMAT2A fmt = {};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_COLOR | CFM_CHARSET;
    fmt.crTextColor = UI::GetThemeSysColor(UI::ThemeColor_Text_4);
    fmt.bCharSet    = DEFAULT_CHARSET;
    SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_ALL,      reinterpret_cast<LPARAM>(&fmt));
    SendMessageA(hWnd, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&fmt));
}
