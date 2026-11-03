#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectTreeStorage;
class NativeEffectActiveFrame;
class NativeParticleGroupFrames;
class INativeContextEffectObjects;

// Original6300D0/630160 orchestration over an initialized native group.
// Borrows all services and storage. Nonempty context/timed trees require a
// bound XML dispatcher, which must outlive frame use and group cleanup.
class NativeEffectGroupFrame final {
public:
    NativeEffectGroupFrame(NativeBaseObjectMemory&,std::uint32_t group,
        NativeEffectTreeStorage&,NativeEffectActiveFrame&,NativeParticleGroupFrames&);
    ~NativeEffectGroupFrame();
    int Update(double seconds);
    void Draw(std::uint32_t context);
    void BindContextEffects(INativeContextEffectObjects* objects);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
