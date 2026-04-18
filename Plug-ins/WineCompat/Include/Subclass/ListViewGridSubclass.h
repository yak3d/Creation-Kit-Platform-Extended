// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#pragma once
#include <windows.h>

// Subclass id=1
LRESULT CALLBACK WineListViewGridSubclass(HWND, UINT, WPARAM, LPARAM,
    UINT_PTR, DWORD_PTR) noexcept(true);
