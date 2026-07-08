# ==========================================================================
#  xwin-toolchain.cmake
#  Cross-compile the CKPE Windows PE artifacts FROM LINUX using
#  clang-cl + lld-link + llvm-rc against Microsoft's real CRT / Win10 SDK,
#  splatted into $XWIN_DIR by the `xwin` tool.
#
#  This file is completely independent of the Visual Studio .sln / .vcxproj
#  build. Nothing under the MSBuild tree references it, and it never edits
#  any source. Windows developers are entirely unaffected.
#
#  Usage:
#    cmake -S linux-build -B build-linux -G Ninja \
#      -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/xwin-toolchain.cmake \
#      -DXWIN_DIR=$HOME/.xwin
# ==========================================================================

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# --- Locate the xwin splat (overridable: -DXWIN_DIR=/path) -----------------
if(NOT DEFINED XWIN_DIR)
    if(DEFINED ENV{XWIN_DIR})
        set(XWIN_DIR "$ENV{XWIN_DIR}")
    else()
        set(XWIN_DIR "$ENV{HOME}/.xwin")
    endif()
endif()

if(NOT EXISTS "${XWIN_DIR}/crt/include")
    message(FATAL_ERROR
        "xwin splat not found at '${XWIN_DIR}'.\n"
        "Run: xwin --accept-license --arch x86_64 splat --output ${XWIN_DIR}\n"
        "or pass -DXWIN_DIR=/path/to/splat")
endif()

# --- Toolchain binaries ----------------------------------------------------
set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_RC_COMPILER  llvm-rc)
set(CMAKE_AR           llvm-lib)
set(CMAKE_LINKER       lld-link)

# clang-cl must be told it is targeting the MSVC ABI on x64.
set(CMAKE_C_COMPILER_TARGET   x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

# The compiler-detection check would otherwise try to LINK a test exe with
# CMake's default debug dynamic CRT (msvcrtd.lib), which the xwin splat does
# not ship. Compile-only detection avoids that; real targets link fine.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# --- Header search: MSVC CRT/STL + Windows SDK (as SYSTEM includes) ---------
# /imsvc marks these as system headers so their warnings stay quiet.
set(_XWIN_INCLUDES
    "/imsvc${XWIN_DIR}/crt/include"
    "/imsvc${XWIN_DIR}/sdk/include/ucrt"
    "/imsvc${XWIN_DIR}/sdk/include/um"
    "/imsvc${XWIN_DIR}/sdk/include/shared"
    "/imsvc${XWIN_DIR}/sdk/include/winrt")
list(JOIN _XWIN_INCLUDES " " _XWIN_INCLUDES_STR)

set(_XWIN_COMMON_FLAGS "--target=x86_64-pc-windows-msvc ${_XWIN_INCLUDES_STR}")
set(CMAKE_C_FLAGS_INIT   "${_XWIN_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${_XWIN_COMMON_FLAGS}")

# --- Library search: MSVC CRT + Windows SDK import libs ---------------------
set(_XWIN_LIBPATHS
    "/libpath:${XWIN_DIR}/crt/lib/x86_64"
    "/libpath:${XWIN_DIR}/sdk/lib/ucrt/x86_64"
    "/libpath:${XWIN_DIR}/sdk/lib/um/x86_64")
list(JOIN _XWIN_LIBPATHS " " _XWIN_LIBPATHS_STR)

# lld-link is driven through clang-cl; pass linker args via the driver.
set(_XWIN_LINK_FLAGS "/manifest:no ${_XWIN_LIBPATHS_STR}")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_XWIN_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_XWIN_LINK_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_XWIN_LINK_FLAGS}")

# --- Resource compiler (llvm-rc) needs the SDK/CRT include dirs -------------
set(CMAKE_RC_FLAGS_INIT
    "-I ${XWIN_DIR}/sdk/include/um -I ${XWIN_DIR}/sdk/include/shared -I ${XWIN_DIR}/crt/include")

# --- Only resolve headers/libs inside the xwin sysroot ---------------------
set(CMAKE_FIND_ROOT_PATH "${XWIN_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # host tools (clang-cl etc.)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
