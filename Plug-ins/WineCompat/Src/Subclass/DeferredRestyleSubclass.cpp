// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <commctrl.h>

#include <Plugin/WineCompatMessages.h>
#include <Plugin/StyleDispatcher.h>
#include <Subclass/DeferredRestyleSubclass.h>

LRESULT CALLBACK DeferredRestyleSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
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
