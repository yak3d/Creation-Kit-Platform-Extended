﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#pragma once

#include "..\BaseWindow.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		namespace Fallout4
		{
			class ProgressWindow : public BaseWindow, public Classes::CUIBaseWindow
			{
			public:
				virtual bool HasOption() const;
				virtual bool HasCanRuntimeDisabled() const;
				virtual const char* GetOptionName() const;
				virtual const char* GetName() const;
				virtual bool HasDependencies() const;
				virtual Array<String> GetDependencies() const;

				ProgressWindow();

				static HWND sub1(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent,
					DLGPROC lpDialogFunc, LPARAM dwInitParam);
				static void sub2(uint32_t nPartId, LPCSTR lpcstrText);
				static void update_progressbar(LPCSTR lpcstrText);

				static LRESULT CALLBACK HKWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
			protected:
				virtual bool QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
					const char* lpcstrPlatformRuntimeVersion) const;
				virtual bool Activate(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem);
				virtual bool Shutdown(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem);
			private:
				::Core::Classes::UI::CUIBaseControl ProgressLabel;
				::Core::Classes::UI::CUIBaseControl	Progress;
				bool isOpen;
			};

			extern ProgressWindow* GlobalProgressWindowPtr;
		}
	}
}