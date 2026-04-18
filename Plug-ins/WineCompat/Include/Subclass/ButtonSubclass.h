// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#pragma once
#include <windows.h>

// Installs the dark-theme button subclass on hWnd (allocates ButtonState internally).
void ButtonSubclass_Attach(HWND hWnd) noexcept(true);

// Returns true if the button subclass is already installed on hWnd.
bool ButtonSubclass_IsAttached(HWND hWnd) noexcept(true);
