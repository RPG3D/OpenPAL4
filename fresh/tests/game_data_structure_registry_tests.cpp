#include "pal4/game_data_structure_registry.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataStructureRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> sources;
    std::vector<pal4::GameDataProperty> before_sort;
    std::string error_suffix = "!";

    void InitializeGlobalStorage() override { calls.emplace_back("globals"); }
    void RegisterGlobalCleanup() override { calls.emplace_back("cleanup"); }
    void CleanupGlobalStorage() override { calls.emplace_back("cleanup storage"); }
    void SeedCollection(std::vector<pal4::GameDataProperty>& entries) override {
        calls.emplace_back("seed");
        entries.push_back({1, 0, pal4::GameDataPropertyKind::numeric, "seed", 0});
    }
    std::string SourceName(std::uint32_t address) override {
        sources.push_back(address);
        return std::to_string(address);
    }
    std::string ErrorSuffix() override { return error_suffix; }
    void Sort(std::vector<pal4::GameDataProperty>& entries) override {
        calls.emplace_back("sort");
        before_sort = entries;
    }
};

void CheckRegistrationOrder() {
    Runtime runtime;
    pal4::GameDataStructureRegistry registry;
    const auto& entries = registry.Initialize(runtime);
    Check(registry.GlobalsInitialized() && registry.EntriesInitialized(),
          "both one-time guards are set");
    Check(runtime.calls == std::vector<std::string>{"globals", "cleanup",
                                                     "seed", "sort"},
          "global setup, seed and sort order");
    Check(&entries == &registry.Entries() && entries.size() == 49 &&
              runtime.sources.size() == 46,
          "seed plus 48 entries, including two special entries");
    struct Group {
        std::uint32_t first;
        std::uint32_t last;
        std::uint32_t source;
        pal4::GameDataPropertyKind kind;
        const char* suffix;
    };
    constexpr std::array<Group, 7> groups{{
        {196, 200, 0x8ee348, pal4::GameDataPropertyKind::text, "!"},
        {168, 192, 0x8ee278, pal4::GameDataPropertyKind::text, "_IMMUNITY"},
        {144, 164, 0x8ee2e8, pal4::GameDataPropertyKind::text, "_IMMUNITY"},
        {204, 240, 0x8ee3b8, pal4::GameDataPropertyKind::numeric, "!"},
        {244, 260, 0x8ee368, pal4::GameDataPropertyKind::text, "!"},
        {264, 288, 0x8ef7a8, pal4::GameDataPropertyKind::text, "_RACE_ATK"},
        {292, 324, 0x8ee018, pal4::GameDataPropertyKind::numeric, "!"},
    }};
    std::size_t index = 1;
    for (const Group& group : groups) {
        for (std::uint32_t id = group.first, address = group.source;
             id <= group.last; id += 4, address += 16) {
            const auto& entry = entries[index++];
            Check(entry.id == id && entry.source_address == address &&
                      entry.kind == group.kind &&
                      entry.name == std::to_string(address) + group.suffix &&
                      entry.source_count == 0,
                  "group entry has exact ID, 16-byte source stride and suffix");
        }
    }
    Check(index == 47 && entries[index].id == 328 &&
              entries[index].source_address == 0x8ef688 &&
              entries[index].kind == pal4::GameDataPropertyKind::combat_position &&
              entries[index].source_count == 6 &&
              entries[index].name == "COMBAT_POSITION!",
          "combat-position special record");
    Check(entries[index + 1].id == 332 &&
              entries[index + 1].kind == pal4::GameDataPropertyKind::numeric &&
              entries[index + 1].name == "AI_TYPE!" &&
              runtime.before_sort == entries,
          "AI type is last before sorting");
    const auto& again = registry.Initialize(runtime);
    Check(&again == &entries && runtime.calls.size() == 4 &&
              runtime.sources.size() == 46,
          "repeated initialization does not rebuild or re-register");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup storage" &&
              registry.Entries().empty() && registry.EntriesInitialized(),
          "CRT callback cleans collection but leaves one-time guard set");
}

}  // namespace

int main() {
    try {
        CheckRegistrationOrder();
        std::cout << "game_data_structure_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
