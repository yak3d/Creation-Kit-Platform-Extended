// ==========================================================================
//  vcomp_serial_shim.cpp
//
//  Serial re-implementation of the small slice of Microsoft's OpenMP runtime
//  ABI (_vcomp_*) that the PREBUILT Dependencies/DirectXTex/lib/DirectXTex.lib
//  references from exactly one member (DirectXTexCompress.obj -- the CPU BCn
//  block compressor used by Fallout4/SkyrimSE Facegen texture bake).
//
//  The xwin cross-toolchain does not ship VCOMP.lib / vcomp140.dll, and clang's
//  own OpenMP (libomp / __kmpc_*) is a different ABI. Rather than depend on a
//  runtime relic whose packaging varies across Wine/Proton builds, we satisfy
//  the _vcomp_* references ourselves by running each parallel region on a single
//  thread. The compressed loop is over independent 4x4 blocks with one 64-bit
//  atomic accumulator -- serial execution yields byte-identical output.
//
//  This file exists ONLY in the Linux/CMake cross-build. It is never referenced
//  by the Visual Studio .sln / .vcxproj, so the Windows build is untouched. As a
//  second safeguard, the whole body is compiled out unless the compiler is clang
//  (clang-cl), so it can never contribute symbols to a real MSVC build.
// ==========================================================================

#if defined(__clang__)

#include <windows.h>
#include <stdarg.h>

extern "C" {

// Linkage sentinel: MSVC-compiled objects reference this to force vcomp linkage.
// Defining it resolves the reference; the value/type is irrelevant.
int _You_must_link_with_Microsoft_OpenMP_library = 0;

// `#pragma omp for` static (simple) schedule split. With a team of one, the
// single thread owns the entire iteration range. NOTE: [begin,end] is INCLUSIVE
// in this ABI -- return [first,last] verbatim (do NOT subtract 1).
void __cdecl _vcomp_for_static_simple_init(unsigned int first,
                                           unsigned int last,
                                           int step,
                                           BOOL increment,
                                           unsigned int* begin,
                                           unsigned int* end)
{
    (void)step;
    (void)increment;
    *begin = first;
    *end   = last;
}

// End-of-loop barrier. Team of one => nothing to wait on.
void __cdecl _vcomp_for_static_end(void) {}

// OpenMP flush. Single thread => no cross-thread visibility problem; a full
// fence is harmless and keeps the memory model honest.
void __cdecl _vcomp_flush(void) { MemoryBarrier(); }

// 64-bit atomic add ("i8" = 8 *bytes*, i.e. int64 -- not int8). Serial => a
// plain add is correct and sufficient.
void __cdecl _vcomp_atomic_add_i8(LONG64* dest, LONG64 val) { *dest += val; }

// Fork a parallel region. In MSVC's ABI this spins up the thread-team and each
// member calls the compiler-outlined region `wrapper` with `nargs` captured
// args (all pointer-width on x64). Serial behaviour: call `wrapper` exactly once
// with those args. Forwarding a runtime-count varargs list can't be done by
// re-emitting varargs in portable C, so we read them into a pointer array and
// dispatch through a fixed-arity cast (ABI-correct since every arg is void*).
void __cdecl _vcomp_fork(BOOL ifval, int nargs, void* wrapper, ...)
{
    (void)ifval;   // whether-to-parallelize; the serial path always runs inline

    void* a[16] = { 0 };   // DirectXTex's outlined region captures only a few
    va_list ap;
    va_start(ap, wrapper);
    for (int i = 0; i < nargs && i < 16; ++i)
        a[i] = va_arg(ap, void*);
    va_end(ap);

    switch (nargs)
    {
    case 0: ((void(__cdecl*)(void))wrapper)(); break;
    case 1: ((void(__cdecl*)(void*))wrapper)(a[0]); break;
    case 2: ((void(__cdecl*)(void*, void*))wrapper)(a[0], a[1]); break;
    case 3: ((void(__cdecl*)(void*, void*, void*))wrapper)(a[0], a[1], a[2]); break;
    case 4: ((void(__cdecl*)(void*, void*, void*, void*))wrapper)(a[0], a[1], a[2], a[3]); break;
    case 5: ((void(__cdecl*)(void*, void*, void*, void*, void*))wrapper)(a[0], a[1], a[2], a[3], a[4]); break;
    case 6: ((void(__cdecl*)(void*, void*, void*, void*, void*, void*))wrapper)(a[0], a[1], a[2], a[3], a[4], a[5]); break;
    case 7: ((void(__cdecl*)(void*, void*, void*, void*, void*, void*, void*))wrapper)(a[0], a[1], a[2], a[3], a[4], a[5], a[6]); break;
    case 8: ((void(__cdecl*)(void*, void*, void*, void*, void*, void*, void*, void*))wrapper)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]); break;
    default:
        // DirectXTexCompress captures < 8 args; if this ever fires, widen the
        // switch (and the array). Fail loud rather than silently mis-call.
        __debugbreak();
        break;
    }
}

} // extern "C"

#endif // defined(__clang__)
