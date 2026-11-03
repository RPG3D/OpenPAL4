#include "pal4/combat_report_preparer.h"

#include <algorithm>
#include <cmath>

namespace pal4 {

std::uint32_t CombatReportPreparer::Prepare(
    std::uint32_t destination, std::uint32_t scratch,
    std::uint8_t tree_seed, std::uint8_t record_seed,
    ICombatReportRuntime& runtime) {
    runtime.ConstructReport(scratch, tree_seed, record_seed);
    std::uint32_t iterator = 0u;
    try {
        const auto stats = runtime.CombatStats();
        const auto total = runtime.TotalStat(stats);
        const auto rank = runtime.BattleRank(runtime.CombatStats(), total);
        iterator = runtime.FirstMonsterIterator();

        while (!runtime.IteratorDone(iterator)) {
            const auto actor = runtime.IteratorActor(iterator);
            const auto state = runtime.ReadWord(actor + 0x33cu);
            if (state == 3u || state == 4u) {
                const float first = static_cast<float>(
                    runtime.ReadInt(actor + 0x394u));
                const float second = static_cast<float>(
                    runtime.ReadInt(actor + 0x398u));
                const auto low = std::min(first, second);
                const auto high = std::max(first, second);
                const auto roll = runtime.RandomNumber() % 1000u;
                const auto amount = runtime.FloatToInt(
                    static_cast<double>(runtime.ReadInt(scratch)) +
                    (static_cast<double>(roll) * static_cast<double>(0.001f) *
                        (static_cast<double>(high) - static_cast<double>(low)) +
                     static_cast<double>(low)));
                runtime.WriteWord(scratch,
                                  static_cast<std::uint32_t>(amount));
                const auto metric = runtime.ActorMetric(actor, 5u);
                runtime.WriteWord(scratch + 12u,
                    runtime.ReadWord(scratch + 12u) +
                    static_cast<std::uint32_t>(metric));

                for (std::uint32_t index = 0u; index < 4u; ++index) {
                    const auto item = runtime.ReadWord(
                        actor + 0x3a0u + index * 4u);
                    if (item == 0u) continue;
                    const auto item_roll = runtime.RandomNumber() % 1000u;
                    const auto probability = runtime.ReadFloat(
                        actor + 0x3b0u + index * 4u);
                    //Original x87 fnstsw/test4100 also accepts unordered NaN.
                    if (std::isnan(probability) || static_cast<double>(item_roll) *
                            static_cast<double>(0.001f) <=
                        static_cast<double>(probability)) {
                        const auto node = runtime.FindOrInsertReward(
                            scratch + 20u, item, 0u);
                        runtime.WriteWord(node + 16u,
                                          runtime.ReadWord(node + 16u) + 1u);
                    }
                }
            }
            runtime.AdvanceIterator(iterator);
        }

        const auto score = runtime.ReadWord(scratch);
        runtime.WriteWord(destination + 4u,
                          static_cast<std::uint32_t>(total));
        runtime.WriteByte(destination, static_cast<std::uint8_t>(score));
        runtime.WriteWord(destination + 8u,
                          static_cast<std::uint32_t>(rank));
        runtime.WriteWord(destination, score);
        runtime.WriteWord(destination + 16u,
                          runtime.ReadWord(scratch + 16u));
        runtime.WriteWord(destination + 12u,
                          runtime.ReadWord(scratch + 12u));
        runtime.AssignTree(destination + 20u, scratch + 20u);
        runtime.AssignTree(destination + 36u, scratch + 36u);
        for (std::uint32_t index = 0u; index < 4u; ++index)
            runtime.CopyReward(destination + 52u + index * 40u,
                               scratch + 52u + index * 40u);
    } catch (...) {
        if (iterator != 0u) runtime.DestroyIterator(iterator);
        runtime.DestroyReport(scratch);
        throw;
    }
    if (iterator != 0u) runtime.DestroyIterator(iterator);
    runtime.DestroyReport(scratch);
    return destination;
}

}  // namespace pal4
