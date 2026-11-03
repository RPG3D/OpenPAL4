#pragma once
#include "pal4/particle_visual_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;

// Binds existing game particle lifecycle/transform algorithms to audited public
// PTank and RenderWare APIs. The caller owns the state and must Destroy it before
// the SDK shuts down. Collection particles reuse the shared billboard visual.
class NativeParticleTankRuntime final {
public:
    explicit NativeParticleTankRuntime(NativeBaseObjectMemory&);
    ~NativeParticleTankRuntime();
    NativeParticleTankRuntime(const NativeParticleTankRuntime&)=delete;
    NativeParticleTankRuntime& operator=(const NativeParticleTankRuntime&)=delete;
    void Construct(ParticleVisualState&);
    [[nodiscard]] bool Initialize(ParticleVisualState&,bool not_point,
        std::uint32_t texture,std::uint32_t emitter_type,
        float size_x,float size_y,float size_z,bool axis_xz);
    [[nodiscard]] bool InitializeWithAtomic(ParticleVisualState&,bool not_point,
        std::uint32_t texture,std::uint32_t atomic,bool axis_xz);
    void SetCollectionEnabled(ParticleVisualState&,bool enabled);
    void Reset(ParticleVisualState&);
    void Destroy(ParticleVisualState&);
    [[nodiscard]] std::uint8_t UpdateFrame(ParticleVisualState&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
} // namespace pal4
