#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeParticleEffectFactory;
class IEffectSystemRenderQueueRuntime;

// Borrows initialized group lists, particle objects, proximity gate storage and
// geometry dispatch. Draw requires the caller's active SDK camera when visible.
class NativeParticleGroupFrames final {
public:
    NativeParticleGroupFrames(NativeBaseObjectMemory&,NativeParticleEffectFactory&,
        std::uint32_t proximity_gate,IEffectSystemRenderQueueRuntime&);
    ~NativeParticleGroupFrames();
    NativeParticleGroupFrames(const NativeParticleGroupFrames&)=delete;
    NativeParticleGroupFrames& operator=(const NativeParticleGroupFrames&)=delete;
    void Update(std::uint32_t group,double delta);
    void Draw(std::uint32_t group,std::uint32_t context);
    void AdvanceObject(std::uint32_t object,std::uint32_t low,std::uint32_t high);
    void DrawObject(std::uint32_t object,std::uint32_t context);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
