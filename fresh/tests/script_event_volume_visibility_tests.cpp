#include "pal4/script_event_volume_visibility.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptEventVolumeVisibilityRuntime {
    std::uint32_t name = 0x5000u;
    std::uint32_t volume = 0x6000u;
    std::uint32_t arena_calls = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1004u, "event volume name payload offset");
        calls.emplace_back("name"); return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x4000u;
    }
    std::uint32_t EventVolumeManager() override {
        calls.emplace_back("volume manager"); return 0x2000u;
    }
    std::uint32_t FindVolume(std::uint32_t manager,
                             std::uint32_t resolved_name) override {
        Check(manager == 0x2000u && resolved_name ==
                  (name == 0u ? 0x4000u : name),
              "event volume lookup arguments");
        calls.emplace_back("find"); return volume;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x3000u + arena_calls++ * 0x100u;
    }
    std::uint32_t Deactivate(std::uint32_t arena,
                             std::uint32_t resolved_volume) override {
        Check(arena == 0x3000u && resolved_volume == volume,
              "deactivate first arena and found volume");
        calls.emplace_back("deactivate"); return 0x81u;
    }
    std::uint32_t Activate(std::uint32_t arena,
                           std::uint32_t resolved_volume) override {
        Check(arena == 0x3100u && resolved_volume == volume,
              "activate refreshed arena and found volume");
        calls.emplace_back("activate"); return 0x82u;
    }
};

void VisibilityBranches() {
    Runtime missing;
    missing.volume = 0u;
    Check(pal4::ScriptEventVolumeVisibility::Set(
                  0x1000u, 1u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "volume manager", "find"},
          "missing event volume skips arena");
    Runtime hidden;
    hidden.name = 0u;
    Check(pal4::ScriptEventVolumeVisibility::Set(
                  0x1000u, 0u, hidden) == 0x81u &&
              hidden.calls == std::vector<std::string>{
                  "name", "empty", "volume manager", "find",
                  "arena", "deactivate"},
          "zero visibility deactivates with empty-name fallback");
    for (const auto visible : {1u, 2u, 255u}) {
        Runtime shown;
        Check(pal4::ScriptEventVolumeVisibility::Set(
                  0x1000u, static_cast<std::uint8_t>(visible), shown) ==
                  0x82u &&
              shown.calls == std::vector<std::string>{
                  "name", "volume manager", "find", "arena",
                  "deactivate", "arena", "activate"},
              "nonzero visibility deactivates then reactivates");
    }
}

}  // namespace

int main() {
    try {
        VisibilityBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
