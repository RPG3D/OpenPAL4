#include "pal4/game_data_est_registry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEstRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::GameDataFieldDescriptor> appended;
    std::uint32_t processed_collection = 0;
    std::size_t processed_count = 0;

    void InitializeGlobalStorage(std::uint32_t address) override {
        Check(address == 0x8ef0f8, "global collection address");
        calls.emplace_back("globals");
    }
    void RegisterGlobalCleanup(std::uint32_t callback) override {
        Check(callback == 0x4e98d0, "CRT cleanup callback");
        calls.emplace_back("register");
    }
    void CleanupGlobalStorage(std::uint32_t address) override {
        Check(address == 0x8ef0f8, "cleanup collection address");
        calls.emplace_back("cleanup");
    }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        appended.push_back(field);
        fields.push_back(std::move(field));
        calls.emplace_back("append");
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        std::sort(fields.begin(), fields.end(),
                  [](const auto& a, const auto& b) {
                      return a.offset < b.offset;
                  });
        calls.emplace_back("sort");
    }
    std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        processed_collection = collection;
        processed_count = fields.size();
        calls.emplace_back("process");
        return 0x5a;
    }
};

void CheckRegistry() {
    constexpr std::array<const char*, 32> names{
        "KEY", "BASE", "PRIZE_S", "PRIZE_1", "PRIZE_2", "PRIZE_3",
        "EST_PROTECT", "EST_JOINT_ATK", "EST_CRITICAL", "EST_MISSED",
        "EST_FEND_OFF", "EST_REACT_ATK", "EST_CAST_MAGIC", "EST_STUNT",
        "EST_USE_PROPERTY", "EST_REVIVE", "EST_FLEE_FAIL", "EST_ROLE_DEAD",
        "EST_UNATTACKED", "EST_HP_IN_MAX", "EST_HP_IN_DANGER",
        "EST_TAKE_ACTION", "EST_MONSTER_DIDNT_ACT", "EST_MONSTER_FLEE",
        "EST_MONSTER_DEAD", "EST_LEVEL_UPPER", "EST_LEVEL_LOWER",
        "EST_PHYSICAL_ATK", "EST_KNOCK_OUT", "EST_WIN_AT_THE_FIRST_BOUT",
        "EST_KO_BY_ONE_ROLE", "EST_ALL_SURVIVE"};
    Runtime runtime;
    pal4::GameDataEstRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    Check(runtime.calls.size() == 35 &&
              runtime.calls[0] == "globals" &&
              runtime.calls[1] == "register" &&
              runtime.calls.back() == "sort", "initialization order");
    Check(fields.size() == names.size() &&
              runtime.appended.size() == names.size(), "descriptor count");
    for (std::size_t i = 0; i < names.size(); ++i) {
        const pal4::GameDataFieldDescriptor expected{
            static_cast<std::uint32_t>(i * 4), names[i],
            pal4::GameDataFieldKind::numeric, 0x843368, 24, 0, 0};
        Check(runtime.appended[i] == expected && fields[i] == expected,
              "field literal, order, offset and descriptor type");
    }
    const auto previous_calls = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == previous_calls, "once-only guards");
    Check(registry.Process(0x9000, runtime) == 0x5a &&
              runtime.processed_collection == 0x9000 &&
              runtime.processed_count == 32 &&
              runtime.calls.size() == previous_calls + 1 &&
              runtime.calls.back() == "process", "wrapper forwards collection");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup" && fields.empty(),
          "cleanup forwards global address and releases fields");
}

}  // namespace

int main() {
    try {
        CheckRegistry();
        std::cout << "game_data_est_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
