﻿// Copyright © 2023-2024 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/gpl-3.0.html

#pragma once

namespace CreationKitPlatformExtended
{
	namespace Core
	{
		enum EDITOR_EXECUTABLE_TYPE {
			EDITOR_UNKNOWN,
			EDITOR_SKYRIM_SE_1_5_3,
			EDITOR_SKYRIM_SE_1_5_73,
			EDITOR_SKYRIM_SE_1_6_438,
			EDITOR_SKYRIM_SE_1_6_1130,
			EDITOR_SKYRIM_SE_1_6_1378_1,
			EDITOR_SKYRIM_SE_LAST = EDITOR_SKYRIM_SE_1_6_1378_1,
			EDITOR_FALLOUT_C4_1_10_162_0,
			EDITOR_FALLOUT_C4_1_10_943_1,
			EDITOR_FALLOUT_C4_1_10_982_3,
			EDITOR_FALLOUT_C4_LAST = EDITOR_FALLOUT_C4_1_10_982_3,
			EDITOR_STARFIELD_1_13_61_0,
			EDITOR_STARFIELD_1_14_70_0,
			EDITOR_STARFIELD_1_14_74_0,
			EDITOR_STARFIELD_1_14_78_0,
			EDITOR_STARFIELD_LAST = EDITOR_STARFIELD_1_14_78_0,
		};

		enum EDITOR_EXECUTABLE_SHORT_TYPE {
			EDITOR_SHORT_UNKNOWN,
			EDITOR_SHORT_SKYRIM_SE,
			EDITOR_SHORT_FALLOUT_C4,
			EDITOR_SHORT_STARFIELD,
		};

		// Не использовать менеджер на данном этапе, считаем, что его ещё нет
		// Связи с этим стандартные STL с обычным аллокатором

		// Список проверенных исполняемых файлов, допущенных к запуску
		static std::unordered_map<uint32_t, EDITOR_EXECUTABLE_TYPE> allowedEditorVersion = {
			//{ 0xA9CD60C7ul, EDITOR_SKYRIM_SE_1_5_3			},	// Default
			//{ 0xED7DB054ul, EDITOR_SKYRIM_SE_1_5_3			},	// Redirect Steam
			{ 0x624E8C84ul, EDITOR_SKYRIM_SE_1_5_73			},	// Default
			{ 0xF7E929A4ul, EDITOR_SKYRIM_SE_1_5_73			},	// Redirect Steam
			{ 0x668F3CB3ul, EDITOR_SKYRIM_SE_1_5_73			},	// Unoffical patch 6.3
			//{ 0x748A3CC4ul, EDITOR_SKYRIM_SE_1_6_438		},	// No Steam
			//{ 0x3FDB3994ul, EDITOR_SKYRIM_SE_1_6_438		},	// With Steam
			//{ 0xFDCAEE10ul, EDITOR_SKYRIM_SE_1_6_438		},	// No Steam and Redirect Steam
			{ 0x2EF668CEul, EDITOR_SKYRIM_SE_1_6_1130		},	// Redirect Steam
			{ 0x0085AA4Cul, EDITOR_SKYRIM_SE_1_6_1130		},	// Default
			{ 0x0085AA4Cul, EDITOR_SKYRIM_SE_1_6_1378_1		},	// Redirect Steam
			{ 0xDF67F346ul, EDITOR_FALLOUT_C4_1_10_162_0	},	// Default
			//{ 0x4BA2E086ul, EDITOR_FALLOUT_C4_1_10_943_1	},	// Redirect Steam
			//{ 0xBAAF1B07ul, EDITOR_FALLOUT_C4_1_10_943_1	},	// No Steam and Redirect Steam
			{ 0x481CCE95ul, EDITOR_FALLOUT_C4_1_10_982_3	},	// Redirect Steam
			{ 0x55F7F580ul, EDITOR_FALLOUT_C4_1_10_982_3	},	// No Steam and Redirect Steam
			//{ 0x6CDE4424ul, EDITOR_STARFIELD_1_13_61_0		},	// Redirect Steam
			//{ 0x8777A522ul, EDITOR_STARFIELD_1_14_70_0		},	// Redirect Steam
			{ 0x01BF6FB3ul, EDITOR_STARFIELD_1_14_74_0		},	// Redirect Steam
			{ 0x8C475320ul, EDITOR_STARFIELD_1_14_78_0		},	// Redirect Steam
		};

		// Список устаревших версий редакторов
		static std::vector<EDITOR_EXECUTABLE_TYPE> outdatedEditorVersion = {
			EDITOR_SKYRIM_SE_1_5_3,
			EDITOR_SKYRIM_SE_1_6_438,
			EDITOR_FALLOUT_C4_1_10_943_1,
			EDITOR_STARFIELD_1_13_61_0,
			EDITOR_STARFIELD_1_14_70_0,
			EDITOR_STARFIELD_1_14_74_0,
		};
		
		// Список ключевых смещений в исполняемых файлах, допущенных к запуску (но не точно)
		static std::unordered_map<uint32_t,
			std::pair<std::string_view, EDITOR_EXECUTABLE_TYPE>> allowedEditorVersion2 = {
			{ 0x3078988ul, { "1.5.3.0",		EDITOR_SKYRIM_SE_1_5_3			} },
			{ 0x3062CC8ul, { "1.5.73.0",	EDITOR_SKYRIM_SE_1_5_73			} },
			{ 0x2E835D8ul, { "1.6.438.0",	EDITOR_SKYRIM_SE_1_6_438		} },
			{ 0x2F3E698ul, { "1.6.1130.0",	EDITOR_SKYRIM_SE_1_6_1130		} },
			{ 0x2F7F018ul, { "1.6.1378.1",	EDITOR_SKYRIM_SE_1_6_1378_1		} },
			{ 0x3896168ul, { "1.10.162.0",	EDITOR_FALLOUT_C4_1_10_162_0	} },
			{ 0x2F8D1C8ul, { "1.10.943.1",	EDITOR_FALLOUT_C4_1_10_943_1	} },
			{ 0x2F8D298ul, { "1.10.982.3",	EDITOR_FALLOUT_C4_1_10_982_3	} },
			{ 0x86DD768ul, { "1.13.61.0",	EDITOR_STARFIELD_1_13_61_0		} },
			{ 0x873D2B8ul, { "1.14.70.0",	EDITOR_STARFIELD_1_14_70_0		} },
			{ 0x875F450ul, { "1.14.74.0",	EDITOR_STARFIELD_1_14_74_0		} },
			{ 0x875F550ul, { "1.14.78.0",	EDITOR_STARFIELD_1_14_78_0		} },
		};
		
		// Список названий редакторов
		static std::vector<std::string_view> allowedEditorVersionStr = {
			"Unknown version",
			"Skyrim Special Edition [v1.5.3]",
			"Skyrim Special Edition [v1.5.73]",
			"Skyrim Special Edition [v1.6.438]",
			"Skyrim Special Edition [v1.6.1130]",
			"Skyrim Special Edition [v1.6.1378.1]",
			"Fallout 4 [v1.10.162.0]",
			"Fallout 4 [v1.10.943.1]",
			"Fallout 4 [v1.10.982.3]",
			"Starfield [v1.13.61.0]",
			"Starfield [v1.14.70.0]",
			"Starfield [v1.14.74.0]",
			"Starfield [v1.14.78.0]",
		};

		// Список имён файлов базы данных
		static std::unordered_map<EDITOR_EXECUTABLE_TYPE, std::string_view> allowedDatabaseVersion = {
			//{ EDITOR_SKYRIM_SE_1_5_3,		"CreationKitPlatformExtended_SSE_1_5_3.database"		},
			{ EDITOR_SKYRIM_SE_1_5_73,		"CreationKitPlatformExtended_SSE_1_5_73.database"		},
			//{ EDITOR_SKYRIM_SE_1_6_438,		"CreationKitPlatformExtended_SSE_1_6_438.database"		},
			{ EDITOR_SKYRIM_SE_1_6_1130,	"CreationKitPlatformExtended_SSE_1_6_1130.database"		},
			{ EDITOR_SKYRIM_SE_1_6_1378_1,	"CreationKitPlatformExtended_SSE_1_6_1378_1.database"	},
			{ EDITOR_FALLOUT_C4_1_10_162_0,	"CreationKitPlatformExtended_FO4_1_10_162.database"		},
			//{ EDITOR_FALLOUT_C4_1_10_943_1,	"CreationKitPlatformExtended_FO4_1_10_943_1.database"	},
			{ EDITOR_FALLOUT_C4_1_10_982_3,	"CreationKitPlatformExtended_FO4_1_10_982_3.database"	},
			//{ EDITOR_STARFIELD_1_13_61_0,	"CreationKitPlatformExtended_SF_1_13_61_0.database"		},
			//{ EDITOR_STARFIELD_1_14_70_0,	"CreationKitPlatformExtended_SF_1_14_70_0.database"		},
			{ EDITOR_STARFIELD_1_14_74_0,	"CreationKitPlatformExtended_SF_1_14_74_0.database"		},
			{ EDITOR_STARFIELD_1_14_78_0,	"CreationKitPlatformExtended_SF_1_14_78_0.database"		},
		};

		// Список коротких названий поддерживаемых игр
		static std::unordered_map<EDITOR_EXECUTABLE_SHORT_TYPE, std::string_view> allowedShortNameGame =
		{
			{ EDITOR_SHORT_SKYRIM_SE,	"SSE" },
			{ EDITOR_SHORT_FALLOUT_C4,	"FO4" },
			{ EDITOR_SHORT_STARFIELD,	"SF"  },
		};

		// Список имён файлов внешних ресурсов для qt
		static std::unordered_map<EDITOR_EXECUTABLE_SHORT_TYPE, std::string_view> qtExternalResourcePackageFile = {
			{ EDITOR_SHORT_STARFIELD,	"CreationKitPlatformExtended_SF_QResources.pak"	},
		};

		static std::vector<std::string_view> allowedEditorFileNameStr = {
			"creationkit",
			//"creationkit_se_1_5_3",
			"creationkit_se_1_5_73",
			//"creationkit_se_1_6_438",
			"creationkit_se_1_6_1130",
			"creationkit_se_1_6_1378_1",
			"creationkit_f4_1_10_162",
			//"creationkit_f4_1_10_943_1",
			"creationkit_f4_1_10_982_3",
			//"creationkit_sf_1_13_61_0",
			//"creationkit_sf_1_14_70_0",
			"creationkit_sf_1_14_74_0",
			"creationkit_sf_1_14_78_0",
		};

		inline EDITOR_EXECUTABLE_SHORT_TYPE GetShortExecutableTypeFromFull(EDITOR_EXECUTABLE_TYPE editorVersion)
		{
			switch (editorVersion)
			{
			case EDITOR_SKYRIM_SE_1_5_3:
			case EDITOR_SKYRIM_SE_1_5_73:
			case EDITOR_SKYRIM_SE_1_6_438:
			case EDITOR_SKYRIM_SE_1_6_1130:
			case EDITOR_SKYRIM_SE_1_6_1378_1:
				return EDITOR_SHORT_SKYRIM_SE;
			case EDITOR_FALLOUT_C4_1_10_162_0:
			case EDITOR_FALLOUT_C4_1_10_943_1:
			case EDITOR_FALLOUT_C4_1_10_982_3:
				return EDITOR_SHORT_FALLOUT_C4;
			case EDITOR_STARFIELD_1_13_61_0:
			case EDITOR_STARFIELD_1_14_70_0:
			case EDITOR_STARFIELD_1_14_74_0:
			case EDITOR_STARFIELD_1_14_78_0:
				return EDITOR_SHORT_STARFIELD;
			default:
				return EDITOR_SHORT_UNKNOWN;
			}
		}

		inline bool CheckFileNameProcess(const char* name)
		{
			for (auto It = allowedEditorFileNameStr.begin();
				It != allowedEditorFileNameStr.end(); It++)
			{
				if (!strcmp(name, It->data()))
					return true;
			}

			return false;
		}

		inline bool CheckOutdatedEditor(EDITOR_EXECUTABLE_TYPE editorVersion)
		{
			return std::find(outdatedEditorVersion.begin(), outdatedEditorVersion.end(), editorVersion) != outdatedEditorVersion.end();
		}

		inline EDITOR_EXECUTABLE_TYPE GetEditorVersionByOffsetSign(uintptr_t basedAddress)
		{
			for (auto editorVersionIterator2 = allowedEditorVersion2.begin();
				editorVersionIterator2 != allowedEditorVersion2.end();
				editorVersionIterator2++)
			{
				// Защита в случаи выхода за пределы при проверке
				__try
				{
					// Сравнение по указанному смещению нужной строки
					if (!_stricmp((const char*)(basedAddress + editorVersionIterator2->first),
						editorVersionIterator2->second.first.data()))
						return editorVersionIterator2->second.second;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
				}
			}

			return EDITOR_UNKNOWN;
		}
	}
}