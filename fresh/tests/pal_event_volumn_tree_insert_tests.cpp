#include "pal4/pal_event_volumn_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalEventVolumnTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::uint32_t> reads;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t, std::uint32_t>> inserts;

    std::uint32_t SharedNil() override { return 0x9000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return memory.at(address);
    }
    void InsertAt(pal4::PalEventVolumnState&, std::uint32_t child,
                  std::uint32_t parent, std::uint32_t key,
                  std::uint32_t value) override {
        inserts.emplace_back(child, parent, key, value);
    }
};

struct CleanupRuntime final : pal4::IPalEventVolumnTreeCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> events;
    std::uint32_t SharedNil() override { return 0x9000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return memory.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        memory[address] = value;
    }
    void FreeNode(std::uint32_t node) override {
        events.push_back("free-node:" + std::to_string(node));
    }
    void EraseNode(std::uint32_t node) override {
        events.push_back("erase:" + std::to_string(node));
    }
    void ObserveFirstElement(std::uint32_t head) override {
        events.push_back("first:" + std::to_string(head));
    }
    int Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 41;
    }
};

void ResetCalls(Runtime& runtime) {
    runtime.reads.clear();
    runtime.inserts.clear();
}

}  // namespace

int main() {
    pal4::PalEventVolumnState state;
    state.words[10u] = 0x1000u;
    Runtime runtime;
    runtime.memory[0x1000u] = 0x1000u;
    runtime.memory[0x1004u] = 0x9000u;
    Check(pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 7u, 0xaau, runtime) &&
              runtime.inserts == std::vector<std::tuple<
                  std::uint32_t, std::uint32_t, std::uint32_t,
                  std::uint32_t>>{{0x9000u, 0x1000u, 7u, 0xaau}},
          "empty unique tree inserts under its head");

    ResetCalls(runtime);
    state.words[11u] = 1u;
    runtime.memory[0x1004u] = 0x2000u;
    runtime.memory[0x200cu] = 7u;
    runtime.memory[0x2000u] = 0x9000u;
    runtime.memory[0x2008u] = 0x9000u;
    Check(pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 7u, 0xbbu, runtime) &&
              runtime.inserts == std::vector<std::tuple<
                  std::uint32_t, std::uint32_t, std::uint32_t,
                  std::uint32_t>>{{0x9000u, 0x2000u, 7u, 0xbbu}},
          "duplicate mode inserts an equal key");

    ResetCalls(runtime);
    state.words[11u] = 0u;
    runtime.memory[0x1000u] = 0x2000u;
    Check(!pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 7u, 0xccu, runtime) && runtime.inserts.empty(),
          "unique mode rejects equal key");

    ResetCalls(runtime);
    Check(pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 8u, 0xddu, runtime) &&
              runtime.inserts.front() == std::tuple{
                  0x9000u, 0x2000u, 8u, 0xddu},
          "unique mode inserts a larger key on the right");

    ResetCalls(runtime);
    Check(pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 6u, 0xeeu, runtime) &&
              runtime.inserts.front() == std::tuple{
                  0x9000u, 0x2000u, 6u, 0xeeu},
          "new minimum inserts without predecessor lookup");

    ResetCalls(runtime);
    runtime.memory[0x1000u] = 0x3000u;
    runtime.memory[0x1004u] = 0x2000u;
    runtime.memory[0x2000u] = 0x3000u;
    runtime.memory[0x2004u] = 0x1000u;
    runtime.memory[0x2008u] = 0x9000u;
    runtime.memory[0x3004u] = 0x2000u;
    runtime.memory[0x300cu] = 4u;
    runtime.memory[0x3000u] = 0x9000u;
    runtime.memory[0x3008u] = 0x4000u;
    runtime.memory[0x400cu] = 6u;
    runtime.memory[0x4000u] = 0x9000u;
    runtime.memory[0x4004u] = 0x3000u;
    runtime.memory[0x4008u] = 0x9000u;
    runtime.memory[0x4014u] = 1u;
    Check(pal4::PalEventVolumnManager::PredecessorTreeNode(
              0x4000u, runtime) == 0x3000u,
          "predecessor climbs from a left-empty node");
    Check(pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 5u, 0xffu, runtime) &&
              runtime.inserts.front() == std::tuple{
                  0x9000u, 0x4000u, 5u, 0xffu},
          "predecessor permits key between existing nodes");

    ResetCalls(runtime);
    Check(!pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 4u, 0u, runtime) && runtime.inserts.empty(),
          "existing left node rejected at equal key");

    ResetCalls(runtime);
    runtime.memory[0x200cu] = 0x80000000u;
    runtime.memory[0x2000u] = 0x9000u;
    runtime.memory[0x1000u] = 0x2000u;
    Check(pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 0x7fffffffu, 1u, runtime) &&
              runtime.inserts.front() == std::tuple{
                  0x9000u, 0x2000u, 0x7fffffffu, 1u},
          "comparison treats keys as signed 32-bit integers");

    runtime.memory[0x1004u] = 0x9000u;
    Check(pal4::PalEventVolumnManager::FindTreeValue(
              state, 5u, runtime) == 0u,
          "empty tree has no matching value");

    runtime.memory[0x1004u] = 0x2000u;
    runtime.memory[0x200cu] = 0u;
    runtime.memory[0x2010u] = 0xaau;
    runtime.memory[0x2000u] = 0x3000u;
    runtime.memory[0x2008u] = 0x4000u;
    runtime.memory[0x300cu] = 0x80000000u;
    runtime.memory[0x3010u] = 0xbbu;
    runtime.memory[0x3000u] = 0x9000u;
    runtime.memory[0x3008u] = 0x9000u;
    runtime.memory[0x400cu] = 0x7fffffffu;
    runtime.memory[0x4010u] = 0xccu;
    runtime.memory[0x4000u] = 0x9000u;
    runtime.memory[0x4008u] = 0x9000u;
    runtime.memory[0x2014u] = 1u;
    runtime.memory[0x3014u] = 1u;
    runtime.memory[0x1014u] = 0u;
    runtime.memory[0x2004u] = 0x1000u;
    runtime.memory[0x1008u] = 0x4000u;
    Check(pal4::PalEventVolumnManager::PredecessorTreeNode(
              0x2000u, runtime) == 0x3000u &&
              pal4::PalEventVolumnManager::PredecessorTreeNode(
                  0x1000u, runtime) == 0x4000u,
          "predecessor uses left subtree and head maximum");
    Check(pal4::PalEventVolumnManager::LowerBoundTreeNode(
              state, 0x80000000u, runtime) == 0x3000u &&
              pal4::PalEventVolumnManager::LowerBoundTreeNode(
                  state, 0xffffffffu, runtime) == 0x2000u &&
              pal4::PalEventVolumnManager::LowerBoundTreeNode(
                  state, 1u, runtime) == 0x4000u &&
              pal4::PalEventVolumnManager::LowerBoundTreeNode(
                  state, 0x7fffffffu, runtime) == 0x4000u,
          "signed lower-bound returns first key at or above request");
    Check(pal4::PalEventVolumnManager::FindTreeValue(
              state, 0x80000000u, runtime) == 0xbbu &&
              pal4::PalEventVolumnManager::FindTreeValue(
                  state, 0u, runtime) == 0xaau &&
              pal4::PalEventVolumnManager::FindTreeValue(
                  state, 0x7fffffffu, runtime) == 0xccu,
          "exact signed minimum, zero and maximum keys return values");
    Check(pal4::PalEventVolumnManager::FindTreeValue(
              state, 0xffffffffu, runtime) == 0u &&
              pal4::PalEventVolumnManager::FindTreeValue(
                  state, 1u, runtime) == 0u,
          "missing keys below and between nodes return zero");

    CleanupRuntime cleanup;
    pal4::PalEventVolumnSharedTree shared{0x9000u, 2u};
    pal4::PalEventVolumnState populated;
    populated.words[10u] = 0x1000u;
    populated.words[12u] = 3u;
    cleanup.memory[0x1000u] = 0x3000u;
    cleanup.memory[0x1004u] = 0x2000u;
    cleanup.memory[0x2000u] = 0x3000u;
    cleanup.memory[0x2008u] = 0x4000u;
    cleanup.memory[0x3000u] = 0x9000u;
    cleanup.memory[0x3008u] = 0x9000u;
    cleanup.memory[0x4000u] = 0x9000u;
    cleanup.memory[0x4008u] = 0x9000u;
    Check(pal4::PalEventVolumnManager::CleanupTree(
              populated, shared, cleanup) == 1 &&
              populated.words[8u] == 0x84b490u &&
              populated.words[10u] == 0u && populated.words[12u] == 0u &&
              shared.nil == 0x9000u && shared.references == 1u &&
              cleanup.memory[0x1000u] == 0x1000u &&
              cleanup.memory[0x1004u] == 0x9000u &&
              cleanup.memory[0x1008u] == 0x1000u &&
              cleanup.events == std::vector<std::string>{
                  "read:4096", "read:4100", "read:8200",
                  "read:16392", "read:16384", "free-node:16384",
                  "read:8192", "free-node:8192", "read:12296",
                  "read:12288", "free-node:12288",
                  "write:4100", "write:4096", "write:4104",
                  "first:4096", "free:4096"},
          "populated tree clears subtree and retains shared nil");

    cleanup.events.clear();
    pal4::PalEventVolumnState inconsistent;
    inconsistent.words[10u] = 0x1100u;
    cleanup.memory[0x1100u] = 0x5000u;
    cleanup.memory[0x1104u] = 0x5000u;
    cleanup.memory[0x1108u] = 0x6000u;
    cleanup.memory[0x5004u] = 0x1100u;
    cleanup.memory[0x5008u] = 0x6000u;
    cleanup.memory[0x6000u] = 0x9000u;
    cleanup.memory[0x6004u] = 0x5000u;
    cleanup.memory[0x6008u] = 0x9000u;
    Check(pal4::PalEventVolumnManager::CleanupTree(
              inconsistent, shared, cleanup) == 41 &&
              shared.references == 0u && shared.nil == 0u &&
              cleanup.events == std::vector<std::string>{
                  "read:4352", "read:20488", "read:24576",
                  "erase:20480", "read:24584", "read:24580",
                  "read:20488", "read:20484", "read:4360",
                  "read:20488", "erase:24576", "free:4352",
                  "free:36864"},
          "zero-count linked tree erases nodes then releases last nil");

    cleanup.events.clear();
    cleanup.memory[0x7008u] = 0x8000u;
    cleanup.memory[0x8000u] = 0x8100u;
    cleanup.memory[0x8100u] = 0x9000u;
    auto cursor = 0x7000u;
    Check(pal4::PalEventVolumnManager::AdvanceTreeIterator(
              cursor, cleanup) == 0x7000u && cursor == 0x8100u,
          "post-increment enters right subtree at its leftmost node");
    cursor = 0x7000u;
    pal4::PalEventVolumnManager::AdvanceTreeCursor(cursor, cleanup);
    Check(cursor == 0x8100u,
          "in-place increment enters right subtree at its leftmost node");

    cleanup.events.clear();
    pal4::PalEventVolumnState empty;
    pal4::PalEventVolumnSharedTree no_nil{0u, 1u};
    empty.words[10u] = 0x1200u;
    cleanup.memory[0x1200u] = 0x1200u;
    Check(pal4::PalEventVolumnManager::CleanupTree(
              empty, no_nil, cleanup) == 0 &&
              cleanup.events == std::vector<std::string>{
                  "read:4608", "free:4608"},
          "empty tree releases head without null nil free");
}
