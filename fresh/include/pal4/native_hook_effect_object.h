#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
struct ParticleVisualState;

// Owns the 16-byte HookEffect and its particle tank. Context (+48), textures
// and engine outlive all updates/destruction. The borrowed clump/emitter must
// survive until DrainTime detaches the frame and suppresses vertex sampling.
class NativeHookEffectObject final {
public:
    NativeHookEffectObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeEffectTextureDictionary&);
    ~NativeHookEffectObject();
    NativeHookEffectObject(const NativeHookEffectObject&)=delete;
    NativeHookEffectObject& operator=(const NativeHookEffectObject&)=delete;
    bool Load(std::uint32_t config,std::uint32_t clump,std::uint32_t context);
    std::uint32_t Address() const noexcept;
    ParticleVisualState* Particle() noexcept;
    void Stop();
    float DrainTime();
    void Update(double seconds);
    void Draw(std::uint32_t context);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
