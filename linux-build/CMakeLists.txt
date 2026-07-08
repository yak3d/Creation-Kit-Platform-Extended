# ==========================================================================
#  CKPE Linux cross-build (parallel to the Visual Studio .sln)
#
#  This tree exists ONLY to cross-compile the Windows PE artifacts on Linux
#  via cmake/xwin-toolchain.cmake (clang-cl + lld-link + llvm-rc + xwin).
#  It never touches or references the .sln / .vcxproj MSBuild files.
#
# ==========================================================================
cmake_minimum_required(VERSION 3.21)

# Manage the MSVC runtime library via CMake (CMP0091 NEW) so we select the
# static RELEASE CRT (libcmt.lib) that matches RuntimeLibrary=MultiThreaded in
# the .vcxproj -- and that the xwin splat actually ships.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

project(ckpe_linux_crossbuild
    LANGUAGES CXX C RC
    DESCRIPTION "Linux cross-compilation of Creation Kit Platform Extended (Windows PE)")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR
        "This build must be configured with the xwin cross toolchain:\n"
        "  -DCMAKE_TOOLCHAIN_FILE=<repo>/cmake/xwin-toolchain.cmake")
endif()

# Repository root (this file lives in <repo>/linux-build/).
get_filename_component(CKPE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# llvm-rc on Linux cannot resolve the .rc's Windows-style relative icon path
# ("..\\Resources\\ckpe.ico"). Generate a build-tree copy of the .rc with that
# rewritten to an absolute, forward-slash path. The original .rc (shared with
# the MSBuild/Windows build) is never modified.
set(_loader_rc_src "${CKPE_ROOT}/CKPE.Loader/ckpe_loader.rc")
set(_loader_rc_gen "${CMAKE_CURRENT_BINARY_DIR}/ckpe_loader.gen.rc")
file(READ "${_loader_rc_src}" _loader_rc_text)
string(REPLACE "..\\\\Resources\\\\ckpe.ico"
               "${CKPE_ROOT}/Resources/ckpe.ico"
               _loader_rc_text "${_loader_rc_text}")
file(WRITE "${_loader_rc_gen}" "${_loader_rc_text}")

add_executable(ckpe_loader
    "${CKPE_ROOT}/CKPE.Loader/ckpe_loader.cpp"
    "${_loader_rc_gen}")

set_target_properties(ckpe_loader PROPERTIES
    OUTPUT_NAME "ckpe_loader"
    SUFFIX ".exe")

# LanguageStandard=stdcpplatest ; ConformanceMode ; optimizations.
# (Static CRT is selected via CMAKE_MSVC_RUNTIME_LIBRARY above, not /MT here.)
target_compile_options(ckpe_loader PRIVATE
    /std:c++latest
    /permissive-
    /EHsc
    /GR
    /O2 /Ob2 /Oi /Gy)

# CharacterSet=Unicode + the vcxproj preprocessor defines
target_compile_definitions(ckpe_loader PRIVATE
    UNICODE _UNICODE
    NDEBUG _CONSOLE _CRT_SECURE_NO_WARNINGS)

# SubSystem=Console ; OptimizeReferences + COMDAT folding ; no incremental
target_link_options(ckpe_loader PRIVATE
    /SUBSYSTEM:CONSOLE
    /OPT:REF /OPT:ICF
    /INCREMENTAL:NO)

# The generated .rc still #includes "resource.h" from the loader source dir,
# so llvm-rc needs that directory on its include path.
set_source_files_properties(
    "${_loader_rc_gen}"
    PROPERTIES INCLUDE_DIRECTORIES "${CKPE_ROOT}/CKPE.Loader")

set(CKPE_DEPS "${CKPE_ROOT}/Dependencies")

add_library(detours STATIC
    "${CKPE_DEPS}/Detours/detours/Detours.cpp"
    "${CKPE_DEPS}/Detours/detours/Detours32.cpp"
    "${CKPE_DEPS}/Detours/detours/Detours64.cpp"
    "${CKPE_DEPS}/Detours/detours/HideStaticLibSymbols.c"
    "${CKPE_DEPS}/Detours/detours/stdafx.cpp")
target_include_directories(detours PUBLIC
    "${CKPE_DEPS}/Detours/detours"
    "${CKPE_DEPS}/Detours/detours/zydis/msvc"
    "${CKPE_DEPS}/Detours/detours/zydis/src"
    "${CKPE_DEPS}/Detours/detours/zydis/dependencies/zycore/include"
    "${CKPE_DEPS}/Detours/detours/zydis/include")
target_compile_definitions(detours PRIVATE
    ZYDIS_STATIC_DEFINE WIN32 NDEBUG _LIB UNICODE _UNICODE)
target_compile_options(detours PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/std:c++latest> /O2 /Ob2 /Oi /Gy)

add_library(libzip STATIC "${CKPE_DEPS}/mzip/src/zip.c")
target_include_directories(libzip PUBLIC "${CKPE_DEPS}/mzip/src")
target_compile_definitions(libzip PRIVATE NDEBUG _LIB _CRT_SECURE_NO_WARNINGS)
target_compile_options(libzip PRIVATE /O2 /Ob2 /Oi /Gy)

file(GLOB _ckpe_src "${CKPE_ROOT}/CKPE/Src/*.cpp")
add_library(CKPE SHARED
    ${_ckpe_src}
    "${CKPE_ROOT}/CKPE/dllmain.cpp"
    "${CKPE_DEPS}/iw/iw.cpp"
    "${CKPE_ROOT}/CKPE/resource_version.rc")
set_target_properties(CKPE PROPERTIES OUTPUT_NAME "CKPE" PREFIX "")

target_include_directories(CKPE PRIVATE
    "${CKPE_ROOT}/CKPE/Include"
    "${CKPE_DEPS}/iw"
    "${CKPE_DEPS}/Detours"
    "${CKPE_DEPS}/mzip/src")
target_compile_definitions(CKPE PRIVATE
    NDEBUG CKPE_EXPORTS _WINDOWS _USRDLL
    WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS
    _SILENCE_CXX23_DENORM_DEPRECATION_WARNING
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
target_compile_options(CKPE PRIVATE
    /std:c++latest /permissive- /EHsc /GR /O2 /Ob2 /Oi /Gy)
# resource_version.rc #includes "resource_version.h" from the CKPE dir.
set_source_files_properties("${CKPE_ROOT}/CKPE/resource_version.rc"
    PROPERTIES INCLUDE_DIRECTORIES "${CKPE_ROOT}/CKPE")

# NOTE: lib names are lowercased because the xwin splat + case-sensitive Linux
# FS only provide lowercase import libs (Shell32.lib -> shell32.lib). MSVC is
# case-insensitive, so the Windows .vcxproj (Shell32.lib etc.) is unaffected.
target_link_libraries(CKPE PRIVATE
    detours libzip
    shell32.lib msimg32.lib gdiplus.lib comctl32.lib shlwapi.lib netapi32.lib)
target_link_options(CKPE PRIVATE /OPT:REF /OPT:ICF /INCREMENTAL:NO)

# This .rc #includes "resource_version2.h", which the PowerShell prebuild emits
# as UTF-16 LE -- llvm-rc's preprocessor cannot read that. A quote-include is
# resolved against the .rc's own directory first, so we place a UTF-8-converted
# copy of the header beside a build-tree copy of the .rc. Originals untouched;
# the committed build number is used as-is (no PowerShell).
set(_wh_rc_dir "${CMAKE_CURRENT_BINARY_DIR}/winhttp_rc")
file(MAKE_DIRECTORY "${_wh_rc_dir}")
execute_process(
    COMMAND iconv -f UTF-16 -t UTF-8
            "${CKPE_ROOT}/CKPE.WinHttp/resource_version2.h"
    OUTPUT_FILE "${_wh_rc_dir}/resource_version2.h"
    RESULT_VARIABLE _iconv_rc)
if(NOT _iconv_rc EQUAL 0)
    message(FATAL_ERROR "iconv failed to convert resource_version2.h (rc=${_iconv_rc})")
endif()
configure_file("${CKPE_ROOT}/CKPE.WinHttp/resource_version.rc"
               "${_wh_rc_dir}/resource_version.rc" COPYONLY)

add_library(winhttp SHARED
    "${CKPE_ROOT}/CKPE.WinHttp/CKPE.SteamAPI.cpp"
    "${CKPE_ROOT}/CKPE.WinHttp/CKPE.WinHttp.Native.cpp"
    "${CKPE_ROOT}/CKPE.WinHttp/dllmain.cpp"
    "${CKPE_ROOT}/CKPE.WinHttp/exports.cpp"
    "${_wh_rc_dir}/resource_version.rc")
set_target_properties(winhttp PROPERTIES OUTPUT_NAME "winhttp" PREFIX "")

target_include_directories(winhttp PRIVATE
    "${CKPE_ROOT}/CKPE/Include"
    "${CKPE_ROOT}/CKPE.WinHttp")
target_compile_definitions(winhttp PRIVATE
    NDEBUG CKPEWINHTTP_EXPORTS _WINDOWS _USRDLL
    WIN32_LEAN_AND_MEAN NOMINMAX CREATIOKITPLATFORMEXTENDEDCORE_EXPORTS
    _SILENCE_CXX23_DENORM_DEPRECATION_WARNING
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
target_compile_options(winhttp PRIVATE
    /std:c++latest /permissive- /EHsc /GR /O2 /Ob2 /Oi /Gy)

target_link_libraries(winhttp PRIVATE CKPE)
target_link_options(winhttp PRIVATE /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /INCREMENTAL:NO)

# Toml.lib  (toml11 submodule) — Dependencies/Toml.vcxproj
file(GLOB _toml_src "${CKPE_DEPS}/toml11/src/*.cpp")
add_library(Toml STATIC ${_toml_src})
target_include_directories(Toml PUBLIC "${CKPE_DEPS}/toml11/include")
target_compile_definitions(Toml PRIVATE NDEBUG _LIB TOML11_COMPILE_SOURCES)
target_compile_options(Toml PRIVATE /std:c++latest /O2 /Ob2 /Oi /Gy)

# libzydis.lib  (in-tree Dependencies/zydis) — Dependencies/libzydis.vcxproj
file(GLOB _zydis_src "${CKPE_DEPS}/zydis/src/*.c")
add_library(libzydis STATIC ${_zydis_src})
target_include_directories(libzydis PUBLIC
    "${CKPE_DEPS}/zydis/include" "${CKPE_DEPS}/zydis/msvc" "${CKPE_DEPS}/zydis/src")
target_compile_definitions(libzydis PRIVATE NDEBUG _LIB ZYDIS_STATIC_DEFINE NOMINMAX)
target_compile_options(libzydis PRIVATE /O2 /Ob2 /Oi /Gy)

# vmm.lib  (in-tree Dependencies/vmm — Voltek memory manager) — vmm/vmm.vcxproj (x64 static)
add_library(vmm STATIC
    "${CKPE_DEPS}/vmm/dllmain.cpp"
    "${CKPE_DEPS}/vmm/iw/iw.cpp"
    "${CKPE_DEPS}/vmm/source/valloc.cpp"
    "${CKPE_DEPS}/vmm/source/vassert.cpp"
    "${CKPE_DEPS}/vmm/source/vbase.cpp"
    "${CKPE_DEPS}/vmm/source/vbits.cpp"
    "${CKPE_DEPS}/vmm/source/vio.cpp"
    "${CKPE_DEPS}/vmm/source/vmapper.cpp"
    "${CKPE_DEPS}/vmm/source/vmm.cpp"
    "${CKPE_DEPS}/vmm/source/vmmmain.cpp"
    "${CKPE_DEPS}/vmm/source/vsimplelock.cpp")
target_include_directories(vmm PUBLIC "${CKPE_DEPS}/vmm/include")
target_compile_definitions(vmm PRIVATE
    NDEBUG NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS _WINDOWS VOLTEK_LIB_BUILD)
target_compile_options(vmm PRIVATE /std:c++latest /O2 /Ob2 /Oi /Gy)

# Two .rc files need build-tree copies:
#  - resource.rc embeds ~17 bitmaps/PNGs/icon via Windows-relative "..\\Resources\\" paths
#    (llvm-rc on Linux can't resolve them) -> rewrite to absolute forward-slash paths.
#  - resource_version.rc includes UTF-16LE resource_version2.h -> iconv to UTF-8 alongside.
set(_cm_rc_dir "${CMAKE_CURRENT_BINARY_DIR}/common_rc")
file(MAKE_DIRECTORY "${_cm_rc_dir}")
# (a) resource.rc with absolute resource paths
file(READ "${CKPE_ROOT}/CKPE.Common/resource.rc" _cm_res_text)
string(REPLACE "..\\\\Resources\\\\" "${CKPE_ROOT}/Resources/" _cm_res_text "${_cm_res_text}")
# Strip non-printable-ASCII bytes: llvm-rc (via CMake's 2-step preprocess) can't
# interpret the 4-byte emoji in one crash-dialog string under the default codepage.
# Only affected: that decorative glyph + Cyrillic in COMMENTS (stripped harmlessly).
string(REGEX REPLACE "[^\t\r\n -~]" "" _cm_res_text "${_cm_res_text}")
file(WRITE "${_cm_rc_dir}/resource.rc" "${_cm_res_text}")
# (b) UTF-8 resource_version2.h + a copy of resource_version.rc beside it
execute_process(
    COMMAND iconv -f UTF-16 -t UTF-8 "${CKPE_ROOT}/CKPE.Common/resource_version2.h"
    OUTPUT_FILE "${_cm_rc_dir}/resource_version2.h" RESULT_VARIABLE _cm_iconv)
if(NOT _cm_iconv EQUAL 0)
    message(FATAL_ERROR "iconv failed on CKPE.Common/resource_version2.h (rc=${_cm_iconv})")
endif()
configure_file("${CKPE_ROOT}/CKPE.Common/resource_version.rc"
               "${_cm_rc_dir}/resource_version.rc" COPYONLY)

file(GLOB _common_src "${CKPE_ROOT}/CKPE.Common/Src/*.cpp")
list(REMOVE_ITEM _common_src
    "${CKPE_ROOT}/CKPE.Common/Src/CKPE.Common.GenerateTableID.cpp"
    "${CKPE_ROOT}/CKPE.Common/Src/CKPE.Common.TableID.cpp")

add_library(CKPE.Common SHARED
    ${_common_src}
    "${CKPE_ROOT}/CKPE.Common/dllmain.cpp"
    "${CKPE_DEPS}/jDialogs/include/jdialogs.cpp"
    "${_cm_rc_dir}/resource.rc"
    "${_cm_rc_dir}/resource_version.rc")
set_target_properties(CKPE.Common PROPERTIES OUTPUT_NAME "CKPE.Common" PREFIX "")

target_include_directories(CKPE.Common PRIVATE
    "${CKPE_DEPS}/DirectXTex/include"
    "${CKPE_ROOT}/CKPE/Include"
    "${CKPE_ROOT}/CKPE.Common/Include"
    "${CKPE_DEPS}/zydis/msvc"
    "${CKPE_DEPS}/zydis/include"
    "${CKPE_DEPS}/toml11/include"
    "${CKPE_DEPS}/jDialogs/include"
    "${CKPE_DEPS}/mzip/src"
    "${CKPE_ROOT}/CKPE.Common"
    "${CKPE_DEPS}/vmm/include")
target_compile_definitions(CKPE.Common PRIVATE
    NDEBUG ZYDIS_STATIC_DEFINE CKPECOMMON_EXPORTS _WINDOWS _USRDLL
    WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS CKPE_NO_LOGGER_FUNCTION
    VOLTEK_LIB_BUILD MIMALLOC_PURGE_DECOMMITS=1
    _SILENCE_CXX23_DENORM_DEPRECATION_WARNING _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
target_compile_options(CKPE.Common PRIVATE
    /std:c++latest /permissive- /EHsc /GR /O2 /Ob2 /Oi /Gy)
# resource.rc still #includes "resource.h" from the module dir.
set_source_files_properties("${_cm_rc_dir}/resource.rc"
    PROPERTIES INCLUDE_DIRECTORIES "${CKPE_ROOT}/CKPE.Common")

target_link_libraries(CKPE.Common PRIVATE
    CKPE Toml libzydis libzip vmm
    "${CKPE_DEPS}/DirectXTex/lib/DirectXTex.lib"
    shell32.lib comctl32.lib msimg32.lib uxtheme.lib gdiplus.lib shlwapi.lib
    dwmapi.lib dbghelp.lib dxgi.lib d3d11.lib d3dcompiler.lib dxguid.lib)
target_link_options(CKPE.Common PRIVATE /OPT:REF /OPT:ICF /INCREMENTAL:NO)

# --------------------------------------------------------------------------
#  Helper: prepare a module's .rc files for llvm-rc into a build-tree dir.
#  - UTF-16LE resource_version2.h -> UTF-8 (iconv)
#  - resource.rc: "..\\Resources\\" -> absolute path + strip non-printable-ASCII
#  Sets <name>_RC (list of generated .rc) in the caller scope.
# --------------------------------------------------------------------------
function(ckpe_prepare_rc name module_dir)
    set(gendir "${CMAKE_CURRENT_BINARY_DIR}/${name}_rc")
    file(MAKE_DIRECTORY "${gendir}")
    set(rcs "")
    if(EXISTS "${module_dir}/resource_version2.h")
        execute_process(
            COMMAND iconv -f UTF-16 -t UTF-8 "${module_dir}/resource_version2.h"
            OUTPUT_FILE "${gendir}/resource_version2.h" RESULT_VARIABLE _r)
        if(NOT _r EQUAL 0)
            message(FATAL_ERROR "iconv failed: ${module_dir}/resource_version2.h")
        endif()
    endif()
    if(EXISTS "${module_dir}/resource_version.rc")
        configure_file("${module_dir}/resource_version.rc" "${gendir}/resource_version.rc" COPYONLY)
        list(APPEND rcs "${gendir}/resource_version.rc")
    endif()
    if(EXISTS "${module_dir}/resource.rc")
        file(READ "${module_dir}/resource.rc" _t)
        string(REPLACE "..\\\\Resources\\\\" "${CKPE_ROOT}/Resources/" _t "${_t}")
        string(REGEX REPLACE "[^\t\r\n -~]" "" _t "${_t}")
        file(WRITE "${gendir}/resource.rc" "${_t}")
        list(APPEND rcs "${gendir}/resource.rc")
    endif()
    # resource.h / resource_version.h are #included from the module dir.
    set_source_files_properties(${rcs} PROPERTIES INCLUDE_DIRECTORIES "${module_dir}")
    set(${name}_RC "${rcs}" PARENT_SCOPE)
endfunction()

# The prebuilt DirectXTex.lib carries a /DEFAULTLIB:VCOMP directive (built with
# OpenMP), and lld-link's /NODEFAULTLIB matching won't suppress it reliably.
# Provide a near-empty stub VCOMP.lib so the open succeeds; FO4/SkyrimSE don't
# reference the OpenMP-using DirectXTex paths, so the _vcomp_* refs dead-strip.
set(_vcomp_stub_dir "${CMAKE_CURRENT_BINARY_DIR}/stublibs")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vcomp_stub.c" "void __ckpe_vcomp_stub(void){}\n")
add_library(vcomp_stub STATIC "${CMAKE_CURRENT_BINARY_DIR}/vcomp_stub.c")
set_target_properties(vcomp_stub PROPERTIES
    OUTPUT_NAME "VCOMP" ARCHIVE_OUTPUT_DIRECTORY "${_vcomp_stub_dir}")

# libdeflate.lib  (submodule) — Dependencies/libdeflate.vcxproj
add_library(libdeflate STATIC
    "${CKPE_DEPS}/libdeflate/lib/adler32.c"
    "${CKPE_DEPS}/libdeflate/lib/crc32.c"
    "${CKPE_DEPS}/libdeflate/lib/deflate_compress.c"
    "${CKPE_DEPS}/libdeflate/lib/deflate_decompress.c"
    "${CKPE_DEPS}/libdeflate/lib/gzip_compress.c"
    "${CKPE_DEPS}/libdeflate/lib/gzip_decompress.c"
    "${CKPE_DEPS}/libdeflate/lib/utils.c"
    "${CKPE_DEPS}/libdeflate/lib/x86/cpu_features.c"
    "${CKPE_DEPS}/libdeflate/lib/zlib_compress.c"
    "${CKPE_DEPS}/libdeflate/lib/zlib_decompress.c")
target_include_directories(libdeflate PUBLIC "${CKPE_DEPS}/libdeflate")
target_compile_definitions(libdeflate PRIVATE NDEBUG _LIB)
target_compile_options(libdeflate PRIVATE /O2 /Ob2 /Oi /Gy)

# ImGUI.lib  (in-tree Dependencies/ImGUI) — Dependencies/ImGUI/ImGUI.vcxproj
add_library(ImGUI STATIC
    "${CKPE_DEPS}/ImGUI/imgui.cpp"
    "${CKPE_DEPS}/ImGUI/imgui_demo.cpp"
    "${CKPE_DEPS}/ImGUI/imgui_draw.cpp"
    "${CKPE_DEPS}/ImGUI/imgui_tables.cpp"
    "${CKPE_DEPS}/ImGUI/imgui_widgets.cpp"
    "${CKPE_DEPS}/ImGUI/backends/imgui_impl_dx11.cpp"
    "${CKPE_DEPS}/ImGUI/backends/imgui_impl_dx12.cpp"
    "${CKPE_DEPS}/ImGUI/backends/imgui_impl_win32.cpp")
target_include_directories(ImGUI PUBLIC "${CKPE_DEPS}/ImGUI")
target_compile_definitions(ImGUI PRIVATE NDEBUG _LIB)
target_compile_options(ImGUI PRIVATE /std:c++latest /O2 /Ob2 /Oi /Gy)

# --------------------------------------------------------------------------
#  CKPE.PluginAPI -> CKPE.PluginAPI.dll (+ import lib). Unicode, /MT.
# --------------------------------------------------------------------------
ckpe_prepare_rc(pluginapi "${CKPE_ROOT}/CKPE.PluginAPI")
add_library(CKPE.PluginAPI SHARED
    "${CKPE_ROOT}/CKPE.PluginAPI/dllmain.cpp"
    "${CKPE_ROOT}/CKPE.PluginAPI/Src/CKPE.PluginAPI.Plugin.cpp"
    "${CKPE_ROOT}/CKPE.PluginAPI/Src/CKPE.PluginAPI.PluginAPI.cpp"
    "${CKPE_ROOT}/CKPE.PluginAPI/Src/CKPE.PluginAPI.PluginManager.cpp"
    ${pluginapi_RC})
set_target_properties(CKPE.PluginAPI PROPERTIES OUTPUT_NAME "CKPE.PluginAPI" PREFIX "")
target_include_directories(CKPE.PluginAPI PRIVATE
    "${CKPE_ROOT}/CKPE/Include" "${CKPE_ROOT}/CKPE.PluginAPI/Include"
    "${CKPE_ROOT}/CKPE.Common/Include" "${CKPE_DEPS}/jDialogs/include")
target_compile_definitions(CKPE.PluginAPI PRIVATE
    UNICODE _UNICODE
    NDEBUG CKPEPLUGINAPI_EXPORTS _WINDOWS _USRDLL WIN32_LEAN_AND_MEAN NOMINMAX
    _CRT_SECURE_NO_WARNINGS _SILENCE_CXX23_DENORM_DEPRECATION_WARNING
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
target_compile_options(CKPE.PluginAPI PRIVATE /std:c++latest /permissive- /EHsc /GR /O2 /Ob2 /Oi /Gy)
target_link_libraries(CKPE.PluginAPI PRIVATE CKPE CKPE.Common)
target_link_options(CKPE.PluginAPI PRIVATE /OPT:REF /OPT:ICF /INCREMENTAL:NO)

# --------------------------------------------------------------------------
#  Per-game modules: CKPE.Fallout4 + CKPE.SkyrimSE (DynamicLibrary, MultiByte, /MT)
#  Mirror CKPE.Fallout4/.vcxproj & CKPE.SkyrimSE/.vcxproj (Release|x64).
# --------------------------------------------------------------------------
function(ckpe_add_game name export_def)
    set(moddir "${CKPE_ROOT}/${name}")
    ckpe_prepare_rc(${name} "${moddir}")
    file(GLOB_RECURSE _src "${moddir}/Src/*.cpp")
    # dllmain.cpp lives in the module root and #includes "resource_version2.h"
    # (UTF-16). Compile a copy from the rc gendir, which holds the UTF-8 version,
    # so its quote-include (resolved against the file's own dir) finds the UTF-8 one.
    configure_file("${moddir}/dllmain.cpp"
                   "${CMAKE_CURRENT_BINARY_DIR}/${name}_rc/dllmain.cpp" COPYONLY)
    add_library(${name} SHARED
        ${_src}
        "${CMAKE_CURRENT_BINARY_DIR}/${name}_rc/dllmain.cpp"
        "${CKPE_DEPS}/DirectXTex/src/ScreenGrab11.cpp"
        # Serial _vcomp_* shim: resolves the OpenMP refs the prebuilt
        # DirectXTex.lib pulls in (see linux-build/shim/vcomp_serial_shim.cpp).
        "${CMAKE_CURRENT_SOURCE_DIR}/shim/vcomp_serial_shim.cpp"
        # Explicit-instantiation shim for BSAutoLock (BSLock.h's extern template
        # has no backing definition; clang honours it strictly -> undefined ctor/dtor).
        "${CMAKE_CURRENT_SOURCE_DIR}/shim/ckpe_game_instantiations.cpp"
        ${${name}_RC})
    # Game namespace for the instantiation shim: CKPE.Fallout4 -> Fallout4 etc.
    string(REPLACE "CKPE." "" _game_ns "${name}")
    set_target_properties(${name} PROPERTIES OUTPUT_NAME "${name}" PREFIX "")
    target_include_directories(${name} PRIVATE
        # UTF-8 resource_version2.h (some .cpp #include it directly; the on-disk
        # module copy is UTF-16 which clang can't read) -- must precede moddir.
        "${CMAKE_CURRENT_BINARY_DIR}/${name}_rc"
        "${CKPE_DEPS}/DirectXTex/include"
        "${CKPE_ROOT}/CKPE/Include"
        "${moddir}/Include"
        "${CKPE_ROOT}/CKPE.Common/Include"
        "${CKPE_DEPS}/xbyak"
        "${moddir}"
        "${CKPE_DEPS}/ImGUI"
        "${CKPE_DEPS}/libdeflate"
        "${CKPE_ROOT}/CKPE.PluginAPI/Include")
    target_compile_definitions(${name} PRIVATE
        NDEBUG ${export_def} _WINDOWS _USRDLL WIN32_LEAN_AND_MEAN NOMINMAX
        _CRT_SECURE_NO_WARNINGS CKPE_NO_LOGGER_FUNCTION XBYAK_NO_OP_NAMES
        _SILENCE_CXX23_DENORM_DEPRECATION_WARNING _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
        CKPE_GAME_NS=${_game_ns})   # selects game namespace for the instantiation shim
    target_compile_options(${name} PRIVATE /std:c++latest /permissive- /EHsc /GR /O2 /Ob2 /Oi /Gy)
    target_link_libraries(${name} PRIVATE
        CKPE CKPE.Common CKPE.PluginAPI ImGUI libdeflate
        "${CKPE_DEPS}/DirectXTex/lib/DirectXTex.lib"
        comctl32.lib dxgi.lib d3d11.lib d3dcompiler.lib dxguid.lib shlwapi.lib)
    # The prebuilt DirectXTex.lib was built with OpenMP (/DEFAULTLIB:VCOMP), whose
    # runtime the xwin splat doesn't ship. Suppress it; if FO4/SkyrimSE don't
    # reference the OpenMP-using DirectXTex paths, the _vcomp_* refs dead-strip.
    add_dependencies(${name} vcomp_stub)
    target_link_options(${name} PRIVATE
        /OPT:REF /OPT:ICF /INCREMENTAL:NO
        "/LIBPATH:${_vcomp_stub_dir}")   # stub VCOMP.lib (see above)
endfunction()

ckpe_add_game(CKPE.Fallout4  CKPEFALLOUT4_EXPORTS)
ckpe_add_game(CKPE.SkyrimSE  CKPESKYRIMSE_EXPORTS)

ckpe_prepare_rc(CKPE.Starfield "${CKPE_ROOT}/CKPE.Starfield")
configure_file("${CKPE_ROOT}/CKPE.Starfield/dllmain.cpp"
               "${CMAKE_CURRENT_BINARY_DIR}/CKPE.Starfield_rc/dllmain.cpp" COPYONLY)
file(GLOB_RECURSE _sf_src "${CKPE_ROOT}/CKPE.Starfield/Src/*.cpp")
# CrashDump.cpp hardcodes  #include "..\\..\\CKPE.Starfield\\resource_version2.h"
# (the UTF-16 source), which clang resolves against the file's own dir before any
# -I can redirect it. Compile a rewritten copy in the rc gendir whose bare include
# resolves to the UTF-8 resource_version2.h there. (Other games use <angle> includes.)
set(_sf_crashdump "${CKPE_ROOT}/CKPE.Starfield/Src/Patches/CKPE.Starfield.Patch.CrashDump.cpp")
list(REMOVE_ITEM _sf_src "${_sf_crashdump}")
file(READ "${_sf_crashdump}" _sf_cd)
string(REPLACE "..\\..\\CKPE.Starfield\\resource_version2.h" "resource_version2.h" _sf_cd "${_sf_cd}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CKPE.Starfield_rc/CKPE.Starfield.Patch.CrashDump.cpp" "${_sf_cd}")
list(APPEND _sf_src "${CMAKE_CURRENT_BINARY_DIR}/CKPE.Starfield_rc/CKPE.Starfield.Patch.CrashDump.cpp")
add_library(CKPE.Starfield SHARED
    ${_sf_src}
    "${CMAKE_CURRENT_BINARY_DIR}/CKPE.Starfield_rc/dllmain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shim/vcomp_serial_shim.cpp"
    ${CKPE.Starfield_RC})
set_target_properties(CKPE.Starfield PROPERTIES OUTPUT_NAME "CKPE.Starfield" PREFIX "")
target_include_directories(CKPE.Starfield PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/CKPE.Starfield_rc"
    "${CKPE_DEPS}/DirectXTex/include"
    "${CKPE_ROOT}/CKPE/Include"
    "${CKPE_ROOT}/CKPE.Starfield/Include"
    "${CKPE_ROOT}/CKPE.Common/Include"
    "${CKPE_DEPS}/xbyak"
    "${CKPE_ROOT}/CKPE.PluginAPI/Include"
    "${CKPE_DEPS}/qt5")
target_compile_definitions(CKPE.Starfield PRIVATE
    NDEBUG CKPESTARFIELD_EXPORTS _WINDOWS _USRDLL WIN32_LEAN_AND_MEAN NOMINMAX
    _CRT_SECURE_NO_WARNINGS CKPE_NO_LOGGER_FUNCTION XBYAK_NO_OP_NAMES
    _SILENCE_CXX23_DENORM_DEPRECATION_WARNING _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
target_compile_options(CKPE.Starfield PRIVATE /std:c++latest /permissive- /EHsc /GR /O2 /Ob2 /Oi /Gy
    # Qt 5.12 headers do bitwise ops between distinct enum types, which /std:c++latest
    # makes a hard error under clang. Downgrade (Qt-vs-c++latest; build-side only).
    -Wno-enum-enum-conversion -Wno-deprecated-enum-enum-conversion)
add_dependencies(CKPE.Starfield vcomp_stub)
target_link_libraries(CKPE.Starfield PRIVATE
    CKPE CKPE.Common CKPE.PluginAPI ImGUI libdeflate
    "${CKPE_DEPS}/DirectXTex/lib/DirectXTex.lib"
    "${CKPE_DEPS}/qt5/lib/Qt5Core.lib"
    "${CKPE_DEPS}/qt5/lib/Qt5Gui.lib"
    "${CKPE_DEPS}/qt5/lib/Qt5Widgets.lib"
    comctl32.lib dxgi.lib d3d11.lib d3dcompiler.lib dxguid.lib shlwapi.lib)
target_link_options(CKPE.Starfield PRIVATE
    /OPT:REF /OPT:ICF /INCREMENTAL:NO "/LIBPATH:${_vcomp_stub_dir}")

message(STATUS "CKPE cross-build configured. XWIN_DIR=${XWIN_DIR}")
