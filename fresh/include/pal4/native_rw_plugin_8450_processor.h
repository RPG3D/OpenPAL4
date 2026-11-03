#pragma once

#include <cstdint>
#include <exception>

#include "pal4/script_builtin_forwarders.h"

struct RpClump;
struct RpAtomic;

namespace pal4 {

class NativeRwPlugin8450Processor final {
public:
    [[nodiscard]] static std::uint32_t ProcessClump(RpClump* clump);
    [[nodiscard]] static std::uint32_t QueryAtomic(RpAtomic* atomic);
    [[nodiscard]] static std::uint32_t QueryClump(RpClump* clump);
    static void RestoreClump(RpClump* clump);
    [[nodiscard]] static std::uint32_t RenderCallbackAddress();
    [[nodiscard]] static std::exception_ptr TakeRenderError();
    static void SetDistanceThreshold(float value);
};

class NativeScriptCameraDistanceRuntime final
    : public IScriptCameraDistanceRuntime {
public:
    [[nodiscard]] std::uint32_t SetDistanceOptimization(
        std::uint32_t enabled) override;
};

}  // namespace pal4
