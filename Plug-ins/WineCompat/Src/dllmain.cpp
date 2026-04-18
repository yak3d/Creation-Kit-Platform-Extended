// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <windows.h>

#include <CKPE.PathUtils.h>
#include <CKPE.PluginAPI.PluginAPI.h>

#include <Plugin/StyleDispatcher.h>
#include <Theme/TreeViewCustomDraw.h>
#include <Iat/IatPatch.h>

extern "C"
{
    __declspec(dllexport) PluginAPI::CKPEPluginVersionData CKPEPlugin_Version =
    {
        CKPE::PluginAPI::CKPEPluginVersionData::kVersion,
        1,
        "WineCompat",
        "CKPE Contributors",
        static_cast<std::uint8_t>(
            CKPE::PluginAPI::CKPEPluginVersionData::kGameSkyrimSE |
            CKPE::PluginAPI::CKPEPluginVersionData::kGameFallout4  |
            CKPE::PluginAPI::CKPEPluginVersionData::kGameStarfield),
        0,
        { 0 },
        CKPE::PluginAPI::CKPEPluginVersionData::kAnyGames |
        CKPE::PluginAPI::CKPEPluginVersionData::kNoLinkedVersionGame
    };

    __declspec(dllexport) bool CKPEPlugin_HasDependencies() noexcept(true)
    {
        return false;
    }

    __declspec(dllexport) std::uint32_t CKPEPlugin_GetDependCount() noexcept(true)
    {
        return 0;
    }

    __declspec(dllexport) const char* CKPEPlugin_GetDependAt([[maybe_unused]] std::uint32_t id) noexcept(true)
    {
        return nullptr;
    }

    __declspec(dllexport) std::uint32_t CKPEPlugin_Load(
        const PluginAPI::CKPEPluginInterface* intf) noexcept(true)
    {
        auto sfname = PathUtils::GetCKPELogsPluginPath() + L"WineCompat.log";
        PathUtils::CreateFolder(PathUtils::ExtractFilePath(PathUtils::Normalize(sfname)));
        PluginAPI::UserPluginLogger.Open(sfname);

        auto* rt = static_cast<PluginAPI::CKPERuntimeInterface*>(
            intf->QueryInterface(PluginAPI::kInterface_Runtime));
        if (!rt || rt->InterfaceVersion != PluginAPI::CKPERuntimeInterface::kInterfaceVersion)
        {
            PluginAPI::_ERROR("WineCompat: CKPERuntimeInterface unavailable");
            return false;
        }

        if (!rt->IsUserUsingWine())
        {
            PluginAPI::_MESSAGE("WineCompat: not running under Wine, unloading");
            return true;
        }

        PluginAPI::_MESSAGE("WineCompat: Wine detected, installing overrides");

        auto* mh = static_cast<PluginAPI::CKPEMessageHookInterface*>(
            intf->QueryInterface(PluginAPI::kInterface_MessageHook));
        if (!mh || mh->InterfaceVersion != PluginAPI::CKPEMessageHookInterface::kInterfaceVersion)
        {
            PluginAPI::_ERROR("WineCompat: CKPEMessageHookInterface unavailable");
            return false;
        }

        auto* to = static_cast<PluginAPI::CKPEThemeOverrideInterface*>(
            intf->QueryInterface(PluginAPI::kInterface_ThemeOverride));
        if (!to || to->InterfaceVersion != PluginAPI::CKPEThemeOverrideInterface::kInterfaceVersion)
        {
            PluginAPI::_ERROR("WineCompat: CKPEThemeOverrideInterface unavailable");
            return false;
        }

        mh->RegisterInitDialogHook(OnInitDialog, nullptr);
        to->SetTreeViewCustomDraw(OnTreeViewCustomDraw, nullptr);

        PatchComctl32ImportTable();

        PluginAPI::_MESSAGE("WineCompat: hooks registered");
        return true;
    }
}

BOOL APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      [[maybe_unused]] LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
