#pragma once

#include "pal4/combat_event_record.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectObjectPoolCleanupRuntime {
public:
    virtual ~IEffectObjectPoolCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedObjectNil() const = 0;
    virtual void DestroyEffectObject(std::uint32_t pool,
                                     std::uint32_t scratch,
                                     std::uint32_t effect) = 0;
    virtual void DestroyObjectBranch(std::uint32_t root) = 0;
    virtual void EraseObjectNode(std::uint32_t tree,
                                 std::uint32_t node) = 0;
};

class IEffectObjectPoolRuntime : public IEffectObjectPoolCleanupRuntime {
public:
    ~IEffectObjectPoolRuntime() override = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    virtual void DestroySubtree(std::uint32_t node) = 0;
    virtual void ReleaseText(std::uint32_t text) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Next(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t tree, std::uint32_t node) = 0;
};

enum class EffectObjectPoolNilKind : std::uint8_t {
    NameCount,
    Object,
};

class IEffectObjectPoolDestructorRuntime : public IEffectObjectPoolRuntime {
public:
    virtual void RemoveNameCountRange(std::uint32_t tree,
                                      std::uint32_t first,
                                      std::uint32_t last) = 0;
    virtual void EraseEffectObjectRange(std::uint32_t tree,
                                        std::uint32_t first,
                                        std::uint32_t last) = 0;
    [[nodiscard]] virtual std::uint32_t FirstElement(
        std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilReferences(
        EffectObjectPoolNilKind kind) = 0;
    virtual void SetSharedNilReferences(EffectObjectPoolNilKind kind,
                                        std::uint32_t references) = 0;
    virtual void ClearSharedNil(EffectObjectPoolNilKind kind) = 0;
};

struct EffectPoolTemporaryRecords {
    std::uint32_t copied_event{};
    std::uint32_t insertion_key{};
    std::uint32_t effect_slot{};
};

class IEffectObjectPoolRecordRuntime : public ICombatEventRecordRuntime {
public:
    ~IEffectObjectPoolRecordRuntime() override = default;
    [[nodiscard]] virtual EffectPoolTemporaryRecords TemporaryRecords() = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void InsertCached(std::uint32_t tree,
                              std::uint32_t record) = 0;
};

class IEffectObjectPoolAcquireRuntime : public IEffectObjectPoolRecordRuntime {
public:
    ~IEffectObjectPoolAcquireRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t FindCached(
        std::uint32_t tree, std::uint32_t key) = 0;
    virtual void RemoveCached(std::uint32_t tree, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeEffectSystem() = 0;
    [[nodiscard]] virtual std::uint32_t LoadEffectFromFile(
        std::uint32_t manager, std::uint32_t text) = 0;
    virtual void SetRotation(std::uint32_t effect,
                             const std::array<float, 3u>& value) = 0;
    virtual void SetScale(std::uint32_t effect,
                          const std::array<float, 3u>& value) = 0;
    virtual void SetPosition(std::uint32_t effect,
                             const std::array<float, 3u>& value) = 0;
    virtual void ClearFacets(std::uint32_t effect) = 0;
    virtual void CleanupXmlNode(std::uint32_t node) = 0;
    virtual void RemoveFromParentFrame(std::uint32_t effect) = 0;
    virtual void ResetVisibility(std::uint32_t effect) = 0;
};

class IEffectObjectPoolNameCountRuntime : public IEffectObjectPoolRecordRuntime {
public:
    ~IEffectObjectPoolNameCountRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t SharedNameNil() const = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryCountRecord() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateNameCountNode(
        std::uint32_t bytes) = 0;
};

struct EffectNameCountLookup {
    std::uint32_t node{};
    bool inserted{};
};

class IEffectObjectPoolReturnRuntime : public IEffectObjectPoolNameCountRuntime {
public:
    ~IEffectObjectPoolReturnRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t TemporaryNameRecord() = 0;
    virtual void ReadName(std::uint32_t xml_node,
                          std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t FindActive(
        std::uint32_t tree, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t NextActive(std::uint32_t node) = 0;
    virtual void RemoveActive(std::uint32_t tree,
                              std::uint32_t node) = 0;
    virtual void RemoveFromParentFrame(std::uint32_t effect) = 0;
    virtual void DestroyEffect(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t SharedObjectNil() const = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryTrimQuery() = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryTrimName() = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryTrimZero() = 0;
    virtual void RemoveCached(std::uint32_t tree,
                              std::uint32_t node) = 0;
    virtual void ReleaseTrimName(std::uint32_t name) = 0;
};

class EffectObjectPool final {
public:
    // IDA 0x639ad0: destroy the three owned tree heads and shared nils.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t pool, std::uint32_t scratch,
        IEffectObjectPoolDestructorRuntime& runtime);
    // IDA 0x62cc40: clear prior state and enable with a new parameter.
    [[nodiscard]] static std::uint8_t ResetAndEnable(
        std::uint32_t pool, std::uint32_t scratch,
        std::uint32_t parameter, IEffectObjectPoolRuntime& runtime);
    // IDA 0x62cc60: release cached names and owned objects when enabled.
    static void Deinitialize(std::uint32_t pool, std::uint32_t scratch,
                             IEffectObjectPoolRuntime& runtime);
    // IDA 0x62d1d0: destroy active effects and clear both object trees.
    static void CleanupAllObjects(
        std::uint32_t pool, std::uint32_t scratch,
        IEffectObjectPoolCleanupRuntime& runtime);
    // IDA 0x62cd30: reuse a cached effect or load and prepare a new one.
    [[nodiscard]] static std::uint32_t AcquireEffect(
        std::uint32_t pool, std::uint32_t key,
        IEffectObjectPoolAcquireRuntime& runtime);
    // IDA 0x62cf80: return an effect to cache or retire it.
    static void ReturnEffect(std::uint32_t pool, std::uint32_t effect,
                             std::uint8_t mode,
                             IEffectObjectPoolReturnRuntime& runtime);
    // IDA 0x62d330: increment or insert the effect-name count.
    static void IncrementNameCount(
        std::uint32_t pool, std::uint32_t name,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62d440: evict the least-used cached effect at capacity.
    static void TrimCache(std::uint32_t pool,
                          IEffectObjectPoolReturnRuntime& runtime);
    // IDA 0x62d630: find or insert a record in the name-count tree.
    [[nodiscard]] static EffectNameCountLookup FindOrCreateNameCount(
        std::uint32_t tree, std::uint32_t name,
        IEffectObjectPoolNameCountRuntime& runtime);
};

}  // namespace pal4
