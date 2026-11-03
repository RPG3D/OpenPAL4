#pragma once
#include <cstdint>
namespace pal4 {
class IGameArenaRenderClumpRuntime {
public:
    virtual ~IGameArenaRenderClumpRuntime() = default;
    virtual std::uint32_t ReadClump(std::uint32_t address) = 0;
    virtual void EnableDistanceOptimization() = 0;
    virtual void AddClump(std::uint32_t arena, std::uint32_t clump) = 0;
    virtual void BindAlphaQueue(std::uint32_t clump, std::uint32_t queue) = 0;
    virtual void BindDistanceCallbacks(std::uint32_t clump) = 0;
};
// IDA518E09..518E81 inside518A40. Queue binding applies to all three
// optional clumps; clipNA deliberately omits the8450 distance wrapper.
void BindGameArenaRenderClumps(std::uint32_t arena, IGameArenaRenderClumpRuntime&);
}
