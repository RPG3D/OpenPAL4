#pragma once

#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;

// Owns only the singleton's +148 owner registry, not the singleton or owners.
// The borrowed singleton image and native allocator must outlive this object.
// Construct once on uninitialized registry storage; destruction releases all
// registry nodes even when the singleton's enabled byte has been cleared.
class NativeEffectOwnerRegistry final {
public:
    NativeEffectOwnerRegistry(NativeBaseObjectMemory& memory,
        std::uint32_t singleton);
    ~NativeEffectOwnerRegistry();
    NativeEffectOwnerRegistry(const NativeEffectOwnerRegistry&) = delete;
    NativeEffectOwnerRegistry& operator=(const NativeEffectOwnerRegistry&) = delete;
    [[nodiscard]] bool Register(std::uint32_t owner);
    void Unregister(std::uint32_t owner);
    [[nodiscard]] bool Contains(std::uint32_t owner);
    [[nodiscard]] std::uint32_t Nil() const;
private:
    class Memory;
    std::unique_ptr<Memory> memory_;
    std::uint32_t singleton_;
};
}  // namespace pal4
