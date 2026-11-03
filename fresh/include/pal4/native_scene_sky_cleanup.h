#pragma once
#include "pal4/scene_sky_dual_clumps.h"
namespace pal4 {
// IDA5FBE60: caller owns both clumps and four SDK matrices; engine must be live.
class NativeSceneSkyCleanup final : private ISceneSkyDualCleanupRuntime {
public:
    static void Cleanup(SceneSkyDualState& state);
    static void CleanupArena(std::uint32_t arena);
private:
    void SetCleanupPipeline(std::uint32_t clump) override;
    void DestroyClump(std::uint32_t clump) override;
    std::uint32_t DestroyMatrix(std::uint32_t matrix) override;
};
}  // namespace pal4
