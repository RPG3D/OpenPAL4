#include "pal4/script_minimap_level.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptMinimapLevelRuntime {
    std::vector<std::string> calls;
    std::uint32_t manager = 0x1000u;
    std::uint32_t index = 2u;
    std::uint32_t level = 0xffffffffu;
    std::uint32_t minimap_calls = 0u;

    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap");
        return 0x3000u + minimap_calls++;
    }
    void SetTexture(std::uint32_t minimap,
                    std::uint32_t value) override {
        Check(minimap == 0x3000u && value == level,
              "minimap texture operands");
        calls.emplace_back("texture");
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("players");
        return manager;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == manager + 0x2c50u,
              "current player index offset");
        calls.emplace_back("index");
        return index;
    }
    void UpdateGrid(std::uint32_t minimap,
                    std::uint32_t direction,
                    std::uint32_t position) override {
        const auto player = manager + index * 0xb14u;
        Check(minimap == 0x3001u && direction == player + 0xacu &&
                  position == player + 0xa0u,
              "grid direction and position operands");
        calls.emplace_back("grid");
    }
};

void NormalAndZero() {
    Runtime normal;
    pal4::ScriptMinimapLevel::Set(normal.level, normal);
    Check(normal.calls == std::vector<std::string>{
              "minimap", "texture", "players", "index", "minimap", "grid"},
          "texture precedes player-based grid refresh");

    Runtime zero;
    zero.manager = 0u;
    zero.index = 0u;
    pal4::ScriptMinimapLevel::Set(zero.level, zero);
    Check(zero.calls == std::vector<std::string>{
              "minimap", "texture", "players", "index"},
          "zero computed player skips second minimap access");

    Runtime zero_manager_nonzero_index;
    zero_manager_nonzero_index.manager = 0u;
    zero_manager_nonzero_index.index = 1u;
    pal4::ScriptMinimapLevel::Set(
        zero_manager_nonzero_index.level, zero_manager_nonzero_index);
    Check(zero_manager_nonzero_index.calls.back() == "grid",
          "gate checks computed player rather than manager");
}

}  // namespace

int main() {
    try {
        NormalAndZero();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
