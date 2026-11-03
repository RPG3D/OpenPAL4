#include "pal4/game_data_race_registry.h"

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

struct Runtime final : pal4::IGameDataPropertyCollectionRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> sources;
    std::vector<pal4::GameDataProperty> appended;
    std::vector<pal4::GameDataProperty> before_sort;
    std::uint8_t first_seed = 0;
    std::uint8_t second_seed = 0;

    void InitializeStorage(std::uint8_t first,
                           std::uint8_t second) override {
        first_seed = first;
        second_seed = second;
        calls.emplace_back("storage");
    }
    void RegisterCleanup() override { calls.emplace_back("register cleanup"); }
    void CleanupStorage() override { calls.emplace_back("cleanup"); }
    void ResetTemporaryName(std::uint8_t seed) override {
        Check(seed == second_seed, "temporary name receives second seed");
        calls.emplace_back("reset name");
    }
    void ReleaseTemporaryName() override { calls.emplace_back("release name"); }
    std::string SourceName(std::uint32_t address) override {
        sources.push_back(address);
        return std::to_string(address);
    }
    std::string ErrorSuffix() override { return "!"; }
    void AppendRecord(const pal4::GameDataProperty& record) override {
        appended.push_back(record);
        calls.emplace_back("append");
    }
    void Sort(std::vector<pal4::GameDataProperty>& records) override {
        before_sort = records;
        calls.emplace_back("sort");
    }
};

void CheckInitializationAndCleanup() {
    pal4::GameDataRaceRegistry registry;
    Runtime runtime;
    const auto address = registry.Initialize(0x12, 0x34, runtime);
    Check(address == 0x8ee7e4 && registry.StorageInitialized() &&
              registry.RecordsInitialized() && runtime.first_seed == 0x12 &&
              runtime.second_seed == 0x34 && registry.Records().size() == 36 &&
              runtime.appended == registry.Records() &&
              runtime.before_sort == registry.Records(),
          "storage, registration, append and sort cover 36 entries");
    Check(runtime.calls.size() == 41 && runtime.calls[0] == "storage" &&
              runtime.calls[1] == "register cleanup" &&
              runtime.calls[2] == "reset name" &&
              runtime.calls[39] == "sort" &&
              runtime.calls[40] == "release name",
          "initialization and temporary-name call order");
    const auto& records = registry.Records();
    Check(records[0].id == 0 && records[0].source_address == 0x8ef7a8 &&
              records[0].kind == pal4::GameDataPropertyKind::race &&
              records[0].name == "RACE!" && records[0].source_count == 7,
          "RACE descriptor has 7 source values");

    struct Group {
        std::uint32_t first_id;
        std::uint32_t count;
        std::uint32_t source;
        std::uint32_t stride;
        pal4::GameDataPropertyKind kind;
        const char* suffix;
    };
    constexpr std::array<Group, 6> groups{{
        {4, 8, 0x8ee4f8, 16, pal4::GameDataPropertyKind::numeric, "!"},
        {36, 4, 0x8ee4b8, 16, pal4::GameDataPropertyKind::text, "!"},
        {52, 6, 0x8ee458, 16, pal4::GameDataPropertyKind::numeric,
         "_ADDITIONAL"},
        {76, 6, 0x8ee458, 16, pal4::GameDataPropertyKind::text, "_REACT"},
        {100, 6, 0x8ee458, 16, pal4::GameDataPropertyKind::text, "_EXTRACT"},
        {124, 5, 0x8efa08, 16, pal4::GameDataPropertyKind::numeric, "!"},
    }};
    std::size_t position = 1;
    for (const auto& group : groups) {
        for (std::uint32_t index = 0; index < group.count;
             ++index, ++position) {
            const auto& record = records[position];
            const auto source = group.source + index * group.stride;
            Check(record.id == group.first_id + index * 4 &&
                      record.source_address == source &&
                      record.kind == group.kind &&
                      record.name == std::to_string(source) + group.suffix &&
                      record.source_count == 0 &&
                      runtime.sources[position - 1] == source,
                  "group ID, source stride, kind and suffix");
        }
    }
    Check(position == records.size() && runtime.sources.size() == 35,
          "all six groups consume exactly 35 source buffers");

    Check(registry.Initialize(0xff, 0x34, runtime) == address &&
              runtime.calls.size() == 43 &&
              runtime.calls[41] == "reset name" &&
              runtime.calls[42] == "release name" &&
              registry.Records().size() == 36,
          "repeated initialization only resets and releases temporary name");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" &&
              registry.Records().empty() && registry.RecordsInitialized(),
          "CRT cleanup frees collection without clearing the one-time guard");
}

}  // namespace

int main() {
    try {
        CheckInitializationAndCleanup();
        std::cout << "game_data_race_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
