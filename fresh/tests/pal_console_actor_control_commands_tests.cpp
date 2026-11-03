#include "pal4/pal_console_actor_control_commands.h"

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

struct Runtime final : pal4::IPalConsoleActorControlRuntime {
    std::vector<std::string> calls;
    std::string output;
    std::uint32_t word28 = 99u;

    std::uint32_t ActorControl() override {
        calls.emplace_back("actor control");
        return 0x1000u;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        Check(address == 0x1110u && value == 1u,
              "jump unlock field +0x110");
        calls.emplace_back("write jump");
    }
    std::int32_t SetWord28(std::uint32_t control,
                           std::uint32_t value) override {
        Check(control == 0x1000u, "control mode actor");
        calls.emplace_back("set word 28");
        word28 = value;
        return 29;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x2000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x2000u, "actor error console");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

void Commands() {
    Runtime jump;
    Check(pal4::PalConsoleActorControlCommands::JumpUnlock(jump) ==
              0x1000u &&
              jump.calls ==
                  std::vector<std::string>{"actor control", "write jump"},
          "jump unlock writes actor field and returns actor");
    for (const auto [input, expected] :
         {std::pair{"1", 0u}, std::pair{"0", 1u},
          std::pair{"-1", 1u}, std::pair{"text", 1u},
          std::pair{"", 1u}}) {
        Runtime runtime;
        Check(pal4::PalConsoleActorControlCommands::SetControlMode(
                  input, runtime) == 29 &&
                  runtime.word28 == expected &&
                  runtime.calls == std::vector<std::string>{
                      "actor control", "set word 28"},
              "control mode maps only atoi one to zero");
    }
    Runtime missing;
    Check(pal4::PalConsoleActorControlCommands::SetControlMode(
              nullptr, missing) == -7 &&
              missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              missing.calls == std::vector<std::string>{"console", "output"},
          "control mode missing parameter");
}

}  // namespace

int main() {
    try {
        Commands();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
