#include "pal4/pal_game_iv_state_registry.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVStateRegistryRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> ids;
    std::vector<std::uint32_t> deleted;
    std::vector<std::uint32_t> freed;
    std::uint32_t next = 0x5000u;
    std::uint32_t allocations = 0u;
    std::uint32_t id_calls = 0u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "node allocation size");
        ++allocations;
        if (fail_allocation) return 0u;
        const auto node = next;
        next += 0x10u;
        return node;
    }
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return address + 0x100u;
    }
    std::uint32_t StateId(std::uint32_t state) override {
        ++id_calls;
        return ids.at(state);
    }
    void DeleteState(std::uint32_t state,
                     std::uint32_t flags) override {
        Check(flags == 1u, "deleting destructor flag");
        deleted.push_back(state);
    }
    std::uint32_t EmptyCleanupResult() override { return 0xdeadu; }
};

void AddLookupAndCleanup() {
    Runtime runtime;
    runtime.ids = {{0x1000u, 1u}, {0x2000u, 2u},
                   {0x3000u, 3u}, {0x4000u, 2u}};
    constexpr auto game = 0x8000u;
    Check(!pal4::PalGameIVStateRegistry::ContainsEntry(
              game, 0x1000u, runtime), "empty contains");
    Check(pal4::PalGameIVStateRegistry::FindEntryById(
              game, 1u, runtime) == 0u, "empty find");
    for (auto entry : {0x1000u, 0x2000u, 0x3000u})
        Check(pal4::PalGameIVStateRegistry::AddEntry(
                  game, entry, runtime), "unique add");
    Check(runtime.allocations == 3u &&
              runtime.ReadWord(game + 0x188u) == 0x5000u &&
              runtime.ReadWord(0x5000u) == 0x1000u &&
              runtime.ReadWord(0x5004u) == 0x5010u &&
              runtime.ReadWord(0x5014u) == 0x5020u &&
              runtime.ReadWord(0x5024u) == 0u, "linked insertion order");
    Check(!pal4::PalGameIVStateRegistry::AddEntry(
              game, 0x4000u, runtime) && runtime.allocations == 3u,
          "duplicate virtual id rejected");
    Check(pal4::PalGameIVStateRegistry::ContainsEntry(
              game, 0x4000u, runtime), "contains by id");
    Check(runtime.id_calls >= 10u, "virtual id comparisons");
    Check(pal4::PalGameIVStateRegistry::FindEntryById(
              game, 2u, runtime) == 0x2000u &&
              pal4::PalGameIVStateRegistry::FindEntryById(
                  game, 4u, runtime) == 0u, "id lookup");
    Check(pal4::PalGameIVStateRegistry::Cleanup(game, runtime) ==
              0x5120u, "last free result");
    Check(runtime.deleted ==
              std::vector<std::uint32_t>({0x1000u, 0x2000u, 0x3000u}) &&
              runtime.freed ==
              std::vector<std::uint32_t>({0x5000u, 0x5010u, 0x5020u}) &&
              runtime.ReadWord(game + 0x188u) == 0u,
          "ordered deleting cleanup");
    Check(pal4::PalGameIVStateRegistry::Cleanup(game, runtime) ==
              0xdeadu, "empty cleanup result boundary");
}

void AllocationFailureAndNullState() {
    Runtime runtime;
    constexpr auto game = 0x8000u;
    runtime.fail_allocation = true;
    try {
        (void)pal4::PalGameIVStateRegistry::AddEntry(
            game, 0x1000u, runtime);
        Check(false, "allocation failure must throw");
    } catch (const std::bad_alloc&) {
        Check(runtime.ReadWord(game + 0x188u) == 0u,
              "failed add preserves list");
    }
    runtime.WriteWord(game + 0x188u, 0x5000u);
    runtime.WriteWord(0x5000u, 0u);
    runtime.WriteWord(0x5004u, 0u);
    Check(pal4::PalGameIVStateRegistry::Cleanup(game, runtime) == 0x5100u &&
              runtime.deleted.empty() && runtime.freed.size() == 1u,
          "null state skips destructor");
}

}  // namespace

int main() {
    try {
        AddLookupAndCleanup();
        AllocationFailureAndNullState();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
