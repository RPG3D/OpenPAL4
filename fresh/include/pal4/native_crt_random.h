#pragma once

#include "pal4/comparison_crt_trace.h"
#include <cstdlib>
#ifdef _WIN32
// Avoid windows.h macros leaking into game interface method declarations.
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
#endif

namespace pal4 {
// Diagnostic labels only. Sources must have static lifetime, like the existing
// __FILE__ labels. Nesting follows native service calls and stays thread-local;
// neither entering nor leaving a scope touches the public CRT.
inline thread_local const char* native_crt_observation_site{};
inline thread_local std::uintptr_t native_crt_observation_owner{};
// Process-local provenance only. Nesting, exceptions and helper threads must
// not leak task identity into later draws; entering this scope never samples.
class NativeCrtObservationOwnerScope final {
public:
    explicit NativeCrtObservationOwnerScope(std::uintptr_t owner) noexcept
        :previous_(native_crt_observation_owner) {native_crt_observation_owner=owner;}
    ~NativeCrtObservationOwnerScope(){native_crt_observation_owner=previous_;}
    NativeCrtObservationOwnerScope(const NativeCrtObservationOwnerScope&)=delete;
    NativeCrtObservationOwnerScope& operator=(const NativeCrtObservationOwnerScope&)=delete;
private:
    std::uintptr_t previous_{};
};
class NativeCrtObservationScope final {
public:
    explicit NativeCrtObservationScope(const char* source) noexcept
        : previous_(native_crt_observation_site) {
        native_crt_observation_site = source;
    }
    ~NativeCrtObservationScope() { native_crt_observation_site = previous_; }
    NativeCrtObservationScope(const NativeCrtObservationScope&) = delete;
    NativeCrtObservationScope& operator=(const NativeCrtObservationScope&) = delete;
private:
    const char* previous_{};
};

inline std::uint32_t NativeCrtRandomThread() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return 0u;
#endif
}
// Public host CRT remains the sole implementation/state owner. Native records
// expose no private CRT word: state_before/state_after are unavailable (zero).
inline int NativeCrtRandom(const char* source) {
    const auto value=std::rand();
    auto& trace=GetComparisonCrtTrace();
    if(native_crt_observation_site)source=native_crt_observation_site;
    if(trace.Enabled())trace.Add({0u,static_cast<std::uint32_t>(value),NativeCrtRandomThread(),0u,0u,0u,source,native_crt_observation_owner});
    return value;
}
inline void NativeCrtSeedRandom(unsigned seed,const char* source) {
    std::srand(seed);
    auto& trace=GetComparisonCrtTrace();
    if(trace.Enabled())trace.Add({1u,seed,NativeCrtRandomThread(),0u,0u,0u,source});
}
} // namespace pal4
