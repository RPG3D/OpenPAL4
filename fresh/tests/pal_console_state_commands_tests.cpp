#include "pal4/pal_console_state_commands.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleStateRuntime {
    std::vector<std::string> calls;
    std::uint32_t written_address = 0;
    std::uint32_t written_value = 99;
    std::int32_t forwarded = 0;
    std::string output;

    std::uint32_t PetManager() override {
        calls.emplace_back("pet");
        return 0x1000u;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        written_address = address;
        written_value = value;
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor");
        return 0x2000u;
    }
    std::int32_t ActivateStatusMode(std::uint32_t actor,
                                    std::int32_t mode) override {
        Check(actor == 0x2000u, "actor control address");
        calls.emplace_back("activate");
        forwarded = mode;
        return 37;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frame");
        return 0x3000u;
    }
    std::int32_t SetTimerFrameDuration(std::uint32_t frame,
                                       std::int32_t duration) override {
        Check(frame == 0x3000u, "UI frame manager address");
        calls.emplace_back("duration");
        forwarded = duration;
        return 43;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x4000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view text) override {
        Check(console == 0x4000u, "console address");
        calls.emplace_back("output");
        output = text;
        return -7;
    }
};

using Command = std::int32_t (*)(const char*, pal4::IPalConsoleStateRuntime&);
constexpr std::array<Command, 3> kCommands{
    pal4::PalConsoleStateCommands::SetPetShow,
    pal4::PalConsoleStateCommands::EnableSceneTransition,
    pal4::PalConsoleStateCommands::ResetUiTimer};

void PetShowClampsAndReturnsManager() {
    constexpr std::array<std::pair<const char*, std::uint32_t>, 6> cases{{
        {"-8", 0u}, {"0", 0u}, {"1", 1u}, {"9", 1u},
        {"garbage", 0u}, {"", 0u}}};
    for (const auto& [input, expected] : cases) {
        Runtime runtime;
        Check(pal4::PalConsoleStateCommands::SetPetShow(input, runtime) ==
                  0x1000 &&
                  runtime.written_address == 0x1654u &&
                  runtime.written_value == expected &&
                  runtime.calls == std::vector<std::string>{"pet", "write"},
              "pet flag clamp, offset and manager result");
    }
}

void NativeForwarding() {
    Runtime scene;
    Check(pal4::PalConsoleStateCommands::EnableSceneTransition(
              "-12", scene) == 37 &&
              scene.forwarded == -12 &&
              scene.calls == std::vector<std::string>{"actor", "activate"},
          "scene transition signed value and result");
    Runtime timer;
    Check(pal4::PalConsoleStateCommands::ResetUiTimer(
              "42", timer) == 43 &&
              timer.forwarded == 42 &&
              timer.calls == std::vector<std::string>{"frame", "duration"},
          "UI timer signed value and result");
    Runtime nonnumeric;
    Check(pal4::PalConsoleStateCommands::ResetUiTimer(
              "bad", nonnumeric) == 43 && nonnumeric.forwarded == 0,
          "atoi nonnumeric zero");
}

void MissingArguments() {
    for (const auto command : kCommands) {
        Runtime runtime;
        Check(command(nullptr, runtime) == -7 &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"},
              "missing input skips native subsystem");
    }
}

}  // namespace

int main() {
    try {
        PetShowClampsAndReturnsManager();
        NativeForwarding();
        MissingArguments();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
