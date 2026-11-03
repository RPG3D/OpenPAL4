#include "pal4/game_data_magic_registry.h"

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

struct Runtime final : pal4::IGameDataMagicRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::GameDataFieldDescriptor> appended;
    bool reverse_on_sort = false;

    void InitializeGlobalStorage() override { calls.emplace_back("globals"); }
    void RegisterGlobalCleanup() override { calls.emplace_back("atexit"); }
    void CleanupGlobalStorage() override { calls.emplace_back("cleanup"); }
    std::string ErrorSuffix() override { return "!"; }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        calls.emplace_back("append");
        appended.push_back(field);
        fields.push_back(std::move(field));
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("sort");
        Check(fields.size() == 17, "sort follows 17 field registrations");
        if (reverse_on_sort) std::reverse(fields.begin(), fields.end());
    }
    std::uint8_t ProcessCollection(
        std::uint32_t object,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("process");
        Check(object == 0x5000 && fields.size() == 17 &&
                  fields.front().offset == (reverse_on_sort ? 136u : 0u),
              "wrapper forwards object and sorted registry");
        return 0xe7;
    }
};

void CheckFields() {
    Runtime runtime;
    pal4::GameDataMagicRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    constexpr std::array<std::uint32_t, 17> offsets{
        0, 4, 8, 12, 28, 44, 60, 64, 68, 72, 76, 80, 84,
        88, 104, 120, 136};
    constexpr std::array<std::string_view, 17> names{
        "ID!", "FELEMENTS!", "AI_CMD_TYPE!", "NAME!",
        "DESCRIPTION!", "PROP!", "NEED_POINT!",
        "NEED_TOTAL_USED_POINT!", "PARENT_MAGIC!", "ATTACHED_SKILL!",
        "CONSUMED_MP!", "TARGET!", "HIT_EXTRA!", "ANIMATION!",
        "EFFECT!", "TARGET_EF!", "TARGET_BIND!"};
    Check(fields.size() == 17 && runtime.appended.size() == 17,
          "exact magic field count");
    Check(runtime.calls.front() == "globals" &&
              runtime.calls[1] == "atexit" &&
              runtime.calls.back() == "sort",
          "global init and CRT precede fields, sort follows them");
    for (std::size_t i = 0; i < fields.size(); ++i) {
        Check(fields[i].offset == offsets[i] && fields[i].name == names[i] &&
                  fields[i].object_size == 24 &&
                  fields[i].source_address == 0 &&
                  fields[i].source_count == 0,
              "magic field name, offset and 24-byte layout");
        const auto kind = i == 12 ? pal4::GameDataFieldKind::hit_extra
                         : (i >= 3 && i <= 5) || i >= 13
                             ? pal4::GameDataFieldKind::text
                             : pal4::GameDataFieldKind::numeric;
        const auto vtable = kind == pal4::GameDataFieldKind::hit_extra
                                ? 0x8444dcu
                            : kind == pal4::GameDataFieldKind::text
                                ? 0x844574u
                                : 0x843368u;
        Check(fields[i].kind == kind && fields[i].vtable == vtable,
              "magic field dispatch type and vtable");
    }
    const auto call_count = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == call_count,
          "repeated initialization leaves registered fields intact");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup",
          "CRT callback delegates to collection cleanup");
}

void CheckWrapper() {
    Runtime runtime;
    runtime.reverse_on_sort = true;
    pal4::GameDataMagicRegistry registry;
    Check(registry.Process(0x5000, runtime) == 0xe7 &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "process",
          "wrapper passes sorted collection and forwards result byte");
}

}  // namespace

int main() {
    try {
        CheckFields();
        CheckWrapper();
        std::cout << "game_data_magic_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
