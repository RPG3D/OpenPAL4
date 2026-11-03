#include "pal4/combat_reward_processor.h"
#include "pal4/combat_reward_record_layout.h"

#include <array>
#include <bit>
#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

void AddRandomReward(std::uint32_t tree, std::uint32_t type,
                     ICombatRewardProcessingRuntime& runtime) {
    std::vector<std::uint32_t> matches;
    for (std::uint32_t index = 0u; index < runtime.ItemCount(); ++index) {
        const auto item = runtime.ItemAt(index);
        if (CombatRewardProcessor::MatchesItemType(type, item, runtime))
            matches.push_back(item);
    }
    const auto roll = runtime.RandomNumber() % 1000u;
    if (matches.empty()) throw std::out_of_range("empty battle reward pool");
    const float maximum = static_cast<float>(matches.size() - 1u);
    const auto selected = runtime.FloatToInt(
        static_cast<double>(roll) * static_cast<double>(0.001f) *
        static_cast<double>(maximum));
    if (selected < 0 || static_cast<std::size_t>(selected) >= matches.size())
        throw std::out_of_range("battle reward index");
    const auto id = runtime.ItemId(matches[static_cast<std::size_t>(selected)]);
    const auto node = runtime.FindOrInsertReward(tree, id);
    runtime.WriteWord(node + 16u, runtime.ReadWord(node + 16u) + 1u);
}

void ApplyRankRewards(std::uint32_t report,
                      ICombatRewardProcessingRuntime& runtime) {
    const auto rank = runtime.ReadWord(report + 8u);
    if (rank > 4u) return;
    const auto counter = rank + 2u;
    runtime.SetGameCounter(counter, runtime.GetGameCounter(counter) + 1u);
    constexpr std::array<std::array<std::uint32_t, 6>, 4> kTypes{{
        {{4u, 3u, 2u, 1u, 5u, 6u}},
        {{3u, 2u, 1u, 6u, 0u, 0u}},
        {{2u, 1u, 6u, 0u, 0u, 0u}},
        {{1u, 0u, 0u, 0u, 0u, 0u}},
    }};
    constexpr std::array<std::uint32_t, 4> kCounts{6u, 4u, 3u, 1u};
    if (rank == 4u) return;
    for (std::uint32_t index = 0u; index < kCounts[rank]; ++index)
        AddRandomReward(report + 36u, kTypes[rank][index], runtime);
    if (rank < 3u) {
        constexpr std::array<float, 3> kMultipliers{1.9f, 1.6f, 1.3f};
        const auto multiplier = static_cast<double>(kMultipliers[rank]);
        runtime.WriteWord(report + 12u,
            static_cast<std::uint32_t>(runtime.FloatToInt(
                static_cast<double>(runtime.ReadInt(report + 12u)) *
                multiplier)));
        runtime.WriteWord(report,
            static_cast<std::uint32_t>(runtime.FloatToInt(
                static_cast<double>(runtime.ReadInt(report)) * multiplier)));
    }
}

void GrantTree(std::uint32_t tree,
               ICombatRewardProcessingRuntime& runtime) {
    const auto end = runtime.TreeEnd(tree);
    for (auto node = runtime.TreeFirst(tree); node != end;
         node = runtime.TreeNext(node)) {
        const auto item = runtime.ReadWord(node + 12u);
        const auto count = runtime.ReadInt(node + 16u);
        if (runtime.IsInventoryItem(item)) {
            runtime.AddInventoryItem(item, count);
        } else if (runtime.IsCollectionItem(item)) {
            for (std::int32_t index = 0; index < count; ++index)
                runtime.AddCollectionItem(item);
        }
    }
}

void UpdatePlayer(std::uint32_t report, std::uint32_t actor,
                  ICombatRewardProcessingRuntime& runtime) {
    const auto slot = runtime.ReadInt(actor + 0x4a8u);
    if (slot < 0 || slot >= 4) return;
    using Record = CombatRewardRecordLayout;
    const auto reward = report + 52u + static_cast<std::uint32_t>(slot) * Record::kBytes;
    runtime.WriteByte(reward + Record::kActive, 1u);

    float experience = static_cast<float>(runtime.ReadInt(report + 12u));
    if (!runtime.ActorReady(actor))
        experience = static_cast<float>(
            static_cast<double>(runtime.ExperienceFactor(30u)) * experience);
    if (runtime.CombatInactive())
        experience = static_cast<float>(
            static_cast<double>(runtime.ExperienceFactor(31u)) * experience);
    if (runtime.IsFocusActor(actor))
        experience = static_cast<float>(
            (static_cast<double>(runtime.ExperienceFactor(29u)) + 1.0) *
            experience);
    if (runtime.ActorStateThree(actor)) experience = 0.0f;
    if (runtime.ActorMetric(actor, 4u) >= 99) experience = 0.0f;

    runtime.WriteWord(reward + Record::kPreviousExperience,
        static_cast<std::uint32_t>(runtime.ActorMetric(actor, 5u)));
    const auto gained = runtime.FloatToInt(static_cast<double>(experience));
    runtime.WriteWord(reward + Record::kGainedExperience, static_cast<std::uint32_t>(gained));
    runtime.ApplyExperience(actor, gained);
    // Original ADD wraps in32 bits before the validated setter.
    runtime.SetIndexedExperience(actor, std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(runtime.IndexedExperience(actor)) +
        static_cast<std::uint32_t>(gained)));
    runtime.WriteWord(reward + Record::kUpdatedExperience,
        static_cast<std::uint32_t>(runtime.ActorMetric(actor, 5u)));

    const auto target_level = runtime.NextLevel(
        actor, runtime.ActorMetric(actor, 5u));
    runtime.WriteWord(reward + Record::kPreviousLevel,
        static_cast<std::uint32_t>(runtime.ActorMetric(actor, 4u)));
    runtime.WriteWord(reward + Record::kTargetLevel,
        static_cast<std::uint32_t>(target_level));
    if (target_level > runtime.ActorMetric(actor, 4u)) {
        runtime.SetActorLevel(actor, target_level);
        runtime.UpdateActorState(actor);
        const auto records = runtime.BuildMatchingRecords(actor);
        try {
            runtime.CopyMatchingRecords(reward, records);
        } catch (...) {
            runtime.ReleaseMatchingRecords(records);
            throw;
        }
        runtime.ReleaseMatchingRecords(records);
    }
}

}  // namespace

bool CombatRewardProcessor::MatchesItemType(
    std::uint32_t type, std::uint32_t item,
    ICombatRewardProcessingRuntime& runtime) {
    return type == runtime.ReadWord(item + 152u);
}

std::uint32_t CombatRewardProcessor::Process(
    std::uint32_t destination, std::uint32_t source,
    std::uint32_t scratch, ICombatRewardProcessingRuntime& runtime) {
    std::uint32_t players = 0u;
    bool scratch_ready = false;
    std::uint32_t copied_trees = 0u;
    std::uint32_t copied_records = 0u;
    try {
        // The by-value report is copied into the native stack object in this order.
        for (const auto offset : {4u, 8u, 0u, 16u, 12u})
            runtime.WriteWord(scratch + offset,
                              runtime.ReadWord(source + offset));
        runtime.CopyTree(scratch + 20u, source + 20u);
        ++copied_trees;
        runtime.CopyTree(scratch + 36u, source + 36u);
        ++copied_trees;
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            runtime.CopyReward(scratch + 52u + index * 40u,
                               source + 52u + index * 40u);
            ++copied_records;
        }
        scratch_ready = true;

        ApplyRankRewards(scratch, runtime);
        runtime.UpdateInventoryCounter(runtime.ReadWord(scratch));
        GrantTree(scratch + 20u, runtime);
        GrantTree(scratch + 36u, runtime);

        players = runtime.FirstPlayerIterator();
        while (!runtime.IteratorDone(players)) {
            UpdatePlayer(scratch, runtime.IteratorActor(players), runtime);
            runtime.AdvanceIterator(players);
        }
        runtime.CopyResult(destination, scratch);
    } catch (...) {
        if (players != 0u) runtime.DestroyIterator(players);
        if (scratch_ready)
            runtime.DestroyReport(scratch);
        else
            runtime.DestroyPartialReport(scratch, copied_trees,
                                         copied_records);
        runtime.DestroyReport(source);
        throw;
    }
    if (players != 0u) runtime.DestroyIterator(players);
    runtime.DestroyReport(scratch);
    runtime.DestroyReport(source);
    return destination;
}

}  // namespace pal4
