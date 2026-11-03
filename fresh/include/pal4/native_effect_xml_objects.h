#pragma once
#include "pal4/native_effect_handle_context_runtime.h"
#include "pal4/effect_system.h"
#include <memory>
#include <cstddef>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class NativeEffectTreeStorage;
class NativePkFileManagerRuntime;

// One dispatcher per initialized group. Group cleanup must retire its context
// and timed trees before this owner, textures or emitter clumps are destroyed.
class NativeEffectXmlObjects final : public INativeContextEffectObjects {
public:
    NativeEffectXmlObjects(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeEffectTextureDictionary&,NativeEffectTreeStorage&,NativePkFileManagerRuntime&,
        std::uint32_t group);
    ~NativeEffectXmlObjects();
    NativeEffectXmlObjects(const NativeEffectXmlObjects&)=delete;
    NativeEffectXmlObjects& operator=(const NativeEffectXmlObjects&)=delete;
    EffectSystemHandle Load(std::uint32_t document,std::uint32_t clump);
    EffectSystemHandle LoadFile(std::string_view name,std::uint32_t clump);
    std::size_t Count() const noexcept;
    void Stop(std::uint32_t) override;
    void Destroy(std::uint32_t,std::uint32_t flag) override;
    float DrainTime(std::uint32_t) override;
    void Update(std::uint32_t,double seconds) override;
    void Draw(std::uint32_t,std::uint32_t context) override;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
