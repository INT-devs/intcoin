/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * INTcoin Core Library Implementation
 */

#include "intcoin/intcoin.h"

namespace intcoin {

// ============================================================================
// Version Information
// ============================================================================

const char* GetVersion() {
    return INTCOIN_VERSION;
}

const char* GetCopyright() {
    return INTCOIN_COPYRIGHT;
}

const char* GetLicense() {
    return INTCOIN_LICENSE;
}

BuildInfo GetBuildInfo() {
    BuildInfo info;
    info.version = INTCOIN_VERSION;

#ifdef GIT_COMMIT_HASH
    info.git_commit = GIT_COMMIT_HASH;
#else
    info.git_commit = "unknown";
#endif

#ifdef BUILD_DATE
    info.build_date = BUILD_DATE;
#else
    info.build_date = __DATE__ " " __TIME__;
#endif

#ifdef __clang__
    info.compiler = "Clang " __clang_version__;
#elif defined(__GNUC__)
    info.compiler = "GCC " __VERSION__;
#elif defined(_MSC_VER)
    info.compiler = "MSVC";
#else
    info.compiler = "Unknown";
#endif

#ifdef __APPLE__
    info.platform = "macOS";
#elif defined(__linux__)
    info.platform = "Linux";
#elif defined(__FreeBSD__)
    info.platform = "FreeBSD";
#elif defined(_WIN32)
    info.platform = "Windows";
#else
    info.platform = "Unknown";
#endif

    return info;
}

} // namespace intcoin
