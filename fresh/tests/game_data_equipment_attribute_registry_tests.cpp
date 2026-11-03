#include "pal4/game_data_equipment_attribute_registry.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEquipmentAttributeRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::GameDataProperty> appended;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t process_result = 0x1234;

    void InitializeStorage(std::uint8_t first,
                           std::uint8_t second) override {
        Check(first == 0x12 && second == 0x34, "storage seeds");
        calls.emplace_back("storage");
    }
    void RegisterCleanup() override { calls.emplace_back("register"); }
    void CleanupStorage() override { calls.emplace_back("cleanup"); }
    void ResetTemporaryName(std::uint8_t seed) override {
        Check(seed == 0x34, "temporary name seed");
        calls.emplace_back("reset");
    }
    void ReleaseTemporaryName() override {
        calls.emplace_back("release");
    }
    std::string SourceName(std::uint32_t address) override {
        return "source:" + std::to_string(address);
    }
    std::string ErrorSuffix() override { return ":suffix"; }
    void AppendRecord(const pal4::GameDataProperty& record) override {
        appended.push_back(record);
        calls.emplace_back("append");
    }
    void Sort(std::vector<pal4::GameDataProperty>& records) override {
        Check(records.size() == 25, "sort after all 25 properties");
        calls.emplace_back("sort");
    }
    void ImportBaseCollection(std::uint32_t destination) override {
        Check(destination == 0x8ee9b8, "base import destination");
        calls.emplace_back("import");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5094, "equipment type member offset");
        calls.emplace_back("read");
        return words[address];
    }
    std::uint32_t ProcessCollection(std::uint32_t object,
                                    std::uint32_t collection) override {
        Check(object == 0x5000 && collection == 0x8ee9b8,
              "process object and collection");
        calls.emplace_back("process");
        return process_result;
    }
};

void CheckRegistrations() {
    Runtime runtime;
    pal4::GameDataEquipmentAttributeRegistry registry;
    Check(registry.Initialize(0x12, 0x34, runtime) == 0x8ee9b8 &&
              runtime.appended.size() == 25 &&
              registry.Records().size() == 25,
          "first initialization builds 25 EA properties");
    Check(runtime.calls.front() == "storage" &&
              runtime.calls[1] == "register" &&
              runtime.calls[2] == "reset" &&
              runtime.calls[3] == "import" &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "release",
          "storage, base import, append, sort and temporary lifetime order");

    const auto& values = runtime.appended;
    Check(values[0].id == 144 && values[0].name == "ID:suffix" &&
              values[0].kind == pal4::GameDataPropertyKind::numeric &&
              values[1].id == 148 && values[1].name == "TYPE:suffix",
          "ID and TYPE numeric properties lead the collection");
    for (std::uint32_t i = 0; i < 13; ++i) {
        const auto address = 0x8ee9f0 + i * 16;
        const auto& value = values[2 + i];
        Check(value.id == 152 + i * 16 &&
                  value.source_address == address &&
                  value.kind == pal4::GameDataPropertyKind::text &&
                  value.name == "source:" + std::to_string(address) +
                                    ":suffix",
              "13 EA text properties keep stride and names");
    }
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto address = 0x8ef9c8 + i * 4;
        const auto& value = values[15 + i];
        Check(value.id == 360 + i * 4 &&
                  value.source_address == address &&
                  value.kind == pal4::GameDataPropertyKind::numeric &&
                  value.name == "source:" + std::to_string(address) +
                                    "_LV_LMT",
              "four level-limit properties retain source and suffix");
    }
    for (std::uint32_t i = 0; i < 6; ++i) {
        const auto address = 0x8eeac0 + i * 16;
        const auto& value = values[19 + i];
        Check(value.id == 376 + i * 4 &&
                  value.source_address == address &&
                  value.kind == pal4::GameDataPropertyKind::numeric &&
                  value.name == "source:" + std::to_string(address) +
                                    ":suffix",
              "six EA numeric properties retain source and member offsets");
    }

    runtime.calls.clear();
    Check(registry.Initialize(0x12, 0x34, runtime) == 0x8ee9b8 &&
              runtime.calls ==
                  std::vector<std::string>{"reset", "release"} &&
              runtime.appended.size() == 25,
          "subsequent initialization only refreshes temporary text");
}

void CheckProcessAndCleanup() {
    Runtime runtime;
    pal4::GameDataEquipmentAttributeRegistry registry;
    Check(registry.Process(0x5000, 0x12, 0x34, runtime) == 0x34 &&
              runtime.calls.back() == "process",
          "wrapper forwards the low byte of processing result");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" && registry.Records().empty(),
          "CRT callback releases the shared collection model");
}

void CheckTypeMapping() {
    Runtime runtime;
    for (std::uint32_t value = 0; value <= 7; ++value) {
        runtime.words[0x5094] = value;
        Check(pal4::GameDataEquipmentAttributeRegistry::TypeInRange(
                  0x5000, runtime) == (value <= 6),
              "type upper bound is inclusive and unsigned");
        const auto expected = value >= 3 && value <= 6 ? value - 2 : 0;
        Check(pal4::GameDataEquipmentAttributeRegistry::TypeIndex(
                  0x5000, runtime) == expected,
              "types three to six map to indices one to four");
    }
    runtime.words[0x5094] = 0xffffffff;
    Check(!pal4::GameDataEquipmentAttributeRegistry::TypeInRange(
              0x5000, runtime) &&
              pal4::GameDataEquipmentAttributeRegistry::TypeIndex(
                  0x5000, runtime) == 0,
          "negative signed representation is out of unsigned range");
}

}  // namespace

int main() {
    try {
        CheckRegistrations();
        CheckProcessAndCleanup();
        CheckTypeMapping();
        std::cout << "game_data_equipment_attribute_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
