#include "pal4/game_data_attribute_registries.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Group = pal4::GameDataAttributeGroup;
using Property = pal4::GameDataProperty;

struct Runtime final : pal4::IGameDataAttributeRegistriesRuntime {
    struct Appended {
        Group group;
        Property record;
        std::uint8_t seed;
    };
    std::vector<std::string> calls;
    std::vector<Appended> appended;
    std::vector<std::uint32_t> sources;
    std::size_t suffix_calls = 0;
    std::uint32_t processed_object = 0;
    std::uint32_t processed_collection = 0;
    std::uint32_t process_result = 0x1234;

    void InitializeStorage(Group group, std::uint8_t first_seed,
                           std::uint8_t second_seed) override {
        Check(first_seed == 0x12 && second_seed == 0x34,
              "both unspecified bytes are supplied explicitly");
        calls.emplace_back(group == Group::conditions ? "conditions storage"
                                                        : "parameters storage");
    }
    void RegisterCleanup(Group group) override {
        calls.emplace_back(group == Group::conditions ? "conditions register"
                                                        : "parameters register");
    }
    void CleanupStorage(Group group) override {
        calls.emplace_back(group == Group::conditions ? "conditions cleanup"
                                                        : "parameters cleanup");
    }
    void AppendRecord(Group group, const Property& record,
                      std::uint8_t seed) override {
        appended.push_back({group, record, seed});
        calls.emplace_back("append");
    }
    std::string SourceName(std::uint32_t address) override {
        sources.push_back(address);
        return "source_" + std::to_string(address);
    }
    std::string ErrorSuffix() override {
        ++suffix_calls;
        return "!";
    }
    void Sort(Group group, std::vector<Property>& records) override {
        Check(records.size() == (group == Group::conditions ? 14 : 17),
              "sort follows complete group registration");
        calls.emplace_back(group == Group::conditions ? "conditions sort"
                                                        : "parameters sort");
    }
    std::uint32_t ProcessCollection(std::uint32_t object,
                                    std::uint32_t collection) override {
        processed_object = object;
        processed_collection = collection;
        calls.emplace_back("process");
        return process_result;
    }
};

void CheckGroup(const std::vector<Property>& records, Group group) {
    const bool conditions = group == Group::conditions;
    const std::uint32_t first_id = conditions ? 4 : 16;
    const std::uint32_t first_source = conditions ? 0x8ee710 : 0x8ee640;
    const std::size_t prefix_count = conditions ? 1 : 4;
    Check(records.size() == prefix_count + 13,
          "one or four leading names plus thirteen source records");
    const std::vector<std::string> expected_prefix = conditions
        ? std::vector<std::string>{"AI_TYPE!"}
        : std::vector<std::string>{"ID!", "AI_TYPE!", "TEAM_STATE!",
                                   "ENEMY_STATE!"};
    for (std::size_t i = 0; i < prefix_count; ++i)
        Check(records[i].id == i * 4 && records[i].source_address == 0 &&
                  records[i].kind == pal4::GameDataPropertyKind::numeric &&
                  records[i].name == expected_prefix[i],
              "leading property ID, type and name");
    for (std::size_t i = 0; i < 13; ++i) {
        const auto& record = records[prefix_count + i];
        const std::uint32_t source = first_source +
                                     static_cast<std::uint32_t>(i * 16);
        Check(record.id == first_id + i * 4 &&
                  record.source_address == source &&
                  record.kind == pal4::GameDataPropertyKind::text &&
                  record.name == "source_" + std::to_string(source) + "!",
              "array property ID, 16-byte stride, type and name");
    }
}

void CheckRegistries() {
    Runtime runtime;
    pal4::GameDataAttributeRegistries registries;
    Check(registries.InitializeConditions(0x12, 0x34, runtime) == 0x8ee60c &&
              runtime.calls[0] == "conditions storage" &&
              runtime.calls[1] == "conditions register" &&
              runtime.calls.back() == "conditions sort",
          "conditions storage, cleanup registration and sort order");
    CheckGroup(registries.Conditions(), Group::conditions);
    Check(runtime.appended.size() == 14 && runtime.suffix_calls == 14 &&
              runtime.sources.size() == 13,
          "conditions append and source reads happen per record");
    for (const auto& value : runtime.appended)
        Check(value.group == Group::conditions && value.seed == 0x34,
              "conditions constructors receive group and second seed");
    const std::size_t conditions_calls = runtime.calls.size();
    Check(registries.ProcessConditions(0x1000, 0x12, 0x34, runtime) == 0x34 &&
              runtime.processed_object == 0x1000 &&
              runtime.processed_collection == 0x8ee60c &&
              runtime.calls.size() == conditions_calls + 1,
          "wrapper initializes once and forwards collection result low byte");

    Check(registries.InitializeParameters(0x12, 0x34, runtime) == 0x8ee5d8,
          "parameters collection has distinct global address");
    CheckGroup(registries.Parameters(), Group::parameters);
    Check(runtime.appended.size() == 31 && runtime.suffix_calls == 31 &&
              runtime.sources.size() == 26,
          "parameters add seventeen independent records");
    for (std::size_t i = 14; i < runtime.appended.size(); ++i)
        Check(runtime.appended[i].group == Group::parameters &&
                  runtime.appended[i].seed == 0x34,
              "parameters constructors receive their own group and seed");
    const std::size_t parameters_calls = runtime.calls.size();
    runtime.process_result = 0x5678;
    Check(registries.ProcessParameters(0x2000, 0x12, 0x34, runtime) == 0x78 &&
              runtime.processed_object == 0x2000 &&
              runtime.processed_collection == 0x8ee5d8 &&
              runtime.calls.size() == parameters_calls + 1,
          "parameter wrapper forwards distinct collection and low byte");

    registries.CleanupConditions(runtime);
    Check(registries.Conditions().empty() &&
              registries.Parameters().size() == 17 &&
              runtime.calls.back() == "conditions cleanup",
          "condition cleanup leaves parameter collection intact");
    const std::size_t after_cleanup = runtime.calls.size();
    Check(registries.InitializeConditions(0x12, 0x34, runtime) == 0x8ee60c &&
              runtime.calls.size() == after_cleanup,
          "CRT cleanup leaves native one-time guards set");
    registries.CleanupParameters(runtime);
    Check(registries.Parameters().empty() &&
              runtime.calls.back() == "parameters cleanup",
          "parameter cleanup invokes its own native collection destructor");
}

}  // namespace

int main() {
    try {
        CheckRegistries();
        std::cout << "game_data_attribute_registries_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
