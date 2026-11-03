#include "pal4/game_data_parameter_registry.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataParameterRegistryRuntime {
    std::vector<pal4::GameDataFieldDescriptor> appended;
    std::vector<std::string> calls;
    std::uint32_t processed_collection = 0;
    std::size_t processed_fields = 0;
    std::uint8_t process_result = 0x5b;

    void InitializeGlobalStorage() override {
        calls.emplace_back("globals");
    }
    void RegisterGlobalCleanup() override {
        calls.emplace_back("register");
    }
    void CleanupGlobalStorage() override {
        calls.emplace_back("cleanup");
    }
    std::string SourceName(std::uint32_t address) override {
        calls.push_back("source " + std::to_string(address));
        return "src" + std::to_string(address);
    }
    std::string ErrorSuffix() override { return ""; }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        appended.push_back(field);
        fields.push_back(std::move(field));
        calls.emplace_back("append");
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("sort");
        std::sort(fields.begin(), fields.end(),
                  [](const auto& left, const auto& right) {
                      return left.offset < right.offset;
                  });
    }
    std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("process");
        processed_collection = collection;
        processed_fields = fields.size();
        return process_result;
    }
};

void CheckGroup(const Runtime& runtime, std::size_t begin,
                std::uint32_t first_offset, std::uint32_t count,
                std::uint32_t source_base, std::uint32_t source_step,
                const std::string& suffix, pal4::GameDataFieldKind kind,
                std::uint32_t vtable) {
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto& field = runtime.appended.at(begin + i);
        const auto address = source_base + i * source_step;
        Check(field.offset == first_offset + i * (kind==pal4::GameDataFieldKind::text?16u:4u) &&
                  field.name == "src" + std::to_string(address) + suffix &&
                  field.kind == kind && field.vtable == vtable &&
                  field.object_size == 24 &&
                  field.source_address == address && field.source_count == 0,
              "descriptor source, offset, text and native class layout");
    }
}

void CheckInitialization() {
    pal4::GameDataParameterRegistry registry;
    Runtime runtime;
    const auto& fields = registry.Initialize(runtime);
    Check(fields.size() == 64 && runtime.appended.size() == 64 &&
              runtime.calls.front() == "globals" &&
              runtime.calls[1] == "register" &&
              runtime.calls.back() == "sort",
          "registry appends 64 fields after global setup and sorts last");
    const auto& first = runtime.appended;
    Check(first[0].offset == 0 && first[0].name == "ID" &&
              first[1].offset == 100 && first[1].name == "TYPE" &&
              first[2].offset == 324 && first[2].name == "CALC_TYPE",
          "three fixed fields lead the registration order");
    for (std::size_t i = 0; i < 3; ++i)
        Check(first[i].kind == pal4::GameDataFieldKind::numeric &&
                  first[i].vtable == 0x843368 &&
                  first[i].object_size == 24,
              "fixed field descriptors use numeric class");

    using Kind = pal4::GameDataFieldKind;
    CheckGroup(runtime, 3, 4, 6, 0x8eef90, 16, "", Kind::text, 0x844574);
    CheckGroup(runtime, 9, 124, 7, 0x8ef958, 16, "", Kind::numeric,
               0x843368);
    CheckGroup(runtime, 16, 152, 4, 0x8eede0, 16, "", Kind::numeric,
               0x843368);
    CheckGroup(runtime, 20, 168, 4, 0x8eede0, 16, "_NUMBER", Kind::numeric,
               0x843368);
    CheckGroup(runtime, 24, 184, 8, 0x8eeec0, 16, "", Kind::numeric,
               0x843368);
    CheckGroup(runtime, 32, 216, 5, 0x8efa08, 16, "", Kind::numeric,
               0x843368);
    CheckGroup(runtime, 37, 236, 6, 0x8eee20, 16, "_ADD", Kind::numeric,
               0x843368);
    CheckGroup(runtime, 43, 260, 6, 0x8eee20, 16, "_EXTRACT", Kind::hit_extra,
               0x8444dc);
    CheckGroup(runtime, 49, 284, 6, 0x8eee20, 16, "_REACT", Kind::hit_extra,
               0x8444dc);
    CheckGroup(runtime, 55, 308, 4, 0x8eee80, 16, "", Kind::hit_extra,
               0x8444dc);
    CheckGroup(runtime, 59, 104, 5, 0x8eef40, 16, "", Kind::numeric,
               0x843368);
    Check(fields.front().offset == 0 && fields.back().offset == 324,
          "runtime sort orders the registered offsets");
    std::array<unsigned,328> coverage{};
    for(const auto& field:fields){const auto size=field.kind==Kind::text?16u:4u;
        Check(field.offset+size<=coverage.size(),"Prescription field fits original328-byte record");
        for(unsigned i=0;i<size;++i)Check(++coverage[field.offset+i]==1u,"Prescription fields cannot overlap");}
    for(const auto value:coverage)Check(value==1u,"Prescription registry covers every original record byte");

    const auto calls = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == calls && runtime.appended.size() == 64,
          "repeat initialization leaves the field registry unchanged");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" && fields.empty(),
          "cleanup releases global storage and clears local descriptors");
}

void CheckProcessing() {
    pal4::GameDataParameterRegistry registry;
    Runtime runtime;
    Check(registry.Process(0x123400, runtime) == 0x5b &&
              runtime.processed_collection == 0x123400 &&
              runtime.processed_fields == 64 &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "process",
          "wrapper initializes registry before processing collection");
    runtime.calls.clear();
    Check(registry.Process(0x567800, runtime) == 0x5b &&
              runtime.calls.size() == 1 &&
              runtime.calls.front() == "process" &&
              runtime.processed_collection == 0x567800,
          "repeated processing reuses initialized registry");
}

}  // namespace

int main() {
    try {
        CheckInitialization();
        CheckProcessing();
        std::cout << "game_data_parameter_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
