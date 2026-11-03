#include "pal4/actor_manager_actor_pool.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t pool = 0x1000u;
constexpr std::uint32_t begin = 0x2000u;

struct Runtime final : pal4::IActorManagerActorPoolRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {pool + 4u, begin}, {pool + 8u, begin},
        {pool + 12u, begin + 32u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::string> trace;
    bool inactive = false;
    bool init_result = true;
    bool allocation_failure = false;

    bool CombatInactive() override {
        trace.emplace_back("combat");
        return inactive;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("flag:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
    std::string ActorName(std::uint32_t actor) override {
        trace.emplace_back("name:" + std::to_string(actor));
        return names.at(actor);
    }
    std::uint32_t Allocate(std::uint32_t bytes_count) override {
        if (bytes_count == 0x610u) {
            trace.emplace_back("allocate");
            return allocation_failure ? 0u : 0x9000u;
        }
        Check(bytes_count != 0u && bytes_count % 8u == 0u,
              "array allocation size");
        trace.emplace_back("allocate-array");
        return allocation_failure ? 0u : 0xe000u;
    }
    std::uint32_t ConstructActor(std::uint32_t storage) override {
        Check(storage == 0x9000u, "actor constructor storage");
        trace.emplace_back("construct");
        return 0xa000u;
    }
    bool InitializeActor(std::uint32_t actor, std::string_view first,
                         std::string_view second) override {
        Check(actor == 0xa000u && first == "new_actor" &&
                  second == first, "slot +0x30 receives same name twice");
        trace.emplace_back("initialize");
        return init_result;
    }
    void DeleteActor(std::uint32_t actor, std::uint32_t flags) override {
        Check(actor != 0u && flags == 1u,
              "owned actor uses deleting destructor");
        trace.emplace_back("delete");
    }
    void DeactivateActor(std::uint32_t actor,
                         std::uint32_t context) override {
        Check(actor == 0xb000u && context == 0xc000u,
              "actor deactivation context");
        trace.emplace_back("deactivate");
    }
    std::uint32_t CreateTemporarySlot(std::uint32_t actor) override {
        Check(actor == 0xb000u, "temporary actor address");
        bytes[0xd000u] = 1u;
        words[0xd004u] = actor;
        trace.emplace_back("temporary");
        return 0xd000u;
    }
};

void AddSlot(Runtime& runtime, std::uint32_t index,
             std::uint32_t actor, std::string name) {
    const auto slot = begin + index * 8u;
    runtime.bytes[slot] = 1u;
    runtime.words[slot + 4u] = actor;
    runtime.names[actor] = std::move(name);
    runtime.words[pool + 8u] = slot + 8u;
}

void InactiveAndEmpty() {
    Runtime runtime;
    runtime.inactive = true;
    Check(pal4::ActorManagerActorPool::TakeOrCreate(
              pool, "new_actor", runtime) == 0u &&
              runtime.trace == std::vector<std::string>{"combat"},
          "inactive combat exits before pool access");
    runtime.inactive = false;
    runtime.trace.clear();
    Check(pal4::ActorManagerActorPool::TakeOrCreate(
              pool, "new_actor", runtime) == 0xa000u &&
              runtime.trace == std::vector<std::string>{
                  "combat", "allocate", "construct", "initialize"},
          "empty pool creates and initializes actor");
}

void ReuseAndCompact() {
    for (std::uint32_t selected = 0u; selected < 3u; ++selected) {
        Runtime runtime;
        AddSlot(runtime, 0u, 0x3000u, "first");
        AddSlot(runtime, 1u, 0x4000u, "middle");
        AddSlot(runtime, 2u, 0x5000u, "last");
        const char* selected_name =
            selected == 0u ? "first" : selected == 1u ? "middle" : "last";
        const auto selected_actor = 0x3000u + selected * 0x1000u;
        Check(pal4::ActorManagerActorPool::TakeOrCreate(
                  pool, selected_name, runtime) == selected_actor,
              "matching actor returned");
        Check(runtime.words[pool + 8u] == begin + 16u &&
                  runtime.bytes[begin] == 1u &&
                  runtime.bytes[begin + 8u] == 1u &&
                  runtime.bytes[begin + 16u] == 0u,
              "selected slot transfers ownership and shortens vector");
        Check(runtime.trace.front() == "combat" &&
                  runtime.trace.back() == "write:4104" &&
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "delete") == runtime.trace.end(),
              "compaction preserves other actors without deleting them");
        const auto first_expected = selected == 0u ? 0x4000u : 0x3000u;
        const auto second_expected = selected == 2u ? 0x4000u : 0x5000u;
        Check(runtime.words[begin + 4u] == first_expected &&
                  runtime.words[begin + 12u] == second_expected,
              "remaining actors retain order");
    }
}

void MissAndFailure() {
    Runtime runtime;
    AddSlot(runtime, 0u, 0x3000u, "existing");
    Check(pal4::ActorManagerActorPool::TakeOrCreate(
              pool, "new_actor", runtime) == 0xa000u &&
              runtime.words[pool + 8u] == begin + 8u,
          "name miss creates without changing pool");
    runtime.trace.clear();
    runtime.init_result = false;
    Check(pal4::ActorManagerActorPool::TakeOrCreate(
              pool, "new_actor", runtime) == 0u &&
              runtime.trace == std::vector<std::string>{
                  "combat", "name:12288", "allocate", "construct",
                  "initialize", "delete"},
          "rejected initialization destroys new actor");
    runtime.trace.clear();
    runtime.allocation_failure = true;
    bool caught = false;
    try {
        (void)pal4::ActorManagerActorPool::TakeOrCreate(
            pool, "new_actor", runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.trace == std::vector<std::string>{
              "combat", "name:12288", "allocate"},
          "allocation failure does not dereference null storage");
}

void ReturnBranches() {
    Runtime runtime;
    pal4::ActorManagerActorPool::Return(
        pool, 2u, 0u, 0xc000u, runtime);
    Check(runtime.trace.empty(), "null actor is ignored");

    pal4::ActorManagerActorPool::Return(
        pool, 2u, 0xb000u, 0xc000u, runtime);
    Check(runtime.trace.front() == "deactivate" &&
              runtime.trace[1] == "temporary" &&
              runtime.words[pool + 8u] == begin + 8u &&
              runtime.words[begin + 4u] == 0xb000u &&
              runtime.bytes[begin] == 1u &&
              runtime.bytes[0xd000u] == 0u,
          "below capacity actor is appended after deactivation");

    Runtime full;
    AddSlot(full, 0u, 0x3000u, "first");
    AddSlot(full, 1u, 0x4000u, "second");
    pal4::ActorManagerActorPool::Return(
        pool, 2u, 0xb000u, 0xc000u, full);
    Check(full.trace.front() == "deactivate" &&
              full.trace[1] == "delete" &&
              full.words[pool + 8u] == begin + 16u &&
              full.words[begin + 4u] == 0x4000u &&
              full.words[begin + 12u] == 0xb000u &&
              full.bytes[0xd000u] == 0u,
          "full pool drops oldest slot before appending new actor");

    Runtime grown;
    AddSlot(grown, 0u, 0x3000u, "old");
    grown.words[pool + 12u] = grown.words[pool + 8u];
    pal4::ActorManagerActorPool::Return(
        pool, 2u, 0xb000u, 0xc000u, grown);
    Check(grown.words[pool + 4u] == 0xe000u &&
              grown.words[pool + 8u] == 0xe010u &&
              grown.words[0xe004u] == 0x3000u &&
              grown.words[0xe00cu] == 0xb000u &&
              grown.bytes[0xd000u] == 0u &&
              std::find(grown.trace.begin(), grown.trace.end(),
                        "free:8192") != grown.trace.end(),
          "full storage reallocates and transfers temporary ownership");

    Runtime failed;
    AddSlot(failed, 0u, 0x3000u, "old");
    failed.words[pool + 12u] = failed.words[pool + 8u];
    failed.allocation_failure = true;
    bool caught = false;
    try {
        pal4::ActorManagerActorPool::Return(
            pool, 2u, 0xb000u, 0xc000u, failed);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed.trace[failed.trace.size() - 2u] ==
              "allocate-array" && failed.trace.back() == "delete" &&
              failed.words[pool + 8u] == begin + 8u,
          "insertion failure releases still-owned temporary actor");
}

}  // namespace

int main() {
    try {
        InactiveAndEmpty();
        ReuseAndCompact();
        MissAndFailure();
        ReturnBranches();
        std::cout << "actor_manager_actor_pool_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
