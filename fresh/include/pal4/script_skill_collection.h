#pragma once

#include "pal4/game_data_structure_state.h"
#include "pal4/script_skill_context.h"
#include "pal4/script_skill_vector8.h"
#include "pal4/skill_data_lookup.h"

#include <cstdint>
#include <span>

namespace pal4 {

class IScriptSkillCollectionInitRuntime : public IGameDataStructureStateRuntime {
public:
    ~IScriptSkillCollectionInitRuntime() override = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class IScriptSkillCollectionRuntime : public IScriptSkillCollectionInitRuntime,
                                      public ISkillDataLookupRuntime,
                                      public IScriptSkillContextRuntime,
                                      public IScriptSkillVector8Runtime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) override = 0;
    virtual void Free(std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    // Native typed catalogs can bind this lookup without fabricating GameDB.
    [[nodiscard]] virtual std::uint32_t FindSkillRecord(std::int32_t id) {
        return SkillDataLookup::FindSkill(GameDatabase() + 12u, id, *this);
    }
    virtual void UpdateHealthStatus(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveSkillRecord(
        std::uint32_t skill) = 0;
};

class IScriptSkillListCleanupRuntime {
public:
    virtual ~IScriptSkillListCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveSkillRecord(
        std::uint32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t vtable_offset,
        std::span<const std::uint32_t> arguments) = 0;
};

class ScriptSkillCollection final {
public:
    static constexpr std::uint32_t kSize = 0x2c4u;
    static constexpr std::uint32_t kVtable = 0x8430e8u;

    // IDA 0x5890e0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, IScriptSkillCollectionInitRuntime& runtime);

    // IDA 0x589130.
    [[nodiscard]] static std::uint8_t HandleFade(
        std::uint32_t object, std::int32_t skill_id,
        std::uint32_t context, IScriptSkillCollectionRuntime& runtime);

    // IDA 0x589170. Inserts an 8-byte or 12-byte skill context at list end.
    [[nodiscard]] static std::uint8_t ProcessComplexObjectList(
        std::uint32_t object, std::uint32_t skill,
        std::uint32_t context, IScriptSkillCollectionRuntime& runtime);

    // IDA 0x589410.
    [[nodiscard]] static std::uint8_t FilterAndCleanData(
        std::uint32_t object, std::uint32_t key,
        IScriptSkillCollectionRuntime& runtime);

    // IDA 0x589580.
    [[nodiscard]] static std::uint8_t ProcessDynamicList(
        std::uint32_t object, std::uint32_t elapsed,
        IScriptSkillCollectionRuntime& runtime);

    // IDA 0x5896a0 and 0x589710.
    [[nodiscard]] static bool HasSkillKey(
        std::uint32_t object, std::uint32_t key,
        IScriptSkillCollectionRuntime& runtime);
    [[nodiscard]] static bool HasRecordField60(
        std::uint32_t object, std::uint32_t value,
        IScriptSkillCollectionRuntime& runtime);

    // IDA 0x589780 / 0x5897f0.
    [[nodiscard]] static std::uint32_t ProcessSkillsBatch(
        std::uint32_t object, std::uint32_t requested_id,
        std::uint32_t target, IScriptSkillContextRuntime& runtime);
    [[nodiscard]] static bool TriggerSkillForTarget(
        std::uint32_t object, std::int32_t skill_id,
        std::uint32_t target, IScriptSkillCollectionRuntime& runtime);

    // IDA 0x589880 / 0x589970.
    [[nodiscard]] static std::uint32_t CleanupObjectLists(
        std::uint32_t object, IScriptSkillCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupObjectLists(
        std::uint32_t object, IScriptSkillListCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetObjectProperties(
        std::uint32_t object, IScriptSkillCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetObjectProperties(
        std::uint32_t object, IScriptSkillListCleanupRuntime& runtime);

    // IDA 0x5899e0 / 0x589aa0.
    [[nodiscard]] static std::uint8_t ProcessObjectFadeAndListUpdate(
        std::uint32_t object, std::uint32_t skill,
        IScriptSkillCollectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t ConditionalListCleanup(
        std::uint32_t object, std::uint32_t field60,
        bool unset_record, IScriptSkillCollectionRuntime& runtime);
};

}  // namespace pal4
