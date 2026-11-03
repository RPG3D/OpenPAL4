#include "pal4/script_ui_state_callbacks.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptWorldMapStateRuntime,
                       pal4::IScriptPuzzleResultRuntime,
                       pal4::IScriptAlwaysJumpRuntime {
    std::uint8_t puzzle = 0u;
    std::uint32_t written = 0xffffffffu;
    std::vector<std::string> calls;

    std::uint32_t SetExternalButtonVisibility(
        std::uint32_t name, std::int32_t state) override {
        Check(name == 0x5000u && state == -2,
              "world map thunk forwards both arguments");
        calls.emplace_back("world map"); return 0x81u;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames"); return 0x2000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x2222u, "puzzle result byte offset");
        calls.emplace_back("puzzle"); return puzzle;
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor"); return 0x3000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x3110u, "always jump word offset");
        calls.emplace_back("write"); written = value;
    }
};

void WorldMapPuzzleAndJump() {
    Runtime runtime;
    Check(pal4::ScriptWorldMapState::Set(0x5000u, -2, runtime) == 0x81u &&
              runtime.calls == std::vector<std::string>{"world map"},
          "world map forwards native return");
    for (const auto value : {0u, 1u, 255u}) {
        runtime.calls.clear();
        runtime.puzzle = static_cast<std::uint8_t>(value);
        Check(pal4::ScriptPuzzleResult::Get(runtime) == value &&
                  runtime.calls == std::vector<std::string>{"frames", "puzzle"},
              "puzzle byte is zero extended");
    }
    for (const auto value : {0u, 1u, 0x1234u}) {
        runtime.calls.clear();
        Check(pal4::ScriptAlwaysJump::Set(value, runtime) == 0x3000u &&
                  runtime.written == (value & 0xffu) &&
                  runtime.calls == std::vector<std::string>{"actor", "write"},
              "always jump writes low byte as dword and returns actor");
    }
}

}  // namespace

int main() {
    try {
        WorldMapPuzzleAndJump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
