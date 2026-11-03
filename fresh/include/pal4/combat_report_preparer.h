#pragma once

#include <cstdint>

namespace pal4 {

class ICombatReportRuntime {
public:
    virtual ~ICombatReportRuntime() = default;
    virtual void ConstructReport(std::uint32_t report,
                                 std::uint8_t tree_seed,
                                 std::uint8_t record_seed) = 0;
    virtual void DestroyReport(std::uint32_t report) = 0;
    [[nodiscard]] virtual std::uint32_t CombatStats() = 0;
    [[nodiscard]] virtual std::int32_t TotalStat(std::uint32_t stats) = 0;
    [[nodiscard]] virtual std::int32_t BattleRank(
        std::uint32_t stats, std::int32_t total) = 0;
    [[nodiscard]] virtual std::uint32_t FirstMonsterIterator() = 0;
    [[nodiscard]] virtual bool IteratorDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceIterator(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    [[nodiscard]] virtual std::int32_t ActorMetric(
        std::uint32_t actor, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindOrInsertReward(
        std::uint32_t tree, std::uint32_t item, std::uint32_t zero) = 0;
    virtual void AssignTree(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void CopyReward(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class CombatReportPreparer final {
public:
    // IDA 0x571240. scratch is the original 212-byte stack report.
    [[nodiscard]] static std::uint32_t Prepare(
        std::uint32_t destination, std::uint32_t scratch,
        std::uint8_t tree_seed, std::uint8_t record_seed,
        ICombatReportRuntime& runtime);
};

}  // namespace pal4
