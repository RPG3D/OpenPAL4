#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerCombatActorRuntime {
public:
    virtual ~IActorManagerPlayerCombatActorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructDamage(
        std::uint32_t storage) = 0;
    virtual void ConstructActorData(std::uint32_t actor, std::uint32_t mode,
                                    std::uint32_t damage) = 0;
    virtual void ConstructTextArray(std::uint32_t address,
                                    std::uint32_t count) = 0;
    virtual void ConstructEmbeddedArrays(std::uint32_t address) = 0;
    virtual void CopyActorData(std::uint32_t actor,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructFacet(
        std::uint32_t storage, std::uint32_t actor) = 0;
    virtual void CleanupProperty(std::uint32_t actor,
                                 std::uint32_t context) = 0;
    virtual void DestroyFacet(std::uint32_t facet) = 0;
    virtual void DeleteChild(std::uint32_t child,
                             std::uint32_t flags) = 0;
    virtual void DestroyTextArray(std::uint32_t address,
                                  std::uint32_t count) = 0;
    virtual void DestroyActorData(std::uint32_t actor,
                                  std::uint32_t context) = 0;
};

class ActorManagerPlayerCombatActor final {
public:
    static constexpr std::uint32_t kObjectSize = 0x528u;
    static constexpr std::uint32_t kVtable = 0x845010u;
    static constexpr std::uint32_t kFacetVtable = 0x844ff8u;
    static constexpr std::uint32_t kPropertyVtable = 0x844ff0u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t actor, std::uint32_t source,
        IActorManagerPlayerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value1200(
        std::uint32_t actor, IActorManagerPlayerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value1204(
        std::uint32_t actor, IActorManagerPlayerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value1196(
        std::uint32_t actor, IActorManagerPlayerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeFacet(
        std::uint32_t actor, IActorManagerPlayerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t actor, std::uint8_t flags, std::uint32_t context,
        IActorManagerPlayerCombatActorRuntime& runtime);
    static void Destroy(
        std::uint32_t actor, std::uint32_t context,
        IActorManagerPlayerCombatActorRuntime& runtime);
};

}  // namespace pal4
