#include "pal4/ui_frame_manager_singleton.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Create = std::uint32_t (*)(
    std::uint32_t, pal4::IUiFrameManagerCachedFrameRuntime&);

struct Case {
    const char* label;
    Create create;
    std::uint32_t offset;
    std::uint32_t bytes;
    pal4::UiCachedFrameKind kind;
    const char* name;
};

constexpr std::array<Case, 6> kCases{{
    {"scenario", &pal4::UiFrameManagerSingleton::CreateScenarioDialog,
     0x1b4u, 0x50u, pal4::UiCachedFrameKind::Scenario, nullptr},
    {"progress", &pal4::UiFrameManagerSingleton::CreateProgressBar,
     0x1b8u, 0x38u, pal4::UiCachedFrameKind::ProgressBar, nullptr},
    {"loading", &pal4::UiFrameManagerSingleton::CreateLoadingWindow,
     0x1bcu, 0x3cu, pal4::UiCachedFrameKind::Loading, nullptr},
    {"hint", &pal4::UiFrameManagerSingleton::CreateHintWindow,
     0x1c0u, 0x48u, pal4::UiCachedFrameKind::Hint, "hintWindow"},
    {"combat hint", &pal4::UiFrameManagerSingleton::CreateCombatHintWindow,
     0x1c4u, 0x48u, pal4::UiCachedFrameKind::CombatHint,
     "CombathintWindow"},
    {"combat hint 2", &pal4::UiFrameManagerSingleton::CreateCombatHintWindow2,
     0x1c8u, 0x48u, pal4::UiCachedFrameKind::CombatHint2, nullptr},
}};

struct Runtime final : pal4::IUiFrameManagerCachedFrameRuntime {
    explicit Runtime(const Case& test_case) : test_case(test_case) {}

    const Case& test_case;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t allocation = 0xa000u;
    bool throw_construct = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5000u + test_case.offset,
              "cache field read offset");
        events.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x5000u + test_case.offset,
              "cache field write offset");
        events.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == test_case.bytes, "IDA allocation size");
        events.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Construct(pal4::UiCachedFrameKind kind,
                            std::uint32_t memory,
                            const char* name) override {
        Check(memory == 0xa000u && kind == test_case.kind,
              "constructor kind and memory");
        Check((name == nullptr && test_case.name == nullptr) ||
                  (name != nullptr && test_case.name != nullptr &&
                   std::string_view(name) == test_case.name),
              "constructor resource name");
        events.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct failed");
        return 0xb000u;
    }
    void FreeAfterConstructionFailure(std::uint32_t memory) override {
        Check(memory == 0xa000u, "SEH frees original allocation");
        events.emplace_back("free");
    }
};

void CheckOne(const Case& test_case) {
    Runtime created(test_case);
    Check(test_case.create(0x5000u, created) == 0xb000u &&
              created.words[0x5000u + test_case.offset] == 0xb000u &&
              created.events == std::vector<std::string>{
                  "read", "allocate", "construct", "write", "read"},
          test_case.label);

    Runtime cached(test_case);
    cached.words[0x5000u + test_case.offset] = 0xc000u;
    Check(test_case.create(0x5000u, cached) == 0xc000u &&
              cached.events == std::vector<std::string>{"read", "read"},
          "cached frame skips allocation and constructor");

    Runtime allocation_failed(test_case);
    allocation_failed.allocation = 0u;
    Check(test_case.create(0x5000u, allocation_failed) == 0u &&
              allocation_failed.events == std::vector<std::string>{
                  "read", "allocate", "write", "read"},
          "null allocation caches zero without constructor");

    Runtime construction_failed(test_case);
    construction_failed.throw_construct = true;
    bool caught = false;
    try {
        (void)test_case.create(0x5000u, construction_failed);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "construct failed";
    }
    Check(caught && construction_failed.events ==
                        std::vector<std::string>{
                            "read", "allocate", "construct", "free"} &&
              construction_failed.words[0x5000u + test_case.offset] == 0u,
          "constructor exception frees before cache write");
}

}  // namespace

int main() {
    try {
        for (const auto& test_case : kCases) CheckOne(test_case);
        std::cout << "ui_frame_manager_cached_windows_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
