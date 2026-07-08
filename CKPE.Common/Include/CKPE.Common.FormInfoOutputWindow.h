// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#pragma once

#include <CKPE.Common.UIBaseWindow.h>

namespace CKPE
{
	namespace Common
	{
		class CKPE_COMMON_API FormInfoOutputWindow : public Common::UI::CUICustomWindow
		{
		public:
			FormInfoOutputWindow();

			INT_PTR OpenModal(const HWND hParentWindow);

			// Renamed from WndProc: this is a DLGPROC (returns INT_PTR), and a static
			// member sharing the base's virtual WndProc name+params is an illegal
			// override under clang (MSVC tolerates it). Behavior is unchanged.
			static INT_PTR CALLBACK DlgProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
		};
	}
}