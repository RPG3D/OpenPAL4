#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;

// Owns a 280-byte StripSystemEffect with its two embedded resource blocks.
// Borrowed clump hooks must survive until DrainTime and the first update have
// captured endpoints. Documents, textures, allocator and SDK outlive this owner.
class NativeStripEffectObject final {
public:
    NativeStripEffectObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
                            NativeEffectTextureDictionary&);
    ~NativeStripEffectObject();
    NativeStripEffectObject(const NativeStripEffectObject&)=delete;
    NativeStripEffectObject& operator=(const NativeStripEffectObject&)=delete;
    bool Load(std::uint32_t config,std::uint32_t clump,std::uint32_t context);
    std::uint32_t Address() const noexcept;
    void Stop();
    float DrainTime();
    void Update(double seconds);
    void Draw(std::uint32_t context);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
