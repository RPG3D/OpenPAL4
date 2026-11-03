#pragma once

#include "pal4/actor_manager_owned_slot.h"

#include <cstdint>

namespace pal4 {

class IActorManagerCombatCopyRuntime {
public:
    virtual ~IActorManagerCombatCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void EraseText(std::uint32_t record, std::uint32_t start,
                           std::uint32_t count) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t value) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
};

class IActorManagerCombatActorRuntime : public IActorManagerCombatCopyRuntime {
public:
    virtual ~IActorManagerCombatActorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructDamage(
        std::uint32_t storage) = 0;
    virtual void ConstructActorData(std::uint32_t object,
                                    std::uint32_t mode,
                                    std::uint32_t damage) = 0;
    virtual void InitializeArrays(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructFacet(
        std::uint32_t storage, std::uint32_t object) = 0;
    virtual void CleanupProperty(std::uint32_t object,
                                 std::uint32_t context) = 0;
    virtual void DestroyFacet(std::uint32_t facet) = 0;
    virtual void DestroyTextArray(std::uint32_t address,
                                  std::uint32_t count,
                                  std::uint32_t stride) = 0;
    virtual void DestroyActorData(std::uint32_t object,
                                  std::uint32_t context) = 0;
};

class IActorManagerCombatStatsRuntime
    : public IActorManagerOwnedSlotRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructDamage(
        std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t TemporarySlotAddress() = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void ProcessSkills(std::uint32_t actor,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t DataElement(
        std::uint32_t data, std::uint32_t index) = 0;
    virtual void SetValidatedProperty(std::uint32_t data,
                                      std::uint32_t index,
                                      std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t UpdateHealthStatus(
        std::uint32_t actor) = 0;
};

struct ActorManagerObjectPointerRange {
    std::uint32_t marker{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity_end{};
};
static_assert(sizeof(ActorManagerObjectPointerRange) == 16u);

class IActorManagerObjectListCleanupRuntime {
public:
    virtual ~IActorManagerObjectListCleanupRuntime() = default;
    virtual void CleanupObjectLists(std::uint32_t actor) = 0;
    virtual void CollectObjects(std::uint32_t collection,
                                ActorManagerObjectPointerRange& output) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void HandleObjectFade(std::uint32_t actor,
                                  std::uint32_t object,
                                  std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
};

class ActorManagerCombatActor final {
public:
    static constexpr std::uint32_t kVtable = 0x844d08u;
    static constexpr std::uint32_t kFacetVtable = 0x844cf0u;
    static constexpr std::uint32_t kPropertyVtable = 0x844ce8u;

    // IDA 0x5308c0: source is a complete monster database record, not text.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t monster_record,
        IActorManagerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value1148(
        std::uint32_t object, IActorManagerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value1152(
        std::uint32_t object, IActorManagerCombatActorRuntime& runtime);
    [[nodiscard]] static bool HasPositive1144(
        std::uint32_t object, IActorManagerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Value960(
        std::uint32_t object, IActorManagerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags, std::uint32_t context,
        IActorManagerCombatActorRuntime& runtime);
    static void Destroy(std::uint32_t object, std::uint32_t context,
                        IActorManagerCombatActorRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyFrom(
        std::uint32_t object, std::uint32_t source,
        IActorManagerCombatCopyRuntime& runtime);
    [[nodiscard]] static std::int32_t RebuildStats(
        std::uint32_t object, IActorManagerCombatStatsRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupObjectLists(
        std::uint32_t object, IActorManagerObjectListCleanupRuntime& runtime);
};

}  // namespace pal4
