#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPropertyFacetRuntime {
public:
    virtual ~IActorManagerPropertyFacetRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerPropertyFacet final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x844f80u;
    static constexpr std::uint32_t kFirstVtable = 0x844f88u;
    static constexpr std::uint32_t kSecondVtable = 0x844f90u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IActorManagerPropertyFacetRuntime& runtime);
    static void MarkFirst(std::uint32_t object,
                          IActorManagerPropertyFacetRuntime& runtime);
    static void MarkSecond(std::uint32_t object,
                           IActorManagerPropertyFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteFirst(
        std::uint32_t object, std::uint8_t flags,
        IActorManagerPropertyFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteSecond(
        std::uint32_t object, std::uint8_t flags,
        IActorManagerPropertyFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteFirstInline(
        std::uint32_t object, std::uint8_t flags,
        IActorManagerPropertyFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteSecondInline(
        std::uint32_t object, std::uint8_t flags,
        IActorManagerPropertyFacetRuntime& runtime);
};

}  // namespace pal4
