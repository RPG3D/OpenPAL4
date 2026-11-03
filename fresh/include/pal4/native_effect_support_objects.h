#pragma once
#include <cstdint>
#include <memory>
#include <exception>

namespace pal4 {
class NativeBaseObjectMemory;
// Rebuilt equivalents of original random93CE14 and auxiliary93CE18 globals.
struct NativeEffectSupportSingletons final {
    std::uint32_t random{},auxiliary{};
};
// Owns the 8-byte random and 44-byte auxiliary objects stored at global effect
// singleton +140/+144. Slots, allocator and singleton state must outlive it.
class NativeEffectSupportObjects final {
public:
    NativeEffectSupportObjects(NativeBaseObjectMemory&,std::uint32_t& random_slot,
        std::uint32_t& auxiliary_slot,NativeEffectSupportSingletons&,std::uint32_t seed=0u);
    ~NativeEffectSupportObjects();
    NativeEffectSupportObjects(const NativeEffectSupportObjects&)=delete;
    NativeEffectSupportObjects& operator=(const NativeEffectSupportObjects&)=delete;
    std::uint32_t NextBounded(std::uint32_t bound);
    double NextUnit();
    double NextRange(float minimum,float maximum);
    void SetRadius(float radius);
    // Preserve the original enabled flag; unlike SetRadius this does not clear effects.
    std::int32_t SetSquaredRadius(float radius);
    float RadiusSquared() const;
    void CleanupAuxiliary();
    bool RegisterClump(std::uint32_t clump);
    void UnregisterClump(std::uint32_t clump);
    std::exception_ptr TakeRenderError();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace pal4
