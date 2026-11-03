#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerStringMethodRuntime {
public:
    virtual ~IActorManagerStringMethodRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DeleteObject(std::uint32_t object,
                              std::uint32_t flags) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerStringMethodProperty final {
public:
    static constexpr std::uint32_t kDerivedVtable = 0x844f64u;
    static constexpr std::uint32_t kBaseVtable = 0x842bd4u;

    static void Destroy(std::uint32_t object,
                        IActorManagerStringMethodRuntime& runtime);
    static void DestroyWithVtable(
        std::uint32_t object, std::uint32_t derived_vtable,
        IActorManagerStringMethodRuntime& runtime);
    static void CleanupOwnedPair(std::uint32_t pair,
                                 IActorManagerStringMethodRuntime& runtime);
    static void ReleaseOwnedSlot(std::uint32_t slot,
                                 IActorManagerStringMethodRuntime& runtime);
};

}  // namespace pal4
