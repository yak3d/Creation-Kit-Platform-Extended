// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>
#include <CKPE.MessageBox.h>
#include <CKPE.Application.h>
#include <CKPE.Asserts.h>
#include <CKPE.ErrorHandler.h>
#include <CKPE.StringUtils.h>
#include <CKPE.PathUtils.h>
#include <CKPE.Utils.h>
#include <CKPE.Common.Interface.h>
#include <CKPE.Common.DialogManager.h>
#include <CKPE.Common.ModernTheme.h>
#include <CKPE.Common.UIListView.h>
#include <CKPE.Common.UITreeView.h>
#include <CKPE.PluginAPI.PluginManager.h>

namespace CKPE
{
	namespace PluginAPI
	{
		static CKPEPluginHandle _currentHandle = 0;
		static PluginManager _PluginManager{};

		static CKPERuntimeInterface _RuntimeInterface
		{
			CKPERuntimeInterface::kInterfaceVersion,
			_RuntimeInterface.IsUserUsingWine = []() { return CKPE_UserUseWine(); },
		};

		static CKPEMessageHookInterface _MessageHookInterface
		{
			CKPEMessageHookInterface::kInterfaceVersion,
			_MessageHookInterface.RegisterInitDialogHook =
				[](CKPEMessageHookInterface::OnInitDialogFn fn, void* ud)
				{
					Common::ModernTheme::RegisterInitDialogHook(
						reinterpret_cast<Common::ModernTheme::OnInitDialogFn>(fn), ud);
				},
			_MessageHookInterface.RegisterControlCreatedHook =
				[](CKPEMessageHookInterface::OnControlCreatedFn fn, void* ud)
				{
					Common::ModernTheme::RegisterControlCreatedHook(
						reinterpret_cast<Common::ModernTheme::OnControlCreatedFn>(fn), ud);
				},
		};

		static CKPEThemeOverrideInterface::CustomDrawFn _sLVCustomDraw = nullptr;
		static void* _sLVCustomDrawUd = nullptr;
		static CKPEThemeOverrideInterface::CustomDrawFn _sTVCustomDraw = nullptr;
		static void* _sTVCustomDrawUd = nullptr;

		static CKPEThemeOverrideInterface _ThemeOverrideInterface
		{
			CKPEThemeOverrideInterface::kInterfaceVersion,
			_ThemeOverrideInterface.SetListViewCustomDraw =
				[](CKPEThemeOverrideInterface::CustomDrawFn fn, void* ud)
				{
					_sLVCustomDraw = fn;
					_sLVCustomDrawUd = ud;
					Common::UI::ListView::InstallCustomDrawHandler(
						[](HWND wnd, LPNMLVCUSTOMDRAW lpcd, bool& bReturn) -> LRESULT {
							if (!_sLVCustomDraw) { bReturn = false; return 0; }
							bReturn = true;
							return _sLVCustomDraw(wnd, lpcd, &bReturn, _sLVCustomDrawUd);
						});
				},
			_ThemeOverrideInterface.SetTreeViewCustomDraw =
				[](CKPEThemeOverrideInterface::CustomDrawFn fn, void* ud)
				{
					_sTVCustomDraw = fn;
					_sTVCustomDrawUd = ud;
					Common::UI::TreeView::InstallCustomDrawHandler(
						[](HWND wnd, LPNMLVCUSTOMDRAW lpcd, bool& bReturn) -> LRESULT {
							if (!_sTVCustomDraw) { bReturn = false; return 0; }
							bReturn = true;
							return _sTVCustomDraw(wnd, lpcd, &bReturn, _sTVCustomDrawUd);
						});
				},
			_ThemeOverrideInterface.ClearListViewCustomDraw =
				[]() { _sLVCustomDraw = nullptr; Common::UI::ListView::InstallCustomDrawHandler(nullptr); },
			_ThemeOverrideInterface.ClearTreeViewCustomDraw =
				[]() { _sTVCustomDraw = nullptr; Common::UI::TreeView::InstallCustomDrawHandler(nullptr); },
		};

		static CKPEDialogManagerInterface _DialogManagerInterface
		{
			CKPEDialogManagerInterface::kInterfaceVersion,
			_DialogManagerInterface.HasDialog =
				[](const std::uintptr_t uid) { return Common::DialogManager::GetSingleton()->HasDialog(uid); },
			_DialogManagerInterface.AddDialog =
				[](const char* json_file, const std::uintptr_t uid) { return json_file && json_file[0] ?
				Common::DialogManager::GetSingleton()->AddDialog(json_file, uid) : false; },
			_DialogManagerInterface.AddDialogByCode =
				[](const char* json_code, const std::uintptr_t uid) { return json_code && json_code[0] ?
				Common::DialogManager::GetSingleton()->AddDialogByCode(json_code, uid) : false; },
			_DialogManagerInterface.LoadFromFilePackage =
				[](const char* filename)
			{
				if (!filename || !filename[0] || !CKPE::PathUtils::FileExists(filename) || 
					_stricmp(CKPE::PathUtils::ExtractFileExt(filename).c_str(), ".pak"))
					return false;

				Common::DialogManager::GetSingleton()->LoadFromFilePackage(filename);
				return true;
			},
		};

		void PluginManager::ReportPluginErrors(const std::vector<std::wstring>* v) const noexcept(true)
		{
			std::wstring message = L"A plug-in you have installed contains a DLL plugin that has failed to load correctly. "
				"If a new version of Creation Kit was just released, the plugin needs to be updated. "
				"Please check the mod's webpage for updates. This is not a problem with CKPE.\n";

			for (auto& plugin : *v)
				message += std::format(L"\n{}: no compatibility with current version CK", PathUtils::ExtractFileName(plugin));

			message += L"\n\nContinuing to load may result in lost save data or other undesired behavior.";
			message += L"\nExit Creation Kit? (yes highly suggested)";

			if (MessageBox::OpenWarning(message, MessageBox::mbYesNo) == MessageBox::mrYes)
				Common::Interface::GetSingleton()->GetApplication()->Terminate();
		}

		CKPEPluginHandle PluginManager::GetPluginHandle() noexcept(true)
		{
			CKPE_ASSERT_MSG(_currentHandle, "A plugin has called CKPEPluginInterface::GetPluginHandle outside of its Load handlers");

			return _currentHandle;
		}

		void* PluginManager::QueryInterface(std::uint32_t id) noexcept(true)
		{
			if (!_PluginManager._plugins || !_currentHandle)
				return nullptr;

			switch (id)
			{
			case kInterface_DialogManager:
				return (void*)&_DialogManagerInterface;
			case kInterface_Runtime:
				return (void*)&_RuntimeInterface;
			case kInterface_MessageHook:
				return (void*)&_MessageHookInterface;
			case kInterface_ThemeOverride:
				return (void*)&_ThemeOverrideInterface;
			default:
				return nullptr;
			}
		}

		PluginManager::PluginManager() noexcept(true) :
			_plugins(new std::vector<Plugin*>)
		{
			
		}

		PluginManager::~PluginManager() noexcept(true)
		{
			if (_plugins)
			{
				for (auto plugin : *_plugins)
				{
					if (plugin)
						delete plugin;
				}

				delete _plugins;
				_plugins = nullptr;
			}
		}

		std::uint32_t PluginManager::Search() noexcept(true)
		{
			if (!_plugins)
				return 0;

			auto path = PathUtils::GetCKPEPluginPath();
			CKPE::_MESSAGE(L"Scanning plugin directory: \"%s\"", path.c_str());

			std::vector<std::wstring> pluginInv;

			auto modules = PathUtils::GetFilesInDir(path, L".dll", true);
			for (auto& info : modules)
			{
				if (_plugins->size() == std::numeric_limits<CKPEPluginHandle>::max())
					break;

				auto plugin = new Plugin;

				auto result = plugin->CanLoad(info.first);
				if (result == Plugin::ErrorNoCompatibility)
					pluginInv.push_back(PathUtils::ExtractFileName(info.first));

				if (result == Plugin::NoError)
				{
					if (plugin->Load(info.first, false))
						_plugins->push_back(plugin);
					else
						delete plugin;
				}
				else
					delete plugin;
			}

			if (pluginInv.size())
				ReportPluginErrors(&pluginInv);

			return (CKPEPluginHandle)_plugins->size();
		}

		void PluginManager::InstallPlugins() noexcept(true)
		{
			if (!_plugins)
				return;

			auto s = Common::Interface::GetSingleton();
			_currentHandle = 1;

			for (auto plug : *_plugins)
			{
				_interface.CKPEVersion = s->GetCKPEInterface()->ckpeVersion;
				_interface.CKPECommonVersion = s->GetVersionDLL();
				_interface.CKPEGameLibraryVersion = s->GetGameLibraryVersionDLL();
				_interface.RuntimeVersion = s->GetEditorVersion();
				_interface.GetPluginHandle = GetPluginHandle;
				_interface.QueryInterface = QueryInterface;

				if (plug->Active((Common::RelocatorDB::PatchDB*)&_interface))
					_currentHandle++;
			}

			_currentHandle = 0;
		}

		PluginManager* PluginManager::GetSingleton() noexcept(true)
		{
			return &_PluginManager;
		}
	}
}