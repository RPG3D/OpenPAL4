#include "pal4/game_data_monster_registry.h"

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

struct Runtime final : pal4::IGameDataMonsterRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::GameDataFieldDescriptor> appended;
    bool reverse_on_sort = false;

    void InitializeGlobalStorage() override { calls.emplace_back("globals"); }
    void RegisterGlobalCleanup() override { calls.emplace_back("atexit"); }
    void CleanupGlobalStorage() override { calls.emplace_back("cleanup"); }
    std::string SourceName(std::uint32_t address) override {
        calls.emplace_back("source");
        return "source_" + std::to_string(address);
    }
    std::string ErrorSuffix() override {
        calls.emplace_back("suffix");
        return "!";
    }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        calls.emplace_back("append");
        appended.push_back(field);
        fields.push_back(std::move(field));
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("sort");
        Check(fields.size() == 31, "sort follows all 31 descriptors");
        if (reverse_on_sort) std::reverse(fields.begin(), fields.end());
    }
};

void CheckField(const pal4::GameDataFieldDescriptor& field,
                std::uint32_t offset, const std::string& name,
                pal4::GameDataFieldKind kind, std::uint32_t vtable,
                std::uint32_t size, std::uint32_t source = 0,
                std::uint32_t source_count = 0) {
    Check(field.offset == offset && field.name == name &&
              field.kind == kind && field.vtable == vtable &&
              field.object_size == size &&
              field.source_address == source &&
              field.source_count == source_count,
          "monster descriptor differs from IDA offset/name/layout/source");
}

void CheckDescriptors() {
    using pal4::GameDataFieldKind;
    Runtime runtime;
    pal4::GameDataMonsterRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    Check(fields.size() == 31 && runtime.appended.size() == 31 &&
              runtime.calls[0] == "globals" &&
              runtime.calls[1] == "atexit" &&
              runtime.calls.back() == "sort",
          "one-time global initialization precedes all descriptors and sort");

    CheckField(fields[0], 248, "IS_BOSS!", GameDataFieldKind::numeric,
               0x843368, 24);
    CheckField(fields[1], 68, "COUNT!", GameDataFieldKind::numeric,
               0x843368, 24);
    CheckField(fields[2], 252, "PHYSICAL_ATK_TARGETT!",
               GameDataFieldKind::action_type, 0x844494, 32, 0x8ef438, 2);
    CheckField(fields[3], 256, "PHYSICAL_ATK_RANGE!",
               GameDataFieldKind::action_type, 0x844494, 32, 0x8ef418, 2);

    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto source = 0x8eed50 + i * 16;
        CheckField(fields[4 + i], i * 4,
                   "source_" + std::to_string(source) + "!",
                   GameDataFieldKind::numeric, 0x843368, 24, source);
    }
    for (std::uint32_t i = 0; i < 6; ++i) {
        const auto source = 0x8eecf0 + i * 16;
        CheckField(fields[9 + i], 72 + i * 16,
                   "source_" + std::to_string(source) + "!",
                   GameDataFieldKind::text, 0x844574, 24, source);
    }
    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto source = 0x8efa58 + i * 16;
        CheckField(fields[15 + i], 168 + i * 16,
                   "source_" + std::to_string(source) + "!",
                   GameDataFieldKind::text, 0x844574, 24, source);
    }
    for (std::uint32_t i = 0; i < 3; ++i) {
        const auto source = 0x8eecc0 + i * 16;
        CheckField(fields[20 + i], 20 + i * 4,
                   "source_" + std::to_string(source) + "!",
                   GameDataFieldKind::numeric, 0x843368, 24, source);
    }
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto source = 0x8eec80 + i * 16;
        CheckField(fields[23 + i], 32 + i * 4,
                   "source_" + std::to_string(source) + "_ID",
                   GameDataFieldKind::numeric, 0x843368, 24, source);
        CheckField(fields[27 + i], 48 + i * 4,
                   "source_" + std::to_string(source) + "_RATE",
                   GameDataFieldKind::hit_extra, 0x8444dc, 24, source);
    }
    const auto calls = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == calls,
          "repeat initialization does not register descriptors again");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" && fields.empty(),
          "CRT callback delegates collection cleanup");
}

void CheckSortDelegation() {
    Runtime runtime;
    runtime.reverse_on_sort = true;
    pal4::GameDataMonsterRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    Check(fields.front().offset == 60 && fields.back().offset == 248 &&
              runtime.appended.front().offset == 248,
          "registry exposes runtime-sorted fields while preserving append order");
}

}  // namespace

int main() {
    try {
        CheckDescriptors();
        CheckSortDelegation();
        std::cout << "game_data_monster_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
