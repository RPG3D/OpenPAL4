#include "pal4/game_data_upgrade_registry.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataUpgradeRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> sources;
    std::vector<pal4::GameDataProperty> appended;
    std::vector<pal4::GameDataProperty> before_sort;
    std::uint8_t first_seed = 0;
    std::uint8_t second_seed = 0;
    std::uint32_t processed_object = 0;
    std::uint32_t processed_collection = 0;
    std::uint32_t process_result = 0x12ab;

    void InitializeStorage(std::uint8_t first,
                           std::uint8_t second) override {
        first_seed = first;
        second_seed = second;
        calls.emplace_back("storage");
    }
    void RegisterCleanup() override { calls.emplace_back("register cleanup"); }
    void CleanupStorage() override { calls.emplace_back("cleanup"); }
    void ResetTemporaryName(std::uint8_t seed) override {
        Check(seed == second_seed, "temporary name uses the second seed");
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
    std::uint32_t ProcessCollection(
        std::uint32_t object, std::uint32_t collection) override {
        processed_object = object;
        processed_collection = collection;
        calls.emplace_back("process");
        return process_result;
    }
};

void CheckRegistrationAndProcessing() {
    Runtime runtime;
    pal4::GameDataUpgradeRegistry registry;
    Check(registry.Initialize(0x45, 0x67, runtime) == 0x8ee818 &&
              registry.StorageInitialized() && registry.RecordsInitialized() &&
              runtime.first_seed == 0x45 && runtime.second_seed == 0x67 &&
              registry.Records().size() == 23 &&
              runtime.appended == registry.Records() &&
              runtime.before_sort == registry.Records(),
          "upgrade registry builds its 23 properties once");
    Check(runtime.calls.size() == 28 && runtime.calls[0] == "storage" &&
              runtime.calls[1] == "register cleanup" &&
              runtime.calls[2] == "reset name" &&
              runtime.calls[26] == "sort" &&
              runtime.calls[27] == "release name",
          "storage, insertion, sort and temporary-name order");
    const auto& records = registry.Records();
    Check(records[0].id == 88 && records[0].source_address == 0 &&
              records[0].kind == pal4::GameDataPropertyKind::numeric &&
              records[0].name == "ID!",
          "ID is the leading numeric property at slot 88");
    for (std::uint32_t index = 0; index < 22; ++index) {
        const auto address = 0x8ee850 + index * 16;
        const auto& record = records[index + 1];
        Check(record.id == index * 4 && record.source_address == address &&
                  record.kind == pal4::GameDataPropertyKind::text &&
                  record.name == std::to_string(address) + "!" &&
                  runtime.sources[index] == address,
              "parameter slot, source buffer and text category");
    }
    Check(runtime.sources.size() == 22,
          "all 22 string buffers are consulted");
    Check(registry.Process(0x5000, 0x45, 0x67, runtime) == 0xabu &&
              runtime.processed_object == 0x5000 &&
              runtime.processed_collection == 0x8ee818 &&
              runtime.calls.size() == 31 &&
              runtime.calls[28] == "reset name" &&
              runtime.calls[29] == "release name" &&
              runtime.calls[30] == "process" &&
              registry.Records().size() == 23,
          "wrapper initializes once then forwards collection and low byte");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" &&
              registry.Records().empty() && registry.RecordsInitialized(),
          "CRT cleanup leaves the one-time guard set");
}

}  // namespace

int main() {
    try {
        CheckRegistrationAndProcessing();
        std::cout << "game_data_upgrade_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
