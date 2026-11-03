#include "pal4/game_data_monster_table_registry.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMonsterTableRegistryRuntime {
    std::vector<std::string> calls;
    bool reverse_on_sort = false;

    void InitializeGlobalStorage() override { calls.emplace_back("globals"); }
    void RegisterGlobalCleanup() override { calls.emplace_back("atexit"); }
    void CleanupGlobalStorage() override { calls.emplace_back("cleanup"); }
    std::string ErrorSuffix() override {
        calls.emplace_back("suffix");
        return "!";
    }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        calls.emplace_back("append");
        fields.push_back(std::move(field));
    }
    void ImportStructureFields(
        std::vector<pal4::GameDataFieldDescriptor>& fields,
        std::uint32_t offset) override {
        Check(offset == 0 && fields.size() == 1,
              "base fields import follows MONSTER_ID with zero offset");
        calls.emplace_back("structure");
        fields.push_back({offset + 8, "BASE", pal4::GameDataFieldKind::text,
                          0x844574, 24, 0, 0});
    }
    void ImportMonsterFields(
        std::vector<pal4::GameDataFieldDescriptor>& fields,
        std::uint32_t offset) override {
        Check(offset == 336 && fields.size() == 2,
              "monster fields import follows base fields at offset 336");
        calls.emplace_back("monster");
        fields.push_back({offset + 20, "MA_NAME",
                          pal4::GameDataFieldKind::numeric,
                          0x843368, 24, 0, 0});
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        Check(fields.size() == 3, "sort follows both imports");
        calls.emplace_back("sort");
        if (reverse_on_sort) std::reverse(fields.begin(), fields.end());
    }
    std::uint8_t ProcessCollection(
        std::uint32_t object,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        Check(object == 0x7000 && fields.size() == 3 &&
                  fields.front().offset == (reverse_on_sort ? 356u : 400u),
              "processing receives object and sorted collection");
        calls.emplace_back("process");
        return 0xd3;
    }
};

void CheckInitializeAndCleanup() {
    Runtime runtime;
    pal4::GameDataMonsterTableRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    Check(fields.size() == 3 &&
              fields[0] == pal4::GameDataFieldDescriptor{
                               400, "MONSTER_ID!",
                               pal4::GameDataFieldKind::numeric,
                               0x843368, 24, 0, 0},
          "MONSTER_ID literal, offset and descriptor layout match IDA");
    Check(runtime.calls == std::vector<std::string>{
                               "globals", "atexit", "suffix", "append",
                               "structure", "monster", "sort"},
          "global init, own field, both imports and sort follow IDA order");
    const auto call_count = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == call_count,
          "one-time guards prevent repeated imports");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" && fields.empty(),
          "CRT callback delegates collection cleanup");
}

void CheckProcess() {
    Runtime runtime;
    runtime.reverse_on_sort = true;
    pal4::GameDataMonsterTableRegistry registry;
    Check(registry.Process(0x7000, runtime) == 0xd3 &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "process",
          "process wrapper initializes, sorts and forwards the result byte");
}

}  // namespace

int main() {
    try {
        CheckInitializeAndCleanup();
        CheckProcess();
        std::cout << "game_data_monster_table_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
