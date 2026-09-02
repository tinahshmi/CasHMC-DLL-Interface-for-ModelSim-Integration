#pragma once
// ============================================================
// POSIX -> MinGW-w64 compatibility shims
// ============================================================
// Force-included (via -include) into every translation unit of
// the Windows DLL build, BEFORE any vendored CasHMC header is
// parsed. This lets packages/CasHMC/sources/ stay a pristine,
// unmodified GitHub checkout while still compiling under MinGW.
//
// Add further shims here as new POSIX/MSVC-only calls surface.

#if defined(_WIN32) || defined(__MINGW32__)

#include <direct.h>

// POSIX mkdir(path, mode) has no Windows equivalent (Windows has
// no permission-bit argument). Provide a 2-arg shim and redirect
// all "mkdir" tokens -- including the header's own declaration --
// to it via an object-like macro, so both mkdir("x") and
// mkdir("x", mode) call sites compile unchanged.
inline int cashmc_compat_mkdir(const char* path, int mode = 0) {
    (void)mode;
    return _mkdir(path);
}
#define mkdir cashmc_compat_mkdir

#endif // _WIN32 || __MINGW32__