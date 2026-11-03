#pragma once

#include <cstdint>

namespace pal4 {

class ICombatRewardProcessingRuntime {
public:
    virtual ~ICombatRewardProcessingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CopyTree(std::uint32_t destination,
                          std::uint32_t source) = 0;
    virtual void CopyReward(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void CopyResult(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyReport(std::uint32_t report) = 0;
    virtual void DestroyPartialReport(std::uint32_t report,
                                      std::uint32_t completed_trees,
                                      std::uint32_t completed_records) = 0;

    [[nodiscard]] virtual std::uint32_t GetGameCounter(
        std::uint32_t index) = 0;
    virtual void SetGameCounter(std::uint32_t index,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ItemCount() = 0;
    [[nodiscard]] virtual std::uint32_t ItemAt(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ItemId(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    [[nodiscard]] virtual std::uint32_t FindOrInsertReward(
        std::uint32_t tree, std::uint32_t item_id) = 0;
    virtual void UpdateInventoryCounter(std::uint32_t value) = 0;

    [[nodiscard]] virtual std::uint32_t TreeFirst(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t TreeEnd(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t TreeNext(std::uint32_t node) = 0;
    [[nodiscard]] virtual bool IsInventoryItem(std::uint32_t item_id) = 0;
    [[nodiscard]] virtual bool IsCollectionItem(std::uint32_t item_id) = 0;
    virtual void AddInventoryItem(std::uint32_t item_id,
                                  std::int32_t count) = 0;
    virtual void AddCollectionItem(std::uint32_t item_id) = 0;

    [[nodiscard]] virtual std::uint32_t FirstPlayerIterator() = 0;
    [[nodiscard]] virtual bool IteratorDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceIterator(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool CombatInactive() = 0;
    [[nodiscard]] virtual bool IsFocusActor(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ExperienceFactor(std::uint32_t index) = 0;
    [[nodiscard]] virtual bool ActorStateThree(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorMetric(
        std::uint32_t actor, std::uint32_t key) = 0;
    virtual void ApplyExperience(std::uint32_t actor,
                                 std::int32_t amount) = 0;
    [[nodiscard]] virtual std::int32_t IndexedExperience(
        std::uint32_t actor) = 0;
    virtual void SetIndexedExperience(std::uint32_t actor,
                                      std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t NextLevel(
        std::uint32_t actor, std::int32_t experience) = 0;
    virtual void SetActorLevel(std::uint32_t actor,
                               std::int32_t level) = 0;
    virtual void UpdateActorState(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t BuildMatchingRecords(
        std::uint32_t actor) = 0;
    virtual void CopyMatchingRecords(std::uint32_t reward,
                                     std::uint32_t records) = 0;
    virtual void ReleaseMatchingRecords(std::uint32_t records) = 0;
};

class CombatRewardProcessor final {
public:
    // IDA 0x5715f0. source is the by-value 212-byte report argument.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t scratch, ICombatRewardProcessingRuntime& runtime);
    // IDA 0x573200. The native one-word type argument is passed by value.
    [[nodiscard]] static bool MatchesItemType(
        std::uint32_t type, std::uint32_t item,
        ICombatRewardProcessingRuntime& runtime);
};

}  // namespace pal4
