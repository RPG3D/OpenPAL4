#include "pal4/combat_reward_processor.h"
#include "pal4/combat_result_experience_updater.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kSource = 0x1000u;
constexpr std::uint32_t kScratch = 0x2000u;
constexpr std::uint32_t kOutput = 0x3000u;
constexpr std::uint32_t kPlayer = 0x4000u;
constexpr std::uint32_t kSecondPlayer = 0x5000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatRewardProcessingRuntime,
                       pal4::ICombatResultExperienceRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> types;
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> tree_nodes;
    std::unordered_map<std::uint32_t, std::uint32_t> counters;
    std::vector<std::uint32_t> items;
    std::vector<std::uint32_t> rolls;
    std::vector<std::uint32_t> players;
    std::vector<std::uint32_t> selected_types;
    std::vector<std::string> calls;
    std::size_t roll_index = 0u;
    std::size_t player_index = 0u;
    std::uint32_t next_node = 0x9000u;
    std::uint32_t copied_trees = 0u;
    std::uint32_t copied_records = 0u;
    bool throw_on_copy = false;
    bool throw_on_matching = false;
    bool inactive = false;
    bool focus = false;
    bool state_three = false;
    bool level_up = false;
    std::int32_t applied_gain = -1;
    std::int32_t indexed_value = -1;
    std::int32_t target_level = 0;
    std::int32_t initial_indexed_value = 20;
    std::int32_t level_experience = -1;
    std::unordered_map<std::uint32_t, std::string> displayed;

    Runtime() {
        words[kSource] = 100u;
        words[kSource + 4u] = 88u;
        words[kSource + 12u] = 10u;
        words[kSource + 16u] = 3u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int32_t ReadInt(std::uint32_t address) override {
        return static_cast<std::int32_t>(words[address]);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        words[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void CopyTree(std::uint32_t destination,
                  std::uint32_t source) override {
        Check(destination - kScratch == source - kSource,
              "tree copied at matching offset");
        ++copied_trees;
        calls.emplace_back("tree-copy");
        for (const auto node : tree_nodes[source]) {
            const auto copy = next_node;
            next_node += 0x20u;
            words[copy + 12u] = words[node + 12u];
            words[copy + 16u] = words[node + 16u];
            tree_nodes[destination].push_back(copy);
        }
    }
    void CopyReward(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination - kScratch == source - kSource,
              "reward copied at matching offset");
        if (throw_on_copy && copied_records == 2u)
            throw std::runtime_error("copy failed");
        ++copied_records;
        calls.emplace_back("record-copy");
        for (std::uint32_t offset = 0u; offset < 40u; offset += 4u)
            words[destination + offset] = words[source + offset];
    }
    void CopyResult(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination == kOutput && source == kScratch,
              "copy final report");
        calls.emplace_back("result-copy");
        for (std::uint32_t offset = 0u; offset <= 16u; offset += 4u)
            words[destination + offset] = words[source + offset];
        for (std::uint32_t offset = 52u; offset < 212u; offset += 4u)
            words[destination + offset] = words[source + offset];
    }
    void DestroyReport(std::uint32_t report) override {
        calls.emplace_back("destroy:" + std::to_string(report));
    }
    void DestroyPartialReport(std::uint32_t report,
                              std::uint32_t trees,
                              std::uint32_t records) override {
        Check(report == kScratch && trees == 2u && records == 2u,
              "partial copy tracks completed members");
        calls.emplace_back("destroy-partial");
    }
    std::uint32_t GetGameCounter(std::uint32_t index) override {
        calls.emplace_back("counter-get:" + std::to_string(index));
        return counters[index];
    }
    void SetGameCounter(std::uint32_t index,
                        std::uint32_t value) override {
        counters[index] = value;
        calls.emplace_back("counter-set:" + std::to_string(index));
    }
    std::uint32_t ItemCount() override {
        return static_cast<std::uint32_t>(items.size());
    }
    std::uint32_t ItemAt(std::uint32_t index) override {
        return items.at(index);
    }
    std::uint32_t ItemId(std::uint32_t item) override {
        return item;
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        return rolls.at(roll_index++);
    }
    std::int32_t FloatToInt(double value) override {
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t FindOrInsertReward(std::uint32_t tree,
                                     std::uint32_t item_id) override {
        Check(tree == kScratch + 36u, "bonus uses second tree");
        selected_types.push_back(types[item_id]);
        calls.emplace_back("reward:" + std::to_string(item_id));
        for (const auto node : tree_nodes[tree])
            if (words[node + 12u] == item_id) return node;
        const auto node = next_node;
        next_node += 0x20u;
        words[node + 12u] = item_id;
        words[node + 16u] = 0u;
        tree_nodes[tree].push_back(node);
        return node;
    }
    void UpdateInventoryCounter(std::uint32_t value) override {
        calls.emplace_back("inventory-counter:" + std::to_string(value));
    }
    std::uint32_t TreeFirst(std::uint32_t tree) override {
        calls.emplace_back("tree-first:" + std::to_string(tree));
        return tree_nodes[tree].empty() ? TreeEnd(tree) :
                                        tree_nodes[tree].front();
    }
    std::uint32_t TreeEnd(std::uint32_t tree) override {
        return tree + 0x100000u;
    }
    std::uint32_t TreeNext(std::uint32_t node) override {
        for (auto& [tree, nodes] : tree_nodes)
            for (std::size_t index = 0u; index < nodes.size(); ++index)
                if (nodes[index] == node)
                    return index + 1u < nodes.size() ?
                           nodes[index + 1u] : TreeEnd(tree);
        throw std::runtime_error("missing tree node");
    }
    bool IsInventoryItem(std::uint32_t item_id) override {
        calls.emplace_back("inventory-check:" + std::to_string(item_id));
        return item_id == 201u || (item_id >= 101u && item_id <= 106u);
    }
    bool IsCollectionItem(std::uint32_t item_id) override {
        calls.emplace_back("collection-check:" + std::to_string(item_id));
        return item_id == 301u;
    }
    void AddInventoryItem(std::uint32_t item_id,
                          std::int32_t count) override {
        calls.emplace_back("inventory:" + std::to_string(item_id) + ":" +
                           std::to_string(count));
    }
    void AddCollectionItem(std::uint32_t item_id) override {
        calls.emplace_back("collection:" + std::to_string(item_id));
    }
    std::uint32_t FirstPlayerIterator() override {
        calls.emplace_back("players-first");
        return 0x7000u;
    }
    bool IteratorDone(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "player iterator identity");
        return player_index == players.size();
    }
    std::uint32_t IteratorActor(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "player actor identity");
        return players.at(player_index);
    }
    void AdvanceIterator(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "player next identity");
        ++player_index;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "player destructor identity");
        calls.emplace_back("players-destroy");
    }
    bool ActorReady(std::uint32_t actor) override {
        return actor != kSecondPlayer;
    }
    bool CombatInactive() override { return inactive; }
    bool IsFocusActor(std::uint32_t actor) override {
        return focus && actor == kSecondPlayer;
    }
    float ExperienceFactor(std::uint32_t index) override {
        calls.emplace_back("factor:" + std::to_string(index));
        return index == 30u ? 0.5f : index == 31u ? 0.5f : 0.25f;
    }
    bool ActorStateThree(std::uint32_t actor) override {
        return state_three && actor == kSecondPlayer;
    }
    std::int32_t ActorMetric(std::uint32_t actor,
                             std::uint32_t key) override {
        if (key == 4u) return static_cast<std::int32_t>(words[actor + 4u]);
        return static_cast<std::int32_t>(words[actor + 5u]);
    }
    void ApplyExperience(std::uint32_t actor,
                         std::int32_t amount) override {
        Check(actor == kPlayer || actor == kSecondPlayer,
              "apply experience actor");
        applied_gain = amount;
        words[actor + 5u] += static_cast<std::uint32_t>(amount);
        calls.emplace_back("apply-experience");
    }
    std::int32_t IndexedExperience(std::uint32_t) override { return initial_indexed_value; }
    void SetIndexedExperience(std::uint32_t,
                              std::int32_t value) override {
        indexed_value = value;
        calls.emplace_back("indexed-experience");
    }
    std::int32_t NextLevel(std::uint32_t,
                           std::int32_t experience) override {
        level_experience = experience;
        return target_level;
    }
    void SetActorLevel(std::uint32_t actor,
                       std::int32_t level) override {
        words[actor + 4u] = static_cast<std::uint32_t>(level);
        level_up = true;
        calls.emplace_back("set-level");
    }
    void UpdateActorState(std::uint32_t) override {
        calls.emplace_back("update-state");
    }
    std::uint32_t BuildMatchingRecords(std::uint32_t) override {
        calls.emplace_back("matching-build");
        return 0xb000u;
    }
    void CopyMatchingRecords(std::uint32_t reward,
                             std::uint32_t records) override {
        Check(records == 0xb000u && reward >= kScratch + 52u,
              "matching records copied to reward");
        if (throw_on_matching) throw std::runtime_error("matching copy failed");
        calls.emplace_back("matching-copy");
    }
    void ReleaseMatchingRecords(std::uint32_t records) override {
        Check(records == 0xb000u, "matching records release");
        calls.emplace_back("matching-release");
    }
    float ReadFloat(std::uint32_t address) override {
        return std::bit_cast<float>(ReadWord(address));
    }
    void WriteInt(std::uint32_t address, std::int32_t value) override {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    std::uint32_t ActorId(std::uint32_t) override { return 1001u; }
    std::int32_t NextLevelExperience(std::uint32_t, std::int32_t) override {
        return 100;
    }
    void SetWindowText(std::uint32_t window, std::string_view text) override {
        displayed[window] = text;
    }
    void ApplyLevelUp(std::uint32_t, std::int32_t, std::uint32_t,
                      std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected level-up during report/display integration");
    }
};

void AddSourceNode(Runtime& runtime, std::uint32_t tree,
                   std::uint32_t item, std::uint32_t count) {
    const auto node = runtime.next_node;
    runtime.next_node += 0x20u;
    runtime.words[node + 12u] = item;
    runtime.words[node + 16u] = count;
    runtime.tree_nodes[tree].push_back(node);
}

void RankCases() {
    constexpr std::array<std::array<std::uint32_t, 6>, 4> expected{{
        {{4u, 3u, 2u, 1u, 5u, 6u}},
        {{3u, 2u, 1u, 6u, 0u, 0u}},
        {{2u, 1u, 6u, 0u, 0u, 0u}},
        {{1u, 0u, 0u, 0u, 0u, 0u}},
    }};
    constexpr std::array<std::uint32_t, 5> counts{6u, 4u, 3u, 1u, 0u};
    for (std::uint32_t rank = 0u; rank <= 5u; ++rank) {
        Runtime runtime;
        runtime.words[kSource + 8u] = rank;
        for (std::uint32_t type = 1u; type <= 6u; ++type) {
            runtime.items.push_back(100u + type);
            runtime.types[100u + type] = type;
            runtime.words[100u + type + 152u] = type;
        }
        runtime.rolls.assign(6u, 0u);
        AddSourceNode(runtime, kSource + 20u, 201u, 2u);
        AddSourceNode(runtime, kSource + 20u, 301u, 3u);
        AddSourceNode(runtime, kSource + 20u, 401u, 4u);
        Check(pal4::CombatRewardProcessor::Process(
                  kOutput, kSource, kScratch, runtime) == kOutput,
              "process returns destination");
        const auto expected_count = rank <= 4u ? counts[rank] : 0u;
        Check(runtime.selected_types.size() == expected_count &&
                  runtime.roll_index == expected_count,
              "rank determines number of item pools and draws");
        for (std::size_t index = 0u; index < expected_count; ++index)
            Check(runtime.selected_types[index] == expected[rank][index],
                  "rank item type order matches IDA");
        Check(runtime.counters[rank + 2u] == (rank <= 4u ? 1u : 0u),
              "only ranks zero through four increment game counter");
        const auto multiplier = rank == 0u ? 1.9f :
                                rank == 1u ? 1.6f :
                                rank == 2u ? 1.3f : 1.0f;
        Check(runtime.words[kOutput] ==
                  static_cast<std::uint32_t>(
                      static_cast<std::int32_t>(
                          100.0 * static_cast<double>(multiplier))) &&
                  runtime.words[kOutput + 12u] ==
                      static_cast<std::uint32_t>(
                          static_cast<std::int32_t>(
                              10.0 * static_cast<double>(multiplier))),
              "money and experience multipliers");
        Check(runtime.calls.back() == "destroy:4096" &&
                  runtime.calls[runtime.calls.size() - 2u] == "destroy:8192" &&
                  runtime.calls[runtime.calls.size() - 3u] == "players-destroy",
              "cleanup follows final result copy");
        std::uint32_t collection_calls = 0u;
        for (const auto& call : runtime.calls)
            if (call == "collection:301") ++collection_calls;
        Check(collection_calls == 3u,
              "collection reward processed once per quantity");
    }
}

void PlayerExperienceAndLevelUp() {
    Runtime runtime;
    runtime.words[kSource + 8u] = 4u;
    runtime.players = {kPlayer, kSecondPlayer};
    runtime.words[kPlayer + 0x4a8u] = 4u;
    runtime.words[kSecondPlayer + 0x4a8u] = 1u;
    runtime.words[kSecondPlayer + 4u] = 2u;
    runtime.words[kSecondPlayer + 5u] = 20u;
    runtime.inactive = true;
    runtime.focus = true;
    runtime.target_level = 3;
    // IDA5715F0's first16 bytes are a vector, including capacity at12.
    // Distinct seeds expose an accidental gained-XP write into that vector.
    const auto source_record = kSource + 52u + 40u;
    runtime.words[source_record + 4u] = 0x11000u;
    runtime.words[source_record + 8u] = 0x11004u;
    runtime.words[source_record + 12u] = 0x11010u;
    (void)pal4::CombatRewardProcessor::Process(
        kOutput, kSource, kScratch, runtime);
    Check(runtime.words[kScratch + 52u + 40u + 36u] == 1u &&
              runtime.words[kScratch + 52u + 40u + 16u] == 3u &&
              runtime.applied_gain == 3 && runtime.indexed_value == 23,
          "experience uses readiness, inactivity and focus multipliers");
    Check(runtime.words[kScratch + 52u + 40u + 20u] == 2u &&
              runtime.words[kScratch + 52u + 40u + 24u] == 3u &&
              runtime.words[kScratch + 52u + 40u + 28u] == 20u &&
              runtime.words[kScratch + 52u + 40u + 32u] == 23u &&
              runtime.level_experience == 23 &&
              runtime.level_up,
          "record keeps prior and target level with updated experience");
    const auto output_record = kOutput + 52u + 40u;
    Check(runtime.words[output_record + 4u] == 0x11000u &&
              runtime.words[output_record + 8u] == 0x11004u &&
              runtime.words[output_record + 12u] == 0x11010u &&
              runtime.words[output_record + 16u] == 3u &&
              runtime.words[output_record + 28u] == 20u &&
              runtime.words[output_record + 32u] == 23u &&
              runtime.words[source_record + 16u] == 0u,
          "published report preserves learned-vector fields and before/after XP for ending UI");
    Check(runtime.calls[runtime.calls.size() - 6u] == "matching-copy" &&
              runtime.calls[runtime.calls.size() - 5u] == "matching-release" &&
              runtime.calls[runtime.calls.size() - 4u] == "result-copy",
          "level-up array copied before final result");
}

void EmptyPoolAndPartialCopy() {
    Runtime empty;
    empty.words[kSource + 8u] = 3u;
    empty.rolls = {7u};
    bool threw = false;
    try {
        (void)pal4::CombatRewardProcessor::Process(
            kOutput, kSource, kScratch, empty);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw && empty.roll_index == 1u &&
              empty.calls[empty.calls.size() - 2u] == "destroy:8192" &&
              empty.calls.back() == "destroy:4096",
          "empty pool becomes defined exception after native random draw");

    Runtime partial;
    partial.throw_on_copy = true;
    threw = false;
    try {
        (void)pal4::CombatRewardProcessor::Process(
            kOutput, kSource, kScratch, partial);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && partial.calls[partial.calls.size() - 2u] ==
                       "destroy-partial" &&
              partial.calls.back() == "destroy:4096",
          "partial report copy cleanup tracks two trees and two records");
}

void CandidateSelectionAndExperienceGates() {
    Runtime selected;
    selected.words[kSource + 8u] = 3u;
    selected.items = {101u, 111u, 121u};
    selected.types[101u] = 1u;
    selected.types[111u] = 1u;
    selected.types[121u] = 1u;
    selected.words[101u + 152u] = 1u;
    selected.words[111u + 152u] = 1u;
    selected.words[121u + 152u] = 1u;
    selected.rolls = {999u};
    (void)pal4::CombatRewardProcessor::Process(
        kOutput, kSource, kScratch, selected);
    Check(selected.calls.end() !=
              std::find(selected.calls.begin(), selected.calls.end(),
                        "reward:111"),
          "three-candidate pool selects middle item at roll 999");

    for (const bool state_gate : {false, true}) {
        Runtime gated;
        gated.words[kSource + 8u] = 4u;
        gated.players = {kSecondPlayer};
        gated.words[kSecondPlayer + 0x4a8u] = 1u;
        gated.words[kSecondPlayer + 4u] = state_gate ? 2u : 99u;
        gated.words[kSecondPlayer + 5u] = 20u;
        gated.state_three = state_gate;
        gated.target_level = state_gate ? 2 : 99;
        (void)pal4::CombatRewardProcessor::Process(
            kOutput, kSource, kScratch, gated);
        Check(gated.words[kScratch + 52u + 40u + 16u] == 0u &&
                  gated.applied_gain == 0 && !gated.level_up &&
                  gated.indexed_value == 20,
              "state-three and level-99 gates independently zero experience");
    }
}

void IndexedExperienceWrapsLikeOriginalAdd() {
    Runtime runtime;
    runtime.words[kSource + 8u] = 4u;
    runtime.players = {kPlayer};
    runtime.words[kPlayer + 0x4a8u] = 0u;
    runtime.words[kPlayer + 4u] = 2u;
    runtime.words[kPlayer + 5u] = 20u;
    runtime.initial_indexed_value = 0x7ffffffb;
    runtime.target_level = 2;
    (void)pal4::CombatRewardProcessor::Process(kOutput, kSource, kScratch, runtime);
    Check(std::bit_cast<std::uint32_t>(runtime.indexed_value) == 0x80000005u &&
              runtime.words[kOutput + 52u + 16u] == 10u &&
              runtime.words[kOutput + 52u + 28u] == 20u &&
              runtime.words[kOutput + 52u + 32u] == 30u &&
              !runtime.level_up,
          "indexed XP wraps as original ADD, independently of effective before/after XP");
}

void PreparedRewardFeedsRealExperienceDisplay() {
    Runtime runtime;
    runtime.words[kSource + 8u] = 4u;
    runtime.players = {kPlayer};
    runtime.words[kPlayer + 0x4a8u] = 0u;
    runtime.words[kPlayer + 4u] = 2u;
    runtime.words[kPlayer + 5u] = 20u;
    runtime.target_level = 2;
    (void)pal4::CombatRewardProcessor::Process(kOutput, kSource, kScratch, runtime);

    // Use the actual processor's output unchanged as the existing548CE0/
    // 548FB0 display input, without copying field values into another fixture.
    constexpr auto element = 0xc000u, row = 0xd000u;
    runtime.words[element + 60u] = row;
    runtime.words[element + 64u] = kOutput;
    runtime.words[element + 68u] = kPlayer;
    runtime.WriteFloat(element + 72u, 20.0f);
    runtime.WriteFloat(element + 76u, 5.0f);
    runtime.words[element + 80u] = 2u;
    runtime.WriteFloat(element + 84u, 80.0f);
    runtime.words[element + 88u] = 70u;
    runtime.words[row + 20u] = 1u;
    runtime.words[row + 24u] = 2u;
    Check(pal4::CombatResultExperienceUpdater::Update(element, 1.0f, runtime) == 1u &&
              runtime.ReadFloat(element + 72u) == 25.0f &&
              runtime.displayed[1u] == "5" && runtime.displayed[2u] == "75" &&
              pal4::CombatResultExperienceUpdater::Complete(element, runtime) == 0u,
          "actual reward report drives half-complete XP animation");
    (void)pal4::CombatResultExperienceUpdater::Update(element, 10.0f, runtime);
    Check(runtime.ReadFloat(element + 72u) == 30.0f &&
              runtime.displayed[1u] == "10" && runtime.displayed[2u] == "70" &&
              pal4::CombatResultExperienceUpdater::Complete(element, runtime) == 1u &&
              runtime.displayed[1u] == "10",
          "actual report caps display at post-reward XP and completes at correct gain");
}

void MatchingCopyFailureUnwinds() {
    Runtime runtime;
    runtime.words[kSource + 8u] = 4u;
    runtime.players = {kPlayer};
    runtime.words[kPlayer + 0x4a8u] = 0u;
    runtime.words[kPlayer + 4u] = 2u;
    runtime.words[kPlayer + 5u] = 20u;
    runtime.target_level = 3;
    runtime.throw_on_matching = true;
    bool threw = false;
    try {
        (void)pal4::CombatRewardProcessor::Process(
            kOutput, kSource, kScratch, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls[runtime.calls.size() - 4u] ==
                       "matching-release" &&
              runtime.calls[runtime.calls.size() - 3u] == "players-destroy" &&
              runtime.calls[runtime.calls.size() - 2u] == "destroy:8192" &&
              runtime.calls.back() == "destroy:4096",
          "matching array, iterator and reports unwind in order");
}

void ItemTypeComparison() {
    Runtime runtime;
    runtime.words[0x8000u + 152u] = 6u;
    Check(pal4::CombatRewardProcessor::MatchesItemType(
              6u, 0x8000u, runtime) &&
              !pal4::CombatRewardProcessor::MatchesItemType(
                  5u, 0x8000u, runtime),
          "item type compares only word at offset 152");
}

}  // namespace

int main() {
    try {
        RankCases();
        PlayerExperienceAndLevelUp();
        IndexedExperienceWrapsLikeOriginalAdd();
        PreparedRewardFeedsRealExperienceDisplay();
        EmptyPoolAndPartialCopy();
        CandidateSelectionAndExperienceGates();
        MatchingCopyFailureUnwinds();
        ItemTypeComparison();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
