#pragma once
#include "pal4/particle_visual_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
// Reuses game emission/simulation/upload and draw-state algorithms. SDK must
// remain active. Draw requires the caller's active RenderWare camera update.
class NativeParticleFrameRuntime final {
public:
    explicit NativeParticleFrameRuntime(NativeBaseObjectMemory&);
    ~NativeParticleFrameRuntime();
    NativeParticleFrameRuntime(const NativeParticleFrameRuntime&)=delete;
    NativeParticleFrameRuntime& operator=(const NativeParticleFrameRuntime&)=delete;
    int Update(ParticleVisualState&,double delta,std::uint32_t argument=0u,bool skip_emission=false);
    int Draw(ParticleVisualState&);
    std::int32_t Deactivate(ParticleVisualState&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
