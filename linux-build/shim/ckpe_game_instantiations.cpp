// ==========================================================================
//  ckpe_game_instantiations.cpp
//
//  BSLock.h (per-game EditorAPI) declares:
//      extern template class BSAutoLock<BSReadWriteLock, BSAutoLockReadLockPolicy>;
//      ... (and sibling policies/specializations)
//  i.e. explicit-instantiation DECLARATIONS whose matching DEFINITION exists
//  nowhere in the repo. MSVC emits the (inline) members anyway; clang honours
//  the `extern template` strictly and leaves the ctor/dtor UNDEFINED at link.
//
//  Rather than edit the shared header, we supply the missing explicit-
//  instantiation DEFINITIONS here. A definition following the header's
//  declaration forces instantiation of all members -> the symbols resolve.
//
//  Linux/CMake cross-build ONLY (never in the .sln/.vcxproj). Also compiled out
//  unless the compiler is clang, so it can never affect a real MSVC build.
//  Compiled once per game target; CKPE_GAME_NS selects the game namespace
//  (Fallout4 / SkyrimSE) and the game include path selects the right BSLock.h.
// ==========================================================================

#if defined(__clang__)
// TU-opaque identity function. CKPE's EditorAPI classes mirror the game's memory
// layout; their virtual functions live in CreationKit.exe. clang would devirtualize
// a virtual call on such an object (it knows the mirror's "exact" static type) and
// bind it to a missing CKPE-side symbol. Routing the object pointer through this
// out-of-line, non-inlinable function severs clang's type knowledge, so the call
// stays indirect and dispatches through the runtime (game) vtable. Linux/clang only.
extern "C" const void* ckpe_opaque_ptr(const void* p) { return p; }
#endif

#if defined(__clang__) && defined(CKPE_GAME_NS)

#include <cstdint>
#include <EditorAPI/BSLock.h>
#include <EditorAPI/BGSLocalizedString.h>

namespace CKPE
{
	namespace CKPE_GAME_NS
	{
		namespace EditorAPI
		{
			// The lock policies are also `extern template` in the header, so their
			// static Lock/Unlock (called from BSAutoLock's ctor/dtor) must be
			// instantiated too.
			template struct BSAutoLockDefaultPolicy<BSSpinLock>;
			template struct BSAutoLockReadLockPolicy<BSReadWriteLock>;
			template struct BSAutoLockWriteLockPolicy<BSReadWriteLock>;

			template class BSAutoLock<BSReadWriteLock, BSAutoLockReadLockPolicy>;
			template class BSAutoLock<BSReadWriteLock, BSAutoLockWriteLockPolicy>;
			template class BSAutoLock<BSSpinLock, BSAutoLockDefaultPolicy>;

			// IBGSLocalizedString::length()/c_str() are implemented in CreationKit.exe.
			// clang devirtualizes calls to them (no override visible in the TU) and
			// leaves the symbols undefined at many call sites. Provide the definitions
			// here: each is reached ONLY via clang's devirtualization, and re-dispatches
			// through the laundered 'this' so the call goes VIRTUALLY to the game's real
			// implementation (ckpe_opaque_ptr keeps clang from devirtualizing again, so
			// there is no self-recursion). Defining these two (non-key) virtuals does not
			// force vtable emission (the key function is release(), left undefined).
			std::uint32_t IBGSLocalizedString::length() const noexcept(true)
			{
				return static_cast<const IBGSLocalizedString*>(ckpe_opaque_ptr(this))->length();
			}

			const char* IBGSLocalizedString::c_str() const noexcept(true)
			{
				return static_cast<const IBGSLocalizedString*>(ckpe_opaque_ptr(this))->c_str();
			}
		}
	}
}

#endif // __clang__ && CKPE_GAME_NS
