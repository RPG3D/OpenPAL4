#include "pal4/game_data_mission_registry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMissionRegistryRuntime {
    std::vector<std::string> calls;
    bool reverse_on_sort = false;

    void InitializeGlobalStorage() override { calls.emplace_back("globals"); }
    void RegisterGlobalCleanup() override { calls.emplace_back("atexit"); }
    void CleanupGlobalStorage() override { calls.emplace_back("cleanup"); }
    std::string ErrorSuffix() override { return "!"; }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        calls.emplace_back("append");
        fields.push_back(std::move(field));
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("sort");
        Check(fields.size() == 8, "sort follows eight field registrations");
        if (reverse_on_sort) std::reverse(fields.begin(), fields.end());
    }
    std::uint8_t ProcessCollection(
        std::uint32_t object,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("process");
        Check(object == 0x5000 && fields.size() == 8 &&
                  fields.front().offset == (reverse_on_sort ? 36u : 0u),
              "Mission wrapper forwards object and sorted fields");
        return 0xd5;
    }
};

void CheckFields() {
    Runtime runtime;
    pal4::GameDataMissionRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    constexpr std::array<std::uint32_t, 8> offsets{
        0, 8, 4, 16, 12, 20, 52, 36};
    constexpr std::array<std::string_view, 8> names{
        "DEPENDED_ID!", "TRUNK!", "QUEST_ID!", "STORY_SHOW!",
        "STORY_PER!", "NAME!", "DESCRIPTION!", "PICTURE!"};
    Check(fields.size() == 8 && runtime.calls.size() == 11 &&
              runtime.calls[0] == "globals" &&
              runtime.calls[1] == "atexit" &&
              runtime.calls.back() == "sort",
          "one-time globals precede eight fields and sorting");
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto kind = i == 4 ? pal4::GameDataFieldKind::hit_extra
                         : i >= 5 ? pal4::GameDataFieldKind::text
                                  : pal4::GameDataFieldKind::numeric;
        const auto vtable = i == 4 ? 0x8444dcu
                            : i >= 5 ? 0x844574u : 0x843368u;
        Check(fields[i].offset == offsets[i] && fields[i].name == names[i] &&
                  fields[i].kind == kind && fields[i].vtable == vtable &&
                  fields[i].object_size == 24 &&
                  fields[i].source_address == 0 &&
                  fields[i].source_count == 0,
              "all Mission fields have exact names, offsets and vtables");
    }
    const auto call_count = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == call_count,
          "repeated initialization does not append again");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup",
          "CRT callback delegates to Mission collection cleanup");
}

void CheckWrapper() {
    Runtime runtime;
    runtime.reverse_on_sort = true;
    pal4::GameDataMissionRegistry registry;
    Check(registry.Process(0x5000, runtime) == 0xd5 &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "process",
          "wrapper passes sorted fields and returns event result byte");
}

}  // namespace

int main() {
    try {
        CheckFields();
        CheckWrapper();
        std::cout << "game_data_mission_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
