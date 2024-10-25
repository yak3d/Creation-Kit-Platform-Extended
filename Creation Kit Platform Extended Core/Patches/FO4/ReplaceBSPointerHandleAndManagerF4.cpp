﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "Core/Engine.h"
#include "Editor API/FO4/BSPointerHandleManager.h"
#include "ReplaceBSPointerHandleAndManagerF4.h"

namespace CreationKitPlatformExtended
{
	namespace Patches
	{
		namespace Fallout4
		{
			using namespace EditorAPI::Fallout4;

			uintptr_t pointer_ReplaceBSPointerHandleAndManager_code1 = 0;
			uintptr_t pointer_ReplaceBSPointerHandleAndManager_code2 = 0;
			uintptr_t pointer_ReplaceBSPointerHandleAndManager_code3 = 0;
			uintptr_t pointer_ReplaceBSPointerHandleAndManager_code4 = 0;

			class HandleManager
			{
			public:
				template<typename HandleType>
				static NiPointer<ObjectType> GetSmartPtr(const HandleType& Handle, bool Exist)
				{
					Exist = false;
					NiPointer<ObjectType> ObjectPtr;
					if (!BSPointerHandleManagerCurrent::PointerHandleManagerCurrentId)
						Exist = BSPointerHandleManagerInterface_Original::GetSmartPointer1(Handle, ObjectPtr);
					else
						Exist = BSPointerHandleManagerInterface_Extended::GetSmartPointer1(Handle, ObjectPtr);
				}
			};

			ReplaceBSPointerHandleAndManagerPatch::ReplaceBSPointerHandleAndManagerPatch() : Module(GlobalEnginePtr)
			{}

			bool ReplaceBSPointerHandleAndManagerPatch::HasOption() const
			{
				return false;
			}

			bool ReplaceBSPointerHandleAndManagerPatch::HasCanRuntimeDisabled() const
			{
				return false;
			}

			const char* ReplaceBSPointerHandleAndManagerPatch::GetOptionName() const
			{
				return nullptr;
			}

			const char* ReplaceBSPointerHandleAndManagerPatch::GetName() const
			{
				return "Replace BSPointerHandle And Manager";
			}

			bool ReplaceBSPointerHandleAndManagerPatch::HasDependencies() const
			{
				return false;
			}

			Array<String> ReplaceBSPointerHandleAndManagerPatch::GetDependencies() const
			{
				return {};
			}

			bool ReplaceBSPointerHandleAndManagerPatch::QueryFromPlatform(EDITOR_EXECUTABLE_TYPE eEditorCurrentVersion,
				const char* lpcstrPlatformRuntimeVersion) const
			{
				return eEditorCurrentVersion != EDITOR_EXECUTABLE_TYPE::EDITOR_FALLOUT_C4_1_10_943_1;
			}

			bool ReplaceBSPointerHandleAndManagerPatch::IsVersionValid(const RelocationDatabaseItem* lpRelocationDatabaseItem) const
			{
				auto verPatch = lpRelocationDatabaseItem->Version();
				return (verPatch == 1) || (verPatch == 2);
			}

			bool ReplaceBSPointerHandleAndManagerPatch::Install_163(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem, bool Extremly)
			{
				if (Extremly)
				{
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(0),
						(uintptr_t)&BSPointerHandleManager_Extended::InitSDM);
					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(1),
						(uintptr_t)&BSPointerHandleManager_Extended::KillSDM);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(2),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::GetCurrentHandle);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(3),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::CreateHandle);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(4),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(5),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy2);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(6),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::GetSmartPointer1);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(7),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::GetSmartPointer2);

					pointer_ReplaceBSPointerHandleAndManager_code1 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(9));
					pointer_ReplaceBSPointerHandleAndManager_code2 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(10));
					pointer_ReplaceBSPointerHandleAndManager_code3 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(11));
					pointer_ReplaceBSPointerHandleAndManager_code4 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(12));

					ScopeRelocator textSection;

					// Stub out the rest of the functions which shouldn't ever be called now
					lpRelocator->Patch(lpRelocationDatabaseItem->At(8), { 0xCC });	// BSUntypedPointerHandle::Set

					// Conversion BSHandleRefObject::IncRef and BSHandleRefObject::DecRef to 64bit

					IncRefPatch();
					DecRefPatch();

					BSPointerHandleManagerCurrent::PointerHandleManagerCurrentId = 1;
				}
				else
				{
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(0),
						(uintptr_t)&BSPointerHandleManager_Original::InitSDM);
					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(1),
						(uintptr_t)&BSPointerHandleManager_Original::KillSDM);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(2),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::GetCurrentHandle);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(3),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::CreateHandle);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(4),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(5),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy2);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(6),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::GetSmartPointer1);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(7),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::GetSmartPointer2);

					pointer_ReplaceBSPointerHandleAndManager_code1 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(9));
					pointer_ReplaceBSPointerHandleAndManager_code2 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(10));
					pointer_ReplaceBSPointerHandleAndManager_code3 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(11));
					pointer_ReplaceBSPointerHandleAndManager_code4 =
						lpRelocator->Rav2Off(lpRelocationDatabaseItem->At(12));

					ScopeRelocator textSection;

					// Stub out the rest of the functions which shouldn't ever be called now
					lpRelocator->Patch(lpRelocationDatabaseItem->At(8), { 0xCC });	// BSUntypedPointerHandle::Set

					BSPointerHandleManagerCurrent::PointerHandleManagerCurrentId = 0;
				}

				return true;
			}

			bool ReplaceBSPointerHandleAndManagerPatch::Install_980(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem, bool Extremly)
			{
				auto restoring_destroy1 = [&lpRelocator](uintptr_t rva, uint32_t removal_size, uintptr_t func)
				{
					lpRelocator->PatchNop(rva, removal_size);
					lpRelocator->Patch(rva, { 0x48, 0x89, 0xC1 });
					lpRelocator->DetourCall(rva + 3, func);
				};

				auto restoring_destroy2 = [&lpRelocator](uintptr_t rva, uint8_t off_rsp, uint32_t removal_size, uintptr_t func)
				{
					lpRelocator->PatchNop(rva, removal_size);
					lpRelocator->Patch(rva, { 0x48, 0x8D, 0x4C, 0x24, off_rsp });
					lpRelocator->DetourCall(rva + 5, func);
				};

				if (Extremly)
				{
					BSPointerHandleManagerCurrent::PointerHandleManagerCurrentId = 1;

					{
						ScopeRelocator textSection;

						// Preparation, removal of all embedded pieces of code
						lpRelocator->PatchNop((uintptr_t)lpRelocationDatabaseItem->At(0) + 12, 0x7A);
						lpRelocator->PatchMovFromRax((uintptr_t)lpRelocationDatabaseItem->At(0) + 5, lpRelocationDatabaseItem->At(1));

						// Stub out the rest of the functions which shouldn't ever be called now
						lpRelocator->Patch(lpRelocationDatabaseItem->At(4), { 0xCC });	// BSUntypedPointerHandle::Set			
					}

					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(0),
						(uintptr_t)&BSPointerHandleManager_Extended::InitSDM);
					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(2),
						(uintptr_t)&BSPointerHandleManager_Extended::KillSDM);
					// Unfortunately, the array cleanup is not going through, so let's reset it ourselves
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(15),
						(uintptr_t)&BSPointerHandleManager_Extended::CleanSDM);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(3),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::CreateHandle);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(5),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(6),
						(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy2);

					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(16), (uintptr_t)&CheckEx);

					{
						ScopeRelocator textSection;

						//
						// Deleting the code, restoring the function
						//
						restoring_destroy1(lpRelocationDatabaseItem->At(7), 0xF2,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(8), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(9), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(10), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(11), 0xEF,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(12), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(13), 0xEF,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy1);
						restoring_destroy2(lpRelocationDatabaseItem->At(14), 0x30, 0x10B,
							(uintptr_t)&BSPointerHandleManagerInterface_Extended::Destroy2);

						auto textRange = GlobalEnginePtr->GetSection(SECTION_TEXT);

						//
						// Change AGE
						// 
						size_t total = 0;

						{
							// test ??, 0x3E00000
							auto patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"A9 00 00 E0 03");
							for (size_t i = 0; i < patterns.size(); i++)
								memcpy((void*)(patterns[i] + 1), &BSUntypedPointerHandle_Extended::MASK_AGE_BIT, 4);
							total += patterns.size();

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"F7 ? 00 00 E0 03");
							for (size_t i = 0; i < patterns.size(); i++)
								memcpy((void*)(patterns[i] + 2), &BSUntypedPointerHandle_Extended::MASK_AGE_BIT, 4);
							total += patterns.size();
						}
						
						{
							// and r??, 0x3E00000
							auto patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"25 00 00 E0 03");
							for (size_t i = 0; i < patterns.size(); i++)
								memcpy((void*)(patterns[i] + 1), &BSUntypedPointerHandle_Extended::MASK_AGE_BIT, 4);
							total += patterns.size();

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"81 ? 00 00 E0 03");
							for (size_t i = 0; i < patterns.size(); i++)
								memcpy((void*)(patterns[i] + 2), &BSUntypedPointerHandle_Extended::MASK_AGE_BIT, 4);
							total += patterns.size();
						}

						// should be 660
						Assert(total == 660);
						//_CONSOLE("AGE_MASK change: %llu", total);

						//
						// Change INDEX
						// 
						total = 0;

						{
							// and r???, 0x1FFFFF
							auto patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"41 81 ? FF FF 1F 00");
							for (size_t i = 0; i < patterns.size(); i++)
								memcpy((void*)(patterns[i] + 3), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
							total += patterns.size();

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"81 E1 FF FF 1F 00");
							for (size_t i = 3; i < patterns.size() - 1; i++)
								memcpy((void*)(patterns[i] + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
							total += patterns.size() - 4;

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"81 E5 FF FF 1F 00");
							for (size_t i = 0; i < patterns.size(); i++)
							{
								if ((i == 4) || (i == 8)) continue;
								memcpy((void*)(patterns[i] + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
							}
							total += patterns.size() - 2;

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"81 E6 FF FF 1F 00");
							Assert(patterns.size() == 7);
							for (size_t i = 3; i < patterns.size() - 2; i++)
							{
								if ((i == 4) || (i == 8)) continue;
								memcpy((void*)(patterns[i] + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
							}
							total += 2;

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"81 E7 FF FF 1F 00");
							Assert(patterns.size() == 3);
							memcpy((void*)(patterns[2] + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
							total++;

							auto addr = voltek::find_pattern(textRange.base, textRange.end - textRange.base,
								"8B C7 25 FF FF 1F 00 8B D8");
							if (addr)
							{
								memcpy((void*)(addr + 3), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
								total++;
							}

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"81 ? FF FF 1F 00 ? ? 48 ? ? 04");
							Assert(patterns.size() == 8);
							for (size_t i = 0; i < patterns.size(); i++)
								memcpy((void*)(patterns[i] + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
							total += patterns.size();

							addr = voltek::find_pattern(textRange.base, textRange.end - textRange.base,
								"81 ? FF FF 1F 00 ? ? ? ? ? ? 48 ? ? 04");
							if (addr)
							{
								memcpy((void*)(addr + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
								total++;
							}

							addr = voltek::find_pattern(textRange.base, textRange.end - textRange.base,
								"81 ? FF FF 1F 00 ? ? ? ? ? ? 44 ? ? 41 ? 00 00 00 00 49 ? ? 04");
							if (addr)
							{
								memcpy((void*)(addr + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
								total++;
							}

							addr = voltek::find_pattern(textRange.base, textRange.end - textRange.base,
								"81 ? FF FF 1F 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 49 ? ? 04");
							if (addr)
							{
								memcpy((void*)(addr + 2), &BSUntypedPointerHandle_Extended::MASK_INDEX_BIT, 4);
								total++;
							}



						}

						// should be 345
						Assert(total == 345);
						//_CONSOLE("INDEX_MASK change: %llu", total);

						//
						// Change UNUSED_BIT_START
						// 
						total = 0;

						{
							// bt ???, 0x1A
							auto patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"0F BA E0 1A");
							Assert(patterns.size() == 322);
							for (size_t i = 0; i < patterns.size() - 5; i++)
								*(uint8_t*)(patterns[i] + 3) = (uint8_t)BSUntypedPointerHandle_Extended::UNUSED_BIT_START;
							total += patterns.size() - 5;

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"0F BA ?? 1A");
							Assert(patterns.size() == 104);
							for (size_t i = 0; i < 57; i++)
							{
								switch (i)
								{
								case 16:
								case 17:
								case 21:
								case 22:
								case 26:
								case 32:
								case 33:
								case 35:
								case 40:
								case 41:
								case 50:
								case 51:
								case 52:
								case 53:
								case 54:
								case 55:
									break;
								default:
								{
									*(uint8_t*)(patterns[i] + 3) = (uint8_t)BSUntypedPointerHandle_Extended::UNUSED_BIT_START;
									total++;
								}
								}
							}
						}

						// should be 358
						//_CONSOLE("UNUSED_BIT_START change: %llu", total);
						Assert(total == 358);

						//
						// Change REFR test eax -> rax
						// 
						//total = 0;

						//{
						//	// mov eax, dword ptr ds:[r??+0x38]
						//	// and eax, 0x3FF
						//	// cmp eax, 0x3FF
						//	auto patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
						//		"8B ? 38 25 FF 03 00 00 3D FF 03 00 00");
						//	for (size_t i = 0; i < patterns.size(); i++)
						//	{
						//		auto prefix = *(uint8_t*)(patterns[i] - 1);
						//		if (prefix == 0x41)
						//			*(uint8_t*)(patterns[i] - 1) = 0x49;
						//		else
						//		{
						//			auto reg = *(uint8_t*)(patterns[i] + 1);
						//			memcpy((void*)(patterns[i]),
						//				"\x48\x8B\x00\x38\x66\x25\xFF\x03\x66\x3D\xFF\x03\x90", 13);
						//			*(uint8_t*)(patterns[i] + 2) = reg;
						//		}
						//	}
						//	total += patterns.size();

						//	// mov e??, dword ptr ds:[r??+0x38]
						//	// and e??, 0x3FF
						//	// cmp e??, 0x3FF
						//	patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
						//		"8B ? 38 81 ? FF 03 00 00 81 ? FF 03 00 00");
						//	for (size_t i = 0; i < patterns.size(); i++)
						//	{
						//		auto reg1 = *(uint8_t*)(patterns[i] + 1);
						//		auto reg2 = *(uint8_t*)(patterns[i] + 4);
						//		auto reg3 = *(uint8_t*)(patterns[i] + 10);
						//		memcpy((void*)(patterns[i]),
						//			"\x48\x8B\x00\x38\x66\x81\x00\xFF\x03\x66\x81\x00\xFF\x03\x90", 15);
						//		*(uint8_t*)(patterns[i] + 2) = reg1;
						//		*(uint8_t*)(patterns[i] + 6) = reg2;
						//		*(uint8_t*)(patterns[i] + 11) = reg3;
						//	}
						//	total += patterns.size();
						//}
						//
						//// should be 298
						////_CONSOLE("Change REFR test eax -> rax: %llu", total);
						//Assert(total == 298);

						//
						// Change REFR test handle index
						// 
						total = 0;

						{
							// mov e??, dword ptr ds:[r??+0x38]
							// shr e??, 0xB
							// cmp e??, r??
							auto patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"8B ? 38 C1 ? 0B 41 3B ?");
							for (size_t i = 0; i < patterns.size(); i++)
							{
								*(uint8_t*)(patterns[i] + 2) = (uint8_t)0x39;
								*(uint8_t*)(patterns[i] + 5) = (uint8_t)0x3;
							}

							total += patterns.size();

							patterns = voltek::find_patterns(textRange.base, textRange.end - textRange.base,
								"8B ? 38 C1 ? 0B 3B ?");
							for (size_t i = 0; i < patterns.size(); i++)
							{
								*(uint8_t*)(patterns[i] + 2) = (uint8_t)0x39;
								*(uint8_t*)(patterns[i] + 5) = (uint8_t)0x3;
							}

							total += patterns.size();
						}

						// should be 308
						//_CONSOLE("Change REFR test handle index: %llu", total);
						Assert(total == 307);
					}
				}
				else
				{
					BSPointerHandleManagerCurrent::PointerHandleManagerCurrentId = 0;

					{
						ScopeRelocator textSection;

						// Preparation, removal of all embedded pieces of code
						lpRelocator->PatchNop((uintptr_t)lpRelocationDatabaseItem->At(0) + 12, 0x7A);
						lpRelocator->PatchMovFromRax((uintptr_t)lpRelocationDatabaseItem->At(0) + 5, lpRelocationDatabaseItem->At(1));

						// Stub out the rest of the functions which shouldn't ever be called now
						lpRelocator->Patch(lpRelocationDatabaseItem->At(4), { 0xCC });	// BSUntypedPointerHandle::Set			
					}

					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(0),
						(uintptr_t)&BSPointerHandleManager_Original::InitSDM);
					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(2),
						(uintptr_t)&BSPointerHandleManager_Original::KillSDM);
					// Unfortunately, the array cleanup is not going through, so let's reset it ourselves
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(15),
						(uintptr_t)&BSPointerHandleManager_Original::CleanSDM);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(3),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::CreateHandle);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(5),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
					lpRelocator->DetourJump(lpRelocationDatabaseItem->At(6),
						(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy2);

					lpRelocator->DetourCall(lpRelocationDatabaseItem->At(16), (uintptr_t)&Check);

					{
						ScopeRelocator textSection;

						//
						// Deleting the code, restoring the function
						//
						restoring_destroy1(lpRelocationDatabaseItem->At(7), 0xF2,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(8), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(9), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(10), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(11), 0xEF,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(12), 0xFE,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy1(lpRelocationDatabaseItem->At(13), 0xEF,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy1);
						restoring_destroy2(lpRelocationDatabaseItem->At(14), 0x30, 0x10B,
							(uintptr_t)&BSPointerHandleManagerInterface_Original::Destroy2);
					}
				}

				return true;
			}

			bool ReplaceBSPointerHandleAndManagerPatch::Activate(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem)
			{
				if (!IsVersionValid(lpRelocationDatabaseItem))
					return false;
				
				auto Extremly = _READ_OPTION_BOOL("CreationKit", "bBSPointerHandleExtremly", false);
				auto verPatch = lpRelocationDatabaseItem->Version();

				if (Extremly)
					_CONSOLE("[WARNING] An extended set of refs has been included. You use it at your own risk.");

				if (verPatch == 1)
					return Install_163(lpRelocator, lpRelocationDatabaseItem, Extremly);
				else if (verPatch == 2)
					return Install_980(lpRelocator, lpRelocationDatabaseItem, Extremly);
				
				return false;
			}

			bool ReplaceBSPointerHandleAndManagerPatch::Shutdown(const Relocator* lpRelocator,
				const RelocationDatabaseItem* lpRelocationDatabaseItem)
			{
				return false;
			}

			uint32_t ReplaceBSPointerHandleAndManagerPatch::CheckEx(uintptr_t unused, uintptr_t refr)
			{
				if (!refr)
					return 0;

				return ((TESObjectREFR_base_Extremly*)refr)->GetHandleEntryIndex();
			}

			uint32_t ReplaceBSPointerHandleAndManagerPatch::Check(uintptr_t unused, uintptr_t refr)
			{
				if (!refr)
					return 0;

				return ((TESObjectREFR_base_Original*)refr)->GetHandleEntryIndex();
			}

			void ReplaceBSPointerHandleAndManagerPatch::IncRefPatch()
			{
				auto Sec = GlobalEnginePtr->GetSection(SECTION_TEXT);
				auto SignIncRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base,
					"4C 8D 05 ? ? ? ? 48 8D 0D ? ? ? ? BA 3B 00 00 00 E8");
				size_t patched = 0, need_p = 0;
				uint8_t szbuff[64];

				for (auto sign : SignIncRef)
				{
					auto start_r = sign + 24;

					if ((*((uint8_t*)start_r) == 0xF0)) // lock
					{
						if (*((uint8_t*)(start_r + 1)) == 0xFF) // inc
						{
							memcpy_s(szbuff, 3, (uint8_t*)start_r + 1, 3);

							memset((uint8_t*)start_r, 0x90, 0xB);
							((uint8_t*)start_r)[0] = 0xF0;
							((uint8_t*)start_r)[1] = 0x48;
							memcpy_s((uint8_t*)(start_r + 2), 3, szbuff, 3);

							((uint8_t*)start_r)[0xB] = 0xEB;	// jmp	(skip test)

							patched++;
#if FALLOUT4_DEVELOPER_MODE
							memset((uint8_t*)sign, 0x90, 7);
#endif
						}
						else if ((*((uint8_t*)(start_r + 1)) == 0x41) && (*((uint8_t*)(start_r + 2)) == 0xFF)) // inc r8 - r15
						{
							((uint8_t*)start_r)[1] = 0x49;
							((uint8_t*)start_r)[5] = 0x49;

							patched++;
#if FALLOUT4_DEVELOPER_MODE
							memset((uint8_t*)sign, 0x90, 7);
#endif
						}
#if FALLOUT4_DEVELOPER_MODE
						need_p++;
#endif
					}
				}

				_MESSAGE("BSHandleRefObject::IncRef (Patched: %d)", patched);
			}

			void ReplaceBSPointerHandleAndManagerPatch::DecRefPatch()
			{
				auto IsJump = [](uintptr_t off) -> bool {
					return ((*((uint8_t*)(off)) == 0x75) || (*((uint8_t*)(off)) == 0x74) ||
						((*((uint8_t*)(off)) == 0x0F) && ((*((uint8_t*)(off + 1)) == 0x85) || (*((uint8_t*)(off + 1)) == 0x84))));
				};

				size_t patched = 0;
				uint8_t szbuff[64];
				uint8_t* src = nullptr;
				int8_t jump_to = 0;
				int8_t jump_from = 0;

				auto refmask = (DWORD)EditorAPI::BSHandleRefObject_64_Extremly::REF_COUNT_MASK;
				auto Sec = GlobalEnginePtr->GetSection(SECTION_TEXT);
				auto SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "83 ? FF F0 0F ? ? ? FF ? ? FF 03 00 00");

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x29;

					patched_var1:

						src = (uint8_t*)(sign - jump_from);

						memcpy_s(szbuff, 64, (uint8_t*)sign, 0xF);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						memcpy_s(src + 1, 0xF, szbuff, 3);
						src[4] = 0xF0;
						src[5] = 0x48;
						memcpy_s(src + 6, 0xF, szbuff + 4, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0xF, szbuff + 8, 2);
						src[13] = 0x48;
						src[14] = szbuff[10];
						memcpy_s(src + 15, 0xF, &refmask, sizeof(refmask));
						src[19] = 0xEB;
						src[20] = jump_to - 21;

						src = (uint8_t*)(sign + 3);
						src[0] = 0xEB;
						src[1] = -(jump_from + 5);

						patched++;
					}
					else if (*((uint8_t*)(sign - 0x1E)) == 0x77)
					{
						jump_from = 0x1E;
						jump_to = 0x2D;

						goto patched_var1;
					}
					else if ((*((uint8_t*)(sign - 0x1B)) == 0x77) && (*((uint8_t*)(sign - 1)) == 0x48))
					{
						jump_from = 0x1B;
						jump_to = 0x2A;

						goto patched_var1;
					}
					else if (*((uint8_t*)(sign - 0x18)) == 0x77)
					{
						jump_from = 0x18;
						jump_to = 0x27;

						goto patched_var1;
					}
					else if (*((uint8_t*)(sign - 0x19)) == 0x77)
					{
						jump_from = 0x19;
						jump_to = 0x28;

						goto patched_var1;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "83 ? FF F0 0F ? ? ? FF ? F7 ? FF 03 00 00");

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x2A;

					patched_var2:

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0x10);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						memcpy_s(src + 1, 0x10, szbuff, 3);
						src[4] = 0xF0;
						src[5] = 0x48;
						memcpy_s(src + 6, 0x10, szbuff + 4, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0x10, szbuff + 8, 2);
						src[13] = 0x48;
						memcpy_s(src + 14, 0x10, szbuff + 10, 2);
						memcpy_s(src + 16, 0x10, &refmask, sizeof(refmask));
						src[20] = 0xEB;
						src[21] = jump_to - 22;

						src = (uint8_t*)(sign + 3);
						src[0] = 0xEB;
						src[1] = -(jump_from + 5);

						patched++;
					}
					else if ((*((uint8_t*)(sign - 0x1B)) == 0x77) && (*((uint8_t*)(sign - 1)) == 0x48))
					{
						jump_from = 0x1B;
						jump_to = 0x2B;

						goto patched_var2;
					}
					else if (*((uint8_t*)(sign - 0x18)) == 0x77)
					{
						jump_from = 0x18;
						jump_to = 0x28;

						goto patched_var2;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "83 ? FF F0 ? 0F ? ? ? FF ? ? FF 03 00 00");

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x2A;

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0x10);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						memcpy_s(src + 1, 0x10, szbuff, 3);
						src[4] = 0xF0;
						src[5] = szbuff[4] == 0x41 ? 0x49 : 0x4C;
						memcpy_s(src + 6, 0x10, szbuff + 5, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0x10, szbuff + 9, 2);
						src[13] = 0x48;
						src[14] = szbuff[11];
						memcpy_s(src + 15, 0x10, &refmask, sizeof(refmask));
						src[19] = 0xEB;
						src[20] = jump_to - 21;

						src = (uint8_t*)(sign + 3);
						src[0] = 0xEB;
						src[1] = -(jump_from + 5);

						patched++;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "8B ? F0 0F ? ? ? FF ? ? FF 03 00 00");

				// or r??, FF
				// lock xadd qword ptr ss:[r??+38], r??
				// dec r??
				// test r??, 3FF
				// 
				// ^ ^ converting the bottom to the top ^ ^
				// 
				// mov e??, e??
				// lock xadd dword ptr ss:[r??+38], e??
				// dec e??
				// test e??, 3FF

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x28;

					patched_var3:

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0xE);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						src[1] = 0x83;
						src[2] = szbuff[8];
						src[3] = 0xFF;

						src[4] = 0xF0;
						src[5] = 0x48;
						memcpy_s(src + 6, 0xE, szbuff + 3, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0xE, szbuff + 7, 2);
						src[13] = 0x48;
						src[14] = szbuff[9];
						memcpy_s(src + 15, 0xE, &refmask, sizeof(refmask));
						src[19] = 0xEB;
						src[20] = jump_to - 21;

						src = (uint8_t*)(sign + 2);
						src[0] = 0xEB;
						src[1] = -(jump_from + 4);

						patched++;
					}
					if ((*((uint8_t*)(sign - 0x1B)) == 0x77) && (*((uint8_t*)(sign - 1)) == 0x41))
					{
						jump_from = 0x1B;
						jump_to = 0x29;

						goto patched_var3;
					}
					if ((*((uint8_t*)(sign - 0x1D)) == 0x77) && (*((uint8_t*)(sign - 3)) == 0x83))
					{
						jump_from = 0x1D;
						jump_to = 0x2B;

						goto patched_var3;
					}
					else if ((*((uint8_t*)(sign - 0x1F)) == 0x77) && (*((uint8_t*)(sign - 4)) == 0x83))
					{
						jump_from = 0x1F;
						jump_to = 0x2D;

						goto patched_var3;
					}
					else if (*((uint8_t*)(sign - 0x18)) == 0x77)
					{
						jump_from = 0x18;
						jump_to = 0x26;

						goto patched_var3;
					}
					else if (*((uint8_t*)(sign - 0x19)) == 0x77)
					{
						jump_from = 0x19;
						jump_to = 0x27;

						goto patched_var3;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "41 8B ? F0 ? 0F ? ? ? FF ? ? FF 03 00 00");

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x2A;

					patched_var5:

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0x10);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						src[1] = 0x83;
						src[2] = szbuff[10];
						src[3] = 0xFF;

						src[4] = 0xF0;
						src[5] = (szbuff[4] == 0x44) ? 0x4C : 0x49;
						memcpy_s(src + 6, 0x10, szbuff + 5, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0xE, szbuff + 9, 2);
						src[13] = 0x48;
						src[14] = szbuff[11];
						memcpy_s(src + 15, 0x10, &refmask, sizeof(refmask));
						src[19] = 0xEB;
						src[20] = jump_to - 21;

						src = (uint8_t*)(sign + 2);
						src[0] = 0xEB;
						src[1] = -(jump_from + 4);

						patched++;
					}
					else if ((*((uint8_t*)(sign - 0x1E)) == 0x77) && (*((uint8_t*)(sign - 3)) == 0x83))
					{
						jump_from = 0x1E;
						jump_to = 0x2E;

						goto patched_var5;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "F0 ? 0F ? ? ? 41 FF ? 41 F7 ? FF 03 00 00");

				// or r??, FF
				// lock xadd qword ptr ss:[r??+38], r??
				// dec r??
				// test r??, 3FF
				// 
				// ^ ^ converting the bottom to the top ^ ^
				// 
				// lock xadd dword ptr ss:[r??+38], e??
				// dec e??
				// test e??, 3FF

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x2A;

					patched_var6:

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0x10);
						memset(src, 0x90, jump_to);

						src[0] = 0x49;
						src[1] = 0x83;
						src[2] = szbuff[8];
						src[3] = 0xFF;

						src[4] = 0xF0;
						src[5] = szbuff[1] == 0x44 ? 0x4C : 0x49;
						memcpy_s(src + 6, 0x10, szbuff + 2, 4);
						src[10] = 0x49;
						memcpy_s(src + 11, 0x10, szbuff + 7, 2);
						src[13] = 0x49;
						memcpy_s(src + 14, 0x10, szbuff + 10, 2);
						memcpy_s(src + 16, 0x10, &refmask, sizeof(refmask));
						src[20] = 0xEB;
						src[21] = jump_to - 22;

						src = (uint8_t*)(sign + 2);
						src[0] = 0xEB;
						src[1] = -(jump_from + 4);

						patched++;
					}
					else if ((*((uint8_t*)(sign - 0x1E)) == 0x77) && (*((uint8_t*)(sign - 3)) == 0x83))
					{
						jump_from = 0x1E;
						jump_to = 0x2E;

						goto patched_var6;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "F0 0F ? ? ? FF ? F7 ? FF 03 00 00");

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x1A)) == 0x77)
					{
						jump_from = 0x1A;
						jump_to = 0x27;

					patched_var7:

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0xD);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						src[1] = 0x83;
						src[2] = szbuff[6];
						src[3] = 0xFF;

						src[4] = 0xF0;
						src[5] = 0x48;
						memcpy_s(src + 6, 0x10, szbuff + 1, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0x10, szbuff + 5, 2);
						src[13] = 0x48;
						memcpy_s(src + 14, 0x10, szbuff + 7, 2);
						memcpy_s(src + 16, 0x10, &refmask, sizeof(refmask));
						src[20] = 0xEB;
						src[21] = jump_to - 22;

						src = (uint8_t*)(sign + 2);
						src[0] = 0xEB;
						src[1] = -(jump_from + 4);

						patched++;
					}
					else if (*((uint8_t*)(sign - 0x18)) == 0x77)
					{
						jump_from = 0x18;
						jump_to = 0x25;

						goto patched_var7;
					}
				}

				SignDecRef = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "8B ? F0 ? 0F ? ? ? FF ? ? FF 03 00 00");

				for (auto sign : SignDecRef)
				{
					if (*((uint8_t*)(sign - 0x18)) == 0x77)
					{
						jump_from = 0x18;
						jump_to = 0x27;

						src = (uint8_t*)(sign - jump_from);
						memcpy_s(szbuff, 64, (uint8_t*)sign, 0xF);
						memset(src, 0x90, jump_to);

						src[0] = 0x48;
						src[1] = 0x83;
						src[2] = szbuff[9];
						src[3] = 0xFF;

						src[4] = 0xF0;
						src[5] = szbuff[3] == 0x41 ? 0x49 : 0x4C;
						memcpy_s(src + 6, 0xF, szbuff + 4, 4);
						src[10] = 0x48;
						memcpy_s(src + 11, 0xF, szbuff + 8, 2);
						src[13] = 0x48;
						src[14] = szbuff[10];
						memcpy_s(src + 15, 0xF, &refmask, sizeof(refmask));
						src[19] = 0xEB;
						src[20] = jump_to - 21;

						src = (uint8_t*)(sign + 2);
						src[0] = 0xEB;
						src[1] = -(jump_from + 4);

						patched++;
					}
				}
				
				auto sign = pointer_ReplaceBSPointerHandleAndManager_code1;
				jump_from = 0x1A;
				jump_to = 0x30;

				src = (uint8_t*)(sign - jump_from);
				memcpy_s(szbuff, 64, (uint8_t*)sign, 0x16);
				memset(src, 0x90, jump_to);

				src[0] = 0x48;
				memcpy_s(src + 1, 0x16, szbuff, 3);
				src[4] = 0xF0;
				src[5] = 0x48;
				memcpy_s(src + 6, 0x16, szbuff + 4, 4);
				memcpy_s(src + 10, 0x16, szbuff + 8, 7);
				src[17] = 0x48;
				memcpy_s(src + 18, 0x16, szbuff + 15, 2);
				src[20] = 0x48;
				src[21] = szbuff[17];
				memcpy_s(src + 22, 0x16, &refmask, sizeof(refmask));
				src[26] = 0xEB;
				src[27] = jump_to - 28;

				src = (uint8_t*)(sign + 2);
				src[0] = 0xEB;
				src[1] = -(jump_from + 4);

				patched++;

				sign = pointer_ReplaceBSPointerHandleAndManager_code2;
				jump_from = 0x1A;
				jump_to = 0x28;

				src = (uint8_t*)(sign - jump_from);
				memcpy_s(szbuff, 64, (uint8_t*)sign, 0xE);
				memset(src, 0x90, jump_to);

				src[0] = 0x48;
				src[1] = 0x83;
				src[2] = szbuff[7];
				src[3] = 0xFF;

				src[4] = 0xF0;
				src[5] = 0x49;
				memcpy_s(src + 6, 0xE, szbuff + 2, 4);
				src[10] = 0x48;
				memcpy_s(src + 11, 0xE, szbuff + 6, 2);
				src[13] = 0x48;
				memcpy_s(src + 14, 0xE, szbuff + 8, 2);
				memcpy_s(src + 16, 0xE, &refmask, sizeof(refmask));
				src[20] = 0xEB;
				src[21] = jump_to - 22;

				src = (uint8_t*)(sign + 2);
				src[0] = 0xEB;
				src[1] = -(jump_from + 4);

				patched++;

				sign = pointer_ReplaceBSPointerHandleAndManager_code3;
				jump_from = 0x1A;
				jump_to = 0x2B;

				src = (uint8_t*)(sign - jump_from);
				memcpy_s(szbuff, 64, (uint8_t*)sign, 0x11);
				memset(src, 0x90, jump_to);

				src[0] = 0x48;
				memcpy_s(src + 1, 0x11, szbuff, 3);
				src[4] = 0xF0;
				src[5] = 0x49;
				memcpy_s(src + 6, 0x11, szbuff + 5, 5);
				src[11] = 0x48;
				memcpy_s(src + 12, 0x11, szbuff + 10, 2);
				src[14] = 0x48;
				src[15] = szbuff[12];
				memcpy_s(src + 16, 0x11, &refmask, sizeof(refmask));
				src[20] = 0xEB;
				src[21] = jump_to - 22;

				src = (uint8_t*)(sign + 2);
				src[0] = 0xEB;
				src[1] = -(jump_from + 4);

				sign = pointer_ReplaceBSPointerHandleAndManager_code4;
				jump_from = 0x1A;
				jump_to = 0x2B;

				src = (uint8_t*)(sign - jump_from);
				memcpy_s(szbuff, 64, (uint8_t*)sign, 0x11);
				memset(src, 0x90, jump_to);

				src[0] = 0x48;
				memcpy_s(src + 1, 0x11, szbuff, 3);
				src[4] = 0xF0;
				src[5] = 0x49;
				memcpy_s(src + 6, 0x11, szbuff + 5, 4);
				src[10] = 0x48;
				memcpy_s(src + 11, 0x11, szbuff + 9, 2);
				src[13] = 0x48;
				memcpy_s(src + 14, 0x11, szbuff + 11, 2);
				memcpy_s(src + 16, 0x11, &refmask, sizeof(refmask));
				src[20] = 0xEB;
				src[21] = jump_to - 22;

				src = (uint8_t*)(sign + 2);
				src[0] = 0xEB;
				src[1] = -(jump_from + 4);

				patched += 2;

				_MESSAGE("BSHandleRefObject::DecRef (Patched: %d)", patched);
			}

			void ReplaceBSPointerHandleAndManagerPatch::IncRefPatch_980()
			{
				//auto Sec = GlobalEnginePtr->GetSection(SECTION_TEXT);
				//auto Signatures = voltek::find_patterns(Sec.base, Sec.end - Sec.base, "? ? 38 ? FF 03 00 00 ? FF 03 00 00 72 ?");
				//Assert(Signatures.size() == 282);
				//
				//size_t total = 0;
				//for (auto sign : Signatures)
				//{
				//	auto start = (uint8_t*)sign;

				//	// je
				//	if (*(start - 2) == 74)
				//	{
				//		// nop's
				//		memset(start + 7, 0x90, (uintptr_t)(*(start - 1)) - 10);

				//		if (*(start + 1) == 0x43)	// rbx
				//			memcpy(start, "/x48/x89/xD9", 3);
				//		else if (*(start + 1) == 0x47)	// rdi
				//			memcpy(start, "/x48/x89/xF9", 3);
				//		else if (*(start + 1) == 0x45)	// rbp
				//			memcpy(start, "/x48/x89/xE9", 3);
				//		else if (*(start + 1) == 0x46)	// rsi
				//			memcpy(start, "/x48/x89/xF1", 3);
				//		else if (*(start + 1) == 0x40)	// rax
				//			memcpy(start, "/x48/x89/xC1", 3);
				//		else if (*(start + 1) == 0x42)	// rdx
				//			memcpy(start, "/x48/x89/xD1", 3);
				//		else if (*(start + 1) == 0x41)	// rcx
				//			memset(start, 0x90, 3);

				//		// mov rcx, r??
				//		// call ....
				//		voltek::detours_function_class_call((uintptr_t)(start + 3), TESObjectREFR_base_Extremly::IncRefCount);

				//		total++;
				//	}
				//	else if (*(start - 3) == 74)	// with prefix
				//	{
				//		// nop's
				//		memset(start + 6, 0x90, (uintptr_t)(*(start - 2)) - 11);

				//		if (*(start + 1) == 0x46)	// r14
				//			memcpy(start - 1, "/x4C/x89/xF1", 3);

				//		// mov rcx, r??
				//		// call ....
				//		voltek::detours_function_class_call((uintptr_t)(start + 2), TESObjectREFR_base_Extremly::IncRefCount);

				//		total++;
				//	}
				//}

				//_CONSOLE("Inc dbg 1 %llu", total);
			}

			void ReplaceBSPointerHandleAndManagerPatch::DecRefPatch_980()
			{

			}
		}
	}
}