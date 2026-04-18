// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#pragma once
#include <windows.h>
#include <commctrl.h>

LRESULT OnTreeViewCustomDraw(HWND wnd, LPNMLVCUSTOMDRAW lpcd,
    bool* handled, void* userdata) noexcept(true);
