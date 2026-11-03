#include "pal4/script_sound_2d.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptSound2dRuntime {
    std::string name = "WA01";
    std::int32_t expected_mode = 0;
    std::uint32_t active = 1u;
    std::uint32_t handle = 0u;
    std::vector<std::string> calls;

    std::string_view SoundName(std::uint32_t source) override {
        Check(source == 0x5000u, "sound source object");
        calls.emplace_back("name"); return name;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio"); return 0x1000u;
    }
    std::uint32_t PlaySound(std::uint32_t audio,
                            std::uint32_t source,
                            std::string_view path,
                            std::int32_t mode) override {
        Check(audio == 0x1000u && source == 0x5000u &&
                  path == "gamedata\\PALSound\\" + name + ".wav" &&
                  mode == expected_mode,
              "sound path and mode forwarded to audio system");
        calls.emplace_back("play"); return 0x4321u;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor"); return 0x2000u;
    }
    std::int32_t InsertSoundId(std::uint32_t executor,
                               std::uint32_t audio_handle) override {
        Check(executor == 0x2000u && audio_handle == 0x4321u,
              "negative mode registers returned audio handle in script tree");
        calls.emplace_back("insert"); return 0x123u;
    }
    void SetCurrentHandle(std::uint32_t address,
                          std::uint32_t value) override {
        Check(address == 0x2020u, "current sound handle at +0x20");
        calls.emplace_back("handle"); handle = value;
    }
    std::uint32_t CheckActive(std::uint32_t address) override {
        Check(address == 0x2020u, "active-check handle offset");
        calls.emplace_back("active"); return active;
    }
    std::uint32_t StopCurrent(std::uint32_t address) override {
        Check(address == 0x2020u, "stop handle offset");
        calls.emplace_back("stop"); return 0x87u;
    }
    std::uint32_t RemoveSoundId(std::uint32_t executor,
                                std::int32_t id) override {
        Check(executor == 0x2000u && id >= 0,
              "nonnegative sound ID removal");
        calls.emplace_back("remove"); return 0x88u +
            static_cast<std::uint32_t>(id);
    }
};

void PlayBranches() {
    Runtime detached;
    Check(pal4::ScriptSound2d::Play(0x5000u, -2, detached) == 0x123 &&
              detached.calls == std::vector<std::string>{
                  "name", "audio", "play", "executor", "insert"},
          "negative mode plays with zero and inserts script sound ID");
    for (const auto mode : {0, 3}) {
        Runtime current;
        current.name.clear();
        current.expected_mode = mode;
        Check(pal4::ScriptSound2d::Play(0x5000u, mode, current) == -1 &&
                  current.handle == 0x4321u &&
                  current.calls == std::vector<std::string>{
                      "name", "executor", "audio", "play", "handle"},
              "nonnegative mode stores current handle and returns minus one");
    }
}

void StopBranches() {
    Runtime inactive;
    inactive.active = 0x100u;
    Check(pal4::ScriptSound2d::StopCurrent(inactive) == 0x100u &&
              inactive.calls == std::vector<std::string>{
                  "executor", "active"},
          "zero low active byte returns full check result");
    Runtime active;
    active.active = 0x101u;
    Check(pal4::ScriptSound2d::StopCurrent(active) == 0x87u &&
              active.calls == std::vector<std::string>{
                  "executor", "active", "executor", "stop"},
          "active sound regets executor then stops current handle");
    for (const auto id : {-1, std::numeric_limits<std::int32_t>::min()}) {
        Runtime runtime;
        Check(pal4::ScriptSound2d::StopId(id, runtime) ==
                  static_cast<std::uint32_t>(id) && runtime.calls.empty(),
              "negative sound ID is original no-op");
    }
    for (const auto id : {0, 17}) {
        Runtime runtime;
        Check(pal4::ScriptSound2d::StopId(id, runtime) ==
                  0x88u + static_cast<std::uint32_t>(id) &&
              runtime.calls == std::vector<std::string>{"executor", "remove"},
              "nonnegative sound ID removed through script tree");
    }
}

}  // namespace

int main() {
    try {
        PlayBranches();
        StopBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
