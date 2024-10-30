﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#pragma once

#include "Core/Module.h"
#include "Core/Relocator.h"
#include "Core/RelocationDatabase.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		namespace Fallout4
		{
			using namespace CreationKitPlatformExtended::Core;

			class AllowSaveESMandMasterESPPatch : public Module
			{
			public:
				AllowSaveESMandMasterESPPatch();

				virtual bool HasOption() const;
				virtual bool HasCanRuntimeDisabled() const;
				virtual const char* GetOptionName() const;
				virtual const char* GetName() const;
				virtual bool HasDependencies() const;
				virtual Array<String> GetDependencies() const;

				static BOOL OpenPluginSaveDialog(HWND ParentWindow, LPCSTR BasePath, BOOL IsESM, LPSTR Buffer, 
					uint32_t BufferSize, LPCSTR Directory);
			protected:
				virtual bool QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
					const char* lpcstrPlatformRuntimeVersion) const;
				virtual bool Activate(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem);
				virtual bool Shutdown(const Relocator* lpRelocator, const RelocationDatabaseItem* lpRelocationDatabaseItem);
			private:
				AllowSaveESMandMasterESPPatch(const AllowSaveESMandMasterESPPatch&) = default;
				AllowSaveESMandMasterESPPatch& operator=(const AllowSaveESMandMasterESPPatch&) = default;
			};
		}
	}
}