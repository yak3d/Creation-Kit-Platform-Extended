// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#pragma once
#include <windows.h>

// Subclass id=2
LRESULT CALLBACK ScrollBarPaintSubclass(HWND, UINT, WPARAM, LPARAM,
    UINT_PTR, DWORD_PTR) noexcept(true);
