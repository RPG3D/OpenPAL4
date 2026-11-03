#include "pal4/system_event_router_tree.h"
#include "pal4/script_event_router.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISystemEventRouterTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> delete_flags;
    std::uint32_t next = 0x8000u;
    std::uint32_t fail_on_allocation = 0;
    std::uint32_t allocation_count = 0;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        ++allocation_count;
        sizes.push_back(size);
        if (allocation_count == fail_on_allocation) return 0;
        const auto address = next;
        next += 0x100u;
        return address;
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void CleanupPayload(const std::uint32_t address) override {
        cleaned.push_back(address);
    }
    std::uint32_t DeleteCallback(const std::uint32_t address,
                                 const std::uint32_t flags) override {
        deleted.push_back(address);
        delete_flags.push_back(flags);
        return 0x1234u;
    }
};

constexpr auto tree = 0x5000u;
constexpr auto head = 0x6000u;
constexpr auto nil = 0x7000u;

void Initialize(Runtime& runtime) {
    runtime.words[tree + 4u] = head;
    runtime.words[head] = head;
    runtime.words[head + 4u] = nil;
    runtime.words[head + 8u] = head;
    runtime.words[nil + 28u] = 1;
}

void ScriptRouterRegistration() {
    Runtime runtime;
    Initialize(runtime);
    runtime.words[pal4::ScriptEventRouter::kNilPointer] = nil;
    const auto first = pal4::ScriptEventRouter::RegisterHandler(
        tree, 3u, 44u, 0x1234u, 0x5678u, runtime);
    const auto second = pal4::ScriptEventRouter::RegisterHandler(
        tree, 1u, 44u, 0x2222u, 0x3333u, runtime);
    const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
        tree, 44u, nil, runtime);
    const auto list_head = runtime.words[group + 4u];
    const auto first_node = runtime.words[list_head];
    const auto second_node = runtime.words[first_node];
    Check(first != 0u && second != 0u &&
              runtime.words[tree + 12u] == 1u &&
              runtime.words[group + 8u] == 2u &&
              runtime.words[first_node + 16u] == 1u &&
              runtime.words[second_node + 16u] == 3u &&
              runtime.words[first_node + 12u] == second &&
              runtime.words[second_node + 12u] == first &&
              runtime.words[second + 4u] == 0x2222u &&
              runtime.words[second + 8u] == 0x3333u &&
              runtime.words[first] == 0x844910u,
          "script router shares one event group and sorts handlers by mode");
}

int CheckNode(const std::uint32_t node, const std::uint32_t parent,
              const std::uint32_t* lower, const std::uint32_t* upper,
              Runtime& runtime) {
    if (node == nil) return 1;
    const auto key = runtime.words[node + 12u];
    Check(runtime.words[node + 4u] == parent &&
          (!lower || *lower < key) && (!upper || key < *upper),
          "integer tree parent and search order");
    const auto left = runtime.words[node];
    const auto right = runtime.words[node + 8u];
    const auto black = runtime.words[node + 28u] != 0u;
    if (!black) {
        Check(left == nil || runtime.words[left + 28u] != 0u,
              "red node has black left child");
        Check(right == nil || runtime.words[right + 28u] != 0u,
              "red node has black right child");
    }
    const auto list_head = runtime.words[node + 20u];
    Check(runtime.bytes[node + 16u] == static_cast<std::uint8_t>(key) &&
          runtime.words[node + 24u] == 0 &&
          runtime.words[list_head] == list_head &&
          runtime.words[list_head + 4u] == list_head,
          "group byte, empty list and count layout");
    const auto left_height = CheckNode(left, node, lower, &key, runtime);
    const auto right_height = CheckNode(right, node, &key, upper, runtime);
    Check(left_height == right_height, "equal black height");
    return left_height + (black ? 1 : 0);
}

void CheckTree(const std::set<std::uint32_t>& expected, Runtime& runtime) {
    Check(runtime.words[tree + 12u] == expected.size(), "tree count");
    const auto root = runtime.words[head + 4u];
    if (expected.empty()) {
        Check(root == nil && runtime.words[head] == head &&
              runtime.words[head + 8u] == head, "empty head links");
        return;
    }
    Check(runtime.words[root + 28u] == 1u &&
          runtime.words[root + 4u] == head,
          "root color and parent");
    (void)CheckNode(root, head, nullptr, nullptr, runtime);
    Check(runtime.words[runtime.words[head] + 12u] == *expected.begin() &&
          runtime.words[runtime.words[head + 8u] + 12u] == *expected.rbegin(),
          "head minimum and maximum");
    for (const auto key : expected) {
        const auto node = pal4::SystemEventRouterTree::LowerBound(
            tree, key, nil, runtime);
        Check(node != head && runtime.words[node + 12u] == key,
              "all inserted keys remain searchable");
    }
}

void IntegerTreeInsertion() {
    Runtime runtime;
    Initialize(runtime);
    std::set<std::uint32_t> expected;
    for (const auto key : {50u, 20u, 80u, 10u, 40u, 70u, 90u, 30u, 60u,
                           100u, 5u, 15u, 25u, 35u, 45u, 55u, 65u, 75u,
                           85u, 95u}) {
        const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
            tree, key, nil, runtime);
        Check(group != 0 && runtime.words[group - 4u] == key,
              "new group points into keyed node");
        expected.insert(key);
        CheckTree(expected, runtime);
    }
    const auto allocations = runtime.allocation_count;
    const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
        tree, 40u, nil, runtime);
    Check(runtime.words[group - 4u] == 40u &&
          runtime.allocation_count == allocations &&
          pal4::SystemEventRouterTree::LowerBound(
              tree, 41u, nil, runtime) ==
              pal4::SystemEventRouterTree::LowerBound(
                  tree, 45u, nil, runtime),
          "duplicate reuses group and lower bound finds next key");
    Check(runtime.sizes.size() == 40u &&
          runtime.sizes.front() == 32u && runtime.sizes[1] == 20u,
          "each unique group owns a 32-byte node and 20-byte list head");
}

void AllocationFailures() {
    Runtime runtime;
    Initialize(runtime);
    runtime.fail_on_allocation = 1;
    Check(pal4::SystemEventRouterTree::FindOrCreateGroup(
              tree, 1u, nil, runtime) == 0 &&
          runtime.freed.empty(), "node allocation failure");
    runtime.fail_on_allocation = 3;
    Check(pal4::SystemEventRouterTree::FindOrCreateGroup(
              tree, 2u, nil, runtime) == 0 &&
          runtime.freed == std::vector<std::uint32_t>{0x8000u},
          "list-head failure frees unlinked node");
    CheckTree({}, runtime);
}

void ListCleanupVariants() {
    Runtime runtime;
    constexpr auto nested = 0x9000u;
    constexpr auto sentinel = 0xa000u;
    constexpr auto first = 0xa100u;
    constexpr auto second = 0xa200u;
    runtime.words[nested + 8u] = sentinel;
    runtime.words[nested + 12u] = 2;
    runtime.words[sentinel] = first;
    runtime.words[sentinel + 4u] = second;
    runtime.words[first] = second;
    runtime.words[first + 4u] = sentinel;
    runtime.words[second] = sentinel;
    runtime.words[second + 4u] = first;
    pal4::SystemEventRouterTree::CleanupNestedList(nested, runtime);
    Check(runtime.cleaned ==
              std::vector<std::uint32_t>{first + 8u, second + 8u} &&
          runtime.freed ==
              std::vector<std::uint32_t>{first, second, sentinel} &&
          runtime.words[nested + 8u] == 0 &&
          runtime.words[nested + 12u] == 0,
          "nested list unlinks and cleans payload before free");
    constexpr auto group = 0xb000u;
    constexpr auto group_head = 0xc000u;
    constexpr auto member = 0xc100u;
    runtime.words[group + 4u] = group_head;
    runtime.words[group + 8u] = 1;
    runtime.words[group_head] = member;
    runtime.words[group_head + 4u] = member;
    runtime.words[member] = group_head;
    runtime.words[member + 4u] = group_head;
    pal4::SystemEventRouterTree::CleanupGroupList(group, runtime);
    Check(runtime.cleaned.back() == member + 8u &&
          runtime.freed[runtime.freed.size() - 2] == member &&
          runtime.freed.back() == group_head &&
          runtime.words[group + 4u] == 0 &&
          runtime.words[group + 8u] == 0,
          "group list uses +4/+8 layout");
}

void DirectHelpersAndDuplicateMode() {
    Runtime runtime;
    Initialize(runtime);
    runtime.words[tree + 8u] = 1u;
    const auto first = pal4::SystemEventRouterTree::InsertNode(
        tree, 42u, nil, runtime);
    const auto duplicate = pal4::SystemEventRouterTree::InsertNode(
        tree, 42u, nil, runtime);
    Check(first && duplicate && first != duplicate &&
          runtime.words[tree + 12u] == 2u &&
          pal4::SystemEventRouterTree::LowerBound(
              tree, 42u, nil, runtime) == first,
          "duplicate-enabled tree retains both integer keys");
    Runtime helper;
    const auto node = pal4::SystemEventRouterTree::AllocateNode(
        0x1234u, 1u, helper);
    Check(helper.sizes == std::vector<std::uint32_t>{32u} &&
          helper.words[node + 4u] == 0x1234u &&
          helper.words[node + 28u] == 1u,
          "32-byte node allocator initializes parent and color");
    const auto empty = pal4::SystemEventRouterTree::CreateEmptyList(
        0, 0, helper);
    const auto linked = pal4::SystemEventRouterTree::CreateEmptyList(
        0x1111u, 0x2222u, helper);
    Check(helper.words[empty] == empty &&
          helper.words[empty + 4u] == empty &&
          helper.words[linked] == 0x1111u &&
          helper.words[linked + 4u] == 0x2222u,
          "empty-list factory uses self-links only for null arguments");
}

void RangeRemoval() {
    Runtime runtime;
    constexpr auto holder = 0x9000u;
    constexpr auto sentinel = 0xa000u;
    constexpr auto first = 0xa100u;
    constexpr auto second = 0xa200u;
    constexpr auto output = 0xb000u;
    runtime.words[holder + 8u] = 2;
    runtime.words[sentinel] = first;
    runtime.words[sentinel + 4u] = second;
    runtime.words[first] = second;
    runtime.words[first + 4u] = sentinel;
    runtime.words[second] = sentinel;
    runtime.words[second + 4u] = first;
    pal4::SystemEventRouterTree::RemoveListRange(
        holder, output, first, second, runtime);
    Check(runtime.cleaned == std::vector<std::uint32_t>{first + 8u} &&
          runtime.freed == std::vector<std::uint32_t>{first} &&
          runtime.words[sentinel] == second &&
          runtime.words[second + 4u] == sentinel &&
          runtime.words[holder + 8u] == 1u &&
          runtime.words[output] == second,
          "partial range removal preserves tail node");
    pal4::SystemEventRouterTree::RemoveListRange(
        holder, output, second, sentinel, runtime);
    Check(runtime.words[sentinel] == sentinel &&
          runtime.words[sentinel + 4u] == sentinel &&
          runtime.words[holder + 8u] == 0 &&
          runtime.words[output] == sentinel &&
          runtime.freed == std::vector<std::uint32_t>{first, second},
          "remaining range removal reaches sentinel and zero count");
}

void PredecessorAndExactLookup() {
    Runtime runtime;
    Initialize(runtime);
    for (const auto key : {40u, 20u, 60u, 10u, 30u, 50u, 70u})
        Check(pal4::SystemEventRouterTree::InsertNode(
                  tree, key, nil, runtime) != 0, "insert lookup fixture");
    const auto node_40 = pal4::SystemEventRouterTree::FindExact(
        tree, 40u, nil, runtime);
    const auto node_30 = pal4::SystemEventRouterTree::FindExact(
        tree, 30u, nil, runtime);
    const auto node_10 = pal4::SystemEventRouterTree::FindExact(
        tree, 10u, nil, runtime);
    Check(pal4::SystemEventRouterTree::Predecessor(
              node_40, head, nil, runtime) == node_30 &&
          pal4::SystemEventRouterTree::Predecessor(
              node_10, head, nil, runtime) == head &&
          runtime.words[pal4::SystemEventRouterTree::Predecessor(
              head, head, nil, runtime) + 12u] == 70u &&
          pal4::SystemEventRouterTree::FindExact(
              tree, 41u, nil, runtime) == head,
          "predecessor handles subtree, minimum and head; exact miss returns head");
}

void RegistrationAndCopies() {
    Runtime runtime;
    Initialize(runtime);
    const auto mode_three = pal4::SystemEventRouterTree::Register(
        tree, 3u, 44u, 0x1234u, 0x5678u, nil, runtime);
    const auto mode_two = pal4::SystemEventRouterTree::Register(
        tree, 2u, 44u, 0x2222u, 0x3333u, nil, runtime);
    const auto second_two = pal4::SystemEventRouterTree::Register(
        tree, 2u, 44u, 0x4444u, 0x5555u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
        tree, 44u, nil, runtime);
    const auto list_head = runtime.words[group + 4u];
    const auto first = runtime.words[list_head];
    const auto second = runtime.words[first];
    const auto third = runtime.words[second];
    Check(mode_three && mode_two && second_two &&
          runtime.words[group + 8u] == 3u &&
          runtime.words[first + 12u] == second_two &&
          runtime.words[second + 12u] == mode_two &&
          runtime.words[third + 12u] == mode_three &&
          runtime.words[third] == list_head &&
          runtime.words[mode_three] == 0x844910u &&
          runtime.words[mode_three + 4u] == 0x1234u &&
          runtime.words[mode_three + 8u] == 0x5678u &&
          runtime.bytes[first + 8u] == 1u,
          "registration orders by mode and owns 12-byte callback objects");
    Check(pal4::SystemEventRouterTree::LowerBoundMode(
              first, list_head, 2u, runtime) == first &&
          pal4::SystemEventRouterTree::LowerBoundMode(
              first, list_head, 3u, runtime) == third &&
          pal4::SystemEventRouterTree::LowerBoundMode(
              first, list_head, 4u, runtime) == list_head &&
          pal4::SystemEventRouterTree::LowerBoundMode(
              list_head, list_head, 2u, runtime) == list_head,
          "list lower bound finds first equal, greater and end");

    constexpr auto copy = 0xd000u;
    Check(pal4::SystemEventRouterTree::CopyGroupRecord(
              copy, group - 4u, false, runtime), "deep-copy group record");
    const auto copied_head = runtime.words[copy + 8u];
    const auto copied_first = runtime.words[copied_head];
    const auto copied_handler = runtime.words[copied_first + 12u];
    Check(runtime.words[copy] == 44u &&
          runtime.bytes[copy + 4u] == 44u &&
          runtime.words[copy + 12u] == 3u &&
          copied_handler != second_two &&
          runtime.words[copied_handler + 4u] == 0x4444u &&
          runtime.bytes[copied_first + 8u] == 1u &&
          runtime.bytes[first + 8u] == 1u,
          "deep copy duplicates callbacks without changing source ownership");

    constexpr auto moved = 0xe000u;
    Check(pal4::SystemEventRouterTree::CopyGroupRecord(
              moved, group - 4u, true, runtime), "move group record");
    const auto moved_first = runtime.words[runtime.words[moved + 8u]];
    Check(runtime.words[moved + 12u] == 3u &&
          runtime.words[moved_first + 12u] == second_two &&
          runtime.bytes[moved_first + 8u] == 1u &&
          runtime.bytes[first + 8u] == 0u &&
          pal4::SystemEventRouterTree::ResetPayload(
              moved_first + 8u, runtime) == second_two &&
          runtime.bytes[moved_first + 8u] == 0u,
          "move transfers ownership flag and reset returns callback pointer");
}

void RegistrationAllocationFailures() {
    Runtime runtime;
    Initialize(runtime);
    Check(pal4::SystemEventRouterTree::InsertNode(
              tree, 15u, nil, runtime) != 0, "failure fixture group");
    const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
        tree, 15u, nil, runtime);
    runtime.fail_on_allocation = runtime.allocation_count + 1u;
    Check(pal4::SystemEventRouterTree::Register(
              tree, 2u, 15u, 1u, 2u, nil, runtime) == 0 &&
          runtime.words[group + 8u] == 1u &&
          runtime.words[runtime.words[group + 4u] + 4u] != 0u &&
          runtime.bytes[runtime.words[runtime.words[group + 4u]] + 8u] == 0u &&
          runtime.words[runtime.words[runtime.words[group + 4u]] + 12u] == 0u,
          "callback allocation failure inserts an unowned null callback");
    runtime.fail_on_allocation = runtime.allocation_count + 2u;
    Check(pal4::SystemEventRouterTree::Register(
              tree, 2u, 15u, 1u, 2u, nil, runtime) == 0 &&
          runtime.words[group + 8u] == 1u &&
          runtime.freed.size() == 1u,
          "list allocation failure frees unlinked callback");
}

void ActorManagerRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::ActorManagerEventRouter::Register(
        tree, 3u, 77u, 0x1234u, 0x5678u, nil, runtime);
    const auto earlier = pal4::ActorManagerPlayerEventRouter::Register(
        tree, 2u, 77u, 0x2345u, 0x6789u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
        tree, 77u, nil, runtime);
    const auto list_head = runtime.words[group + 4u];
    const auto first = runtime.words[list_head];
    const auto second = runtime.words[first];
    Check(later && earlier && runtime.words[tree + 12u] == 1u &&
          runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 3u &&
          runtime.words[earlier] == 0x844910u &&
          runtime.words[earlier + 4u] == 0x2345u &&
          runtime.words[earlier + 8u] == 0x6789u &&
          runtime.bytes[first + 8u] == 1u &&
          runtime.sizes == std::vector<std::uint32_t>{
              32u, 20u, 12u, 20u, 12u, 20u},
          "actor and player managers register ordered callbacks in one group");

    Runtime failed;
    Initialize(failed);
    failed.fail_on_allocation = 1u;
    Check(pal4::ActorManagerEventRouter::Register(
              tree, 2u, 77u, 1u, 2u, nil, failed) == 0u &&
          failed.words[tree + 12u] == 0u,
          "actor manager registration preserves tree on allocation failure");

    Runtime player_failed;
    Initialize(player_failed);
    player_failed.fail_on_allocation = 1u;
    Check(pal4::ActorManagerPlayerEventRouter::Register(
              tree, 2u, 77u, 1u, 2u, nil, player_failed) == 0u &&
          player_failed.words[tree + 12u] == 0u,
          "player registration preserves tree on allocation failure");

    Runtime late;
    Initialize(late);
    const auto first_late = pal4::ActorManagerEventRouter::RegisterLate(
        tree, 3u, 91u, 0x1111u, 0x2222u, nil, late);
    const auto second_late = pal4::ActorManagerEventRouter::RegisterLate(
        tree, 1u, 91u, 0x3333u, 0x4444u, nil, late);
    const auto late_group = pal4::SystemEventRouterTree::FindExact(
        tree, 91u, nil, late) + 16u;
    const auto late_head = late.words[late_group + 4u];
    const auto late_first = late.words[late_head];
    Check(first_late && second_late &&
          late.words[late_group + 8u] == 2u &&
          late.words[late_first + 12u] == second_late &&
          late.words[late_first + 16u] == 1u &&
          late.words[late.words[late_first] + 12u] == first_late &&
          late.words[first_late + 4u] == 0x1111u &&
          late.words[second_late + 8u] == 0x4444u,
          "late actor manager entry preserves keyed group and mode ordering");

    Runtime combat;
    Initialize(combat);
    const auto first_combat = pal4::ActorManagerEventRouter::RegisterCombatHandler(
        tree, 4u, 95u, 0x5555u, 0x6666u, nil, combat);
    const auto second_combat = pal4::ActorManagerEventRouter::RegisterCombatHandler(
        tree, 1u, 95u, 0x7777u, 0x8888u, nil, combat);
    const auto combat_group = pal4::SystemEventRouterTree::FindExact(
        tree, 95u, nil, combat) + 16u;
    const auto combat_head = combat.words[combat_group + 4u];
    const auto combat_first = combat.words[combat_head];
    Check(first_combat && second_combat &&
          combat.words[combat_group + 8u] == 2u &&
          combat.words[combat_first + 12u] == second_combat &&
          combat.words[combat_first + 16u] == 1u &&
          combat.words[combat.words[combat_first] + 12u] == first_combat &&
          combat.words[first_combat] == 0x844910u &&
          combat.words[first_combat + 4u] == 0x5555u &&
          combat.words[second_combat + 8u] == 0x8888u,
          "combat handler entry preserves callback fields and mode ordering");

    Runtime failed_combat;
    Initialize(failed_combat);
    failed_combat.fail_on_allocation = 1u;
    Check(pal4::ActorManagerEventRouter::RegisterCombatHandler(
              tree, 1u, 95u, 1u, 2u, nil, failed_combat) == 0u &&
          failed_combat.words[tree + 12u] == 0u,
          "combat handler entry leaves tree empty on group allocation failure");
}

void WorldManagerRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::WorldManagerEventRouter::Register(
        tree, 3u, 111u, 0x3100u, 0x4100u, nil, runtime);
    const auto earlier = pal4::WorldManagerEventRouter::Register(
        tree, 1u, 111u, 0x3200u, 0x4200u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 111u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    Check(later != 0u && earlier != 0u &&
              runtime.words[group + 8u] == 2u &&
              runtime.words[first + 12u] == earlier &&
              runtime.words[first + 16u] == 1u &&
              runtime.words[runtime.words[first] + 12u] == later &&
              runtime.words[later] == 0x844910u &&
              runtime.words[later + 4u] == 0x3100u &&
              runtime.words[earlier + 8u] == 0x4200u,
          "world manager registration uses the shared ordered event tree");

    Runtime failed;
    Initialize(failed);
    failed.fail_on_allocation = 1u;
    Check(pal4::WorldManagerEventRouter::Register(
              tree, 2u, 111u, 1u, 2u, nil, failed) == 0u &&
              failed.words[tree + 12u] == 0u,
          "world manager registration preserves tree on group failure");
}

void CombatConsoleRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatConsoleEventRouter::Register(
        tree, 3u, 112u, 0x5100u, 0x6100u, nil, runtime);
    const auto earlier = pal4::CombatConsoleEventRouter::Register(
        tree, 2u, 112u, 0x5200u, 0x6200u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 112u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    Check(later != 0u && earlier != 0u &&
              runtime.words[group + 8u] == 2u &&
              runtime.words[first + 12u] == earlier &&
              runtime.words[first + 16u] == 2u &&
              runtime.words[runtime.words[first] + 12u] == later &&
              runtime.words[later] == 0x844910u &&
              runtime.words[later + 4u] == 0x5100u &&
              runtime.words[earlier + 8u] == 0x6200u,
          "combat console uses shared mode-ordered event tree");

    Runtime failed;
    Initialize(failed);
    failed.fail_on_allocation = 1u;
    Check(pal4::CombatConsoleEventRouter::Register(
              tree, 2u, 112u, 1u, 2u, nil, failed) == 0u &&
              failed.words[tree + 12u] == 0u,
          "combat console registration preserves tree on group failure");
}

void CombatActionWindowRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatActionConsoleWindowEventRouter::Register(
        tree, 4u, 113u, 0xfffffff0u, 0x6100u, 0x20u, nil, runtime);
    const auto earlier = pal4::CombatActionConsoleWindowEventRouter::Register(
        tree, 1u, 113u, 0x5200u, 0x6200u, 3u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 113u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    Check(later && earlier && runtime.words[group + 8u] == 2u &&
              runtime.words[first + 12u] == earlier &&
              runtime.words[first + 16u] == 1u &&
              runtime.words[runtime.words[first] + 12u] == later &&
              runtime.words[later] == 0x844910u &&
              runtime.words[later + 4u] == 0x10u &&
              runtime.words[later + 8u] == 0x6100u &&
              runtime.words[earlier + 4u] == 0x5203u &&
              runtime.words[earlier + 8u] == 0x6200u,
          "combat action registration creates script event and orders modes");

    Runtime failed;
    Initialize(failed);
    failed.fail_on_allocation = 1u;
    Check(pal4::CombatActionConsoleWindowEventRouter::Register(
              tree, 1u, 113u, 1u, 2u, 3u, nil, failed) == 0u &&
              failed.words[tree + 12u] == 0u,
          "combat action registration preserves empty tree on group failure");
}

void ScriptEventCreationAndCleanup() {
    std::uint32_t callback_slot = 0;
    Check(pal4::ScriptEventPayload::Create(
              0xfffffff0u, 0x4567u, 0x20u, callback_slot) == 0x10u &&
          callback_slot == 0x4567u,
          "script event stores callback and wraps the 32-bit owner sum");

    Runtime runtime;
    constexpr auto slot = 0x4000u;
    Check(pal4::ScriptEventPayload::CleanupSlot(slot, runtime) == 0u &&
          runtime.deleted.empty() && runtime.words[slot] == 0u,
          "empty script event slot is cleared without a virtual delete");
    runtime.words[slot] = 0x9876u;
    Check(pal4::ScriptEventPayload::CleanupSlot(slot, runtime) == 0x1234u &&
          runtime.words[slot] == 0u &&
          runtime.deleted == std::vector<std::uint32_t>{0x9876u} &&
          runtime.delete_flags == std::vector<std::uint32_t>{1u},
          "owned callback uses deleting destructor flag one");
}

void CombatMessageRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatMessageEventRouter::Register(
        tree, 7u, 0xa1u, 0xfffffff0u, 0x1100u, 0x20u, nil, runtime);
    const auto earlier = pal4::CombatMessageEventRouter::Register(
        tree, 2u, 0xa1u, 0x2200u, 0x3300u, 3u, nil, runtime);
    const auto other = pal4::CombatMessageEventRouter::Register(
        tree, 4u, 0xb2u, 0x4400u, 0x5500u, 5u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0xa1u, nil, runtime) + 16u;
    const auto list_head = runtime.words[group + 4u];
    const auto first = runtime.words[list_head];
    const auto second = runtime.words[first];
    Check(later && earlier && other &&
          runtime.words[tree + 12u] == 2u &&
          runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 7u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x10u &&
          runtime.words[later + 8u] == 0x1100u &&
          runtime.words[earlier + 4u] == 0x2203u &&
          runtime.words[other + 8u] == 0x5500u &&
          runtime.bytes[first + 8u] == 1u,
          "combat message registration shares groups and transfers callback ownership");

    Runtime failed;
    Initialize(failed);
    failed.fail_on_allocation = 1u;
    Check(pal4::CombatMessageEventRouter::Register(
              tree, 1u, 0xa1u, 1u, 2u, 3u, nil, failed) == 0u &&
          failed.words[tree + 12u] == 0u,
          "combat message registration leaves tree empty on group failure");
    Runtime callback_failed;
    Initialize(callback_failed);
    callback_failed.fail_on_allocation = 3u;
    Check(pal4::CombatMessageEventRouter::Register(
              tree, 1u, 0xa1u, 1u, 2u, 3u, nil, callback_failed) == 0u &&
          callback_failed.words[tree + 12u] == 1u,
          "callback allocation failure still inserts an unowned mode node");
    const auto failed_group = pal4::SystemEventRouterTree::FindExact(
        tree, 0xa1u, nil, callback_failed) + 16u;
    const auto failed_head = callback_failed.words[failed_group + 4u];
    const auto failed_node = callback_failed.words[failed_head];
    Check(callback_failed.words[failed_group + 8u] == 1u &&
          callback_failed.words[failed_node + 12u] == 0u &&
          callback_failed.bytes[failed_node + 8u] == 0u,
          "failed callback retains native empty payload layout");
}

void CombatMessagePlainRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatMessageEventRouter::RegisterPlain(
        tree, 3u, 0x66u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::CombatMessageEventRouter::RegisterPlain(
        tree, 1u, 0x66u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0x66u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    Check(later && earlier &&
          runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 1u &&
          runtime.words[runtime.words[first] + 12u] == later &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "plain combat-message registration stores owner and callback directly");
}

void CombatRoleStateRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatRoleStateEventRouter::Register(
        tree, 4u, 0x77u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::CombatRoleStateEventRouter::Register(
        tree, 2u, 0x77u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0x77u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    const auto second = runtime.words[first];
    Check(later && earlier &&
          runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 4u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "role-state registration preserves callback fields and mode order");

    Runtime failed;
    Initialize(failed);
    failed.fail_on_allocation = 1u;
    Check(pal4::CombatRoleStateEventRouter::Register(
              tree, 1u, 0x77u, 1u, 2u, nil, failed) == 0u &&
          failed.words[tree + 12u] == 0u,
          "role-state registration preserves tree on group allocation failure");
}

void CombatEventOutputRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatEventOutputEventRouter::Register(
        tree, 4u, 0x88u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::CombatEventOutputEventRouter::Register(
        tree, 2u, 0x88u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0x88u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    Check(later && earlier &&
          runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "event-output registration shares mode-ordered router layout");
}

void CombatProcessMessageRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::CombatProcessMessageEventRouter::Register(
        tree, 4u, 0x99u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::CombatProcessMessageEventRouter::Register(
        tree, 2u, 0x99u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0x99u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    const auto second = runtime.words[first];
    Check(later && earlier && runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 4u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "process-message registration uses mode-ordered callback list");
}

void MagicActionRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::MagicActionEventRouter::Register(
        tree, 4u, 0xa1u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::MagicActionEventRouter::Register(
        tree, 2u, 0xa1u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0xa1u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    const auto second = runtime.words[first];
    Check(later && earlier && runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 4u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "MagicAction registration uses shared mode-ordered event tree");
}

void FinalHitRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::FinalHitEventRouter::Register(
        tree, 4u, 0xa1u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::FinalHitEventRouter::Register(
        tree, 2u, 0xa1u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0xa1u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    const auto second = runtime.words[first];
    Check(later && earlier && runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 4u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "FinalHit registration uses shared mode-ordered event tree");
}

void StuntActionRegistration() {
    Runtime runtime;
    Initialize(runtime);
    const auto later = pal4::StuntActionEventRouter::Register(
        tree, 4u, 0xa1u, 0x1111u, 0x2222u, nil, runtime);
    const auto earlier = pal4::StuntActionEventRouter::Register(
        tree, 2u, 0xa1u, 0x3333u, 0x4444u, nil, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        tree, 0xa1u, nil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    const auto first = runtime.words[head];
    const auto second = runtime.words[first];
    Check(later && earlier && runtime.words[group + 8u] == 2u &&
          runtime.words[first + 12u] == earlier &&
          runtime.words[first + 16u] == 2u &&
          runtime.words[second + 12u] == later &&
          runtime.words[second + 16u] == 4u &&
          runtime.words[later] == 0x844910u &&
          runtime.words[later + 4u] == 0x1111u &&
          runtime.words[later + 8u] == 0x2222u,
          "StuntAction registration uses shared mode-ordered event tree");
}

void PropertyActionRegistrations() {
    using Register = std::uint32_t (*)(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t,
        pal4::ISystemEventRouterTreeRuntime&);
    constexpr std::array<Register, 6> registrations{{
        &pal4::PropertyActionEventRouter::Register,
        &pal4::ThrowPropertyActionEventRouter::Register,
        &pal4::CastPropertyActionEventRouter::Register,
        &pal4::CombatEndSoundEventRouter::Register,
        &pal4::CombatActorEventRouter::Register,
        &pal4::CombatExtendedEventRouter::Register,
    }};
    for (const auto register_event : registrations) {
        Runtime runtime;
        Initialize(runtime);
        const auto later = register_event(
            tree, 4u, 0xa1u, 0x1111u, 0x2222u, nil, runtime);
        const auto earlier = register_event(
            tree, 2u, 0xa1u, 0x3333u, 0x4444u, nil, runtime);
        const auto group = pal4::SystemEventRouterTree::FindExact(
            tree, 0xa1u, nil, runtime) + 16u;
        const auto head = runtime.words[group + 4u];
        const auto first = runtime.words[head];
        const auto second = runtime.words[first];
        Check(later && earlier && runtime.words[group + 8u] == 2u &&
              runtime.words[first + 12u] == earlier &&
              runtime.words[first + 16u] == 2u &&
              runtime.words[second + 12u] == later &&
              runtime.words[second + 16u] == 4u &&
              runtime.words[later] == 0x844910u &&
              runtime.words[later + 4u] == 0x1111u &&
              runtime.words[later + 8u] == 0x2222u,
              "property action routers share mode-ordered event tree");
    }
}

}  // namespace

int main() {
    try {
        IntegerTreeInsertion();
        AllocationFailures();
        ListCleanupVariants();
        DirectHelpersAndDuplicateMode();
        RangeRemoval();
        PredecessorAndExactLookup();
        RegistrationAndCopies();
        RegistrationAllocationFailures();
        ActorManagerRegistration();
        WorldManagerRegistration();
        CombatConsoleRegistration();
        CombatActionWindowRegistration();
        ScriptEventCreationAndCleanup();
        CombatMessageRegistration();
        CombatMessagePlainRegistration();
        CombatRoleStateRegistration();
        CombatEventOutputRegistration();
        CombatProcessMessageRegistration();
        MagicActionRegistration();
        FinalHitRegistration();
        StuntActionRegistration();
        PropertyActionRegistrations();
        ScriptRouterRegistration();
        std::cout << "system_event_router_tree_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
