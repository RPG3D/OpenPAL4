#include "pal4/game_data_property_registry.h"

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

struct Runtime final : pal4::IGameDataPropertyRegistryRuntime {
    std::vector<pal4::GameDataFieldDescriptor> appended;
    std::vector<std::string> calls;
    std::uint32_t processed_collection = 0;
    std::size_t processed_fields = 0;
    std::uint8_t process_result = 0x6c;

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
        calls.emplace_back("source");
        return "src" + std::to_string(address);
    }
    std::string ErrorSuffix() override { return "_E"; }
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

void CheckField(const pal4::GameDataFieldDescriptor& actual,
                std::uint32_t offset, const std::string& name,
                pal4::GameDataFieldKind kind, std::uint32_t vtable,
                std::uint32_t source) {
    Check(actual == pal4::GameDataFieldDescriptor{
                        offset, name, kind, vtable, 24, source, 0},
          "field descriptor matches IDA offset, name, kind and layout");
}

void CheckInitialization() {
    pal4::GameDataPropertyRegistry registry;
    Runtime runtime;
    const auto& fields = registry.Initialize(runtime);
    Check(fields.size() == 21 && runtime.appended.size() == 21 &&
              runtime.calls.front() == "globals" &&
              runtime.calls[1] == "register" &&
              runtime.calls.back() == "sort",
          "registry sets up globals, appends 21 fields, then sorts");

    using Kind = pal4::GameDataFieldKind;
    CheckField(runtime.appended[0], 0, "ID_E", Kind::numeric,
               0x843368, 0);
    CheckField(runtime.appended[1], 4, "TYPE_E", Kind::numeric,
               0x843368, 0);
    CheckField(runtime.appended[2], 8, "AI_CMD_TYPE_E", Kind::numeric,
               0x843368, 0);

    for (std::uint32_t i = 0; i < 7; ++i) {
        const auto source = 0x8ef080 + i * 16;
        CheckField(runtime.appended[3 + i], 16 + i * 16,
                   "src" + std::to_string(source) + "_E",
                   Kind::text, 0x844574, source);
    }
    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto source = 0x8ef030 + i * 16;
        CheckField(runtime.appended[10 + i], 144 + i * 4,
                   "src" + std::to_string(source) + "_E",
                   Kind::numeric, 0x843368, source);
    }
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto source = 0x8ef9c8 + i * 16;
        CheckField(runtime.appended[15 + i], 128 + i * 4,
                   "src" + std::to_string(source) + "_CAN_USE",
                   Kind::numeric, 0x843368, source);
    }
    CheckField(runtime.appended[19], 12, "TARGET_E", Kind::numeric,
               0x843368, 0);
    CheckField(runtime.appended[20], 164, "PA_ATK_DELAY_E",
               Kind::hit_extra, 0x8444dc, 0);

    constexpr std::array<std::uint32_t, 21> kSortedOffsets{
        0, 4, 8, 12, 16, 32, 48, 64, 80, 96, 112,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164};
    for (std::size_t i = 0; i < fields.size(); ++i)
        Check(fields[i].offset == kSortedOffsets[i],
              "runtime sorted all sparse record offsets");
    const auto calls = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == calls &&
              runtime.appended.size() == 21,
          "second initialization does not register duplicate fields");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" && fields.empty(),
          "cleanup releases the collection");
}

void CheckProcessing() {
    pal4::GameDataPropertyRegistry registry;
    Runtime runtime;
    Check(registry.Process(0x125000, runtime) == 0x6c &&
              runtime.processed_collection == 0x125000 &&
              runtime.processed_fields == 21 &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "process",
          "process wrapper initializes and sorts before forwarding");
    runtime.calls.clear();
    Check(registry.Process(0x126000, runtime) == 0x6c &&
              runtime.calls == std::vector<std::string>{"process"} &&
              runtime.processed_collection == 0x126000,
          "repeated processing reuses the initialized registry");
}

}  // namespace

int main() {
    try {
        CheckInitialization();
        CheckProcessing();
        std::cout << "game_data_property_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
