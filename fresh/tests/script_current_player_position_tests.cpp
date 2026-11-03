#include "pal4/script_current_player_position.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCurrentPlayerPositionRuntime {
    std::uint32_t manager = 0x1000u;
    std::uint32_t index = 2u;
    std::uint32_t expected_offset = 0xacu;
    float value = -1.25f;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == manager + 0x2c50u, "current player index offset");
        calls.emplace_back("index");
        return index;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == manager + index * 0xb14u + expected_offset,
              "current player stride and axis offset");
        calls.emplace_back("coordinate");
        return value;
    }
};

void AxisBranches() {
    const auto query_x = [](Runtime& runtime) {
        return pal4::ScriptCurrentPlayerPosition::X(runtime);
    };
    const auto query_y = [](Runtime& runtime) {
        return pal4::ScriptCurrentPlayerPosition::Y(runtime);
    };
    const auto query_z = [](Runtime& runtime) {
        return pal4::ScriptCurrentPlayerPosition::Z(runtime);
    };
    const std::array<std::pair<std::uint32_t, double (*)(Runtime&)>, 3> axes{{
        {0xacu, query_x}, {0xb0u, query_y}, {0xb4u, query_z}}};
    for (const auto [offset, query] : axes) {
        Runtime runtime;
        runtime.expected_offset = offset;
        Check(query(runtime) == static_cast<double>(runtime.value) &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "index", "coordinate"},
              "each axis reads exact float from current player");
        Runtime zero;
        zero.manager = 0u;
        zero.index = 0u;
        Check(query(zero) == 0.0 &&
                  zero.calls == std::vector<std::string>{
                      "manager", "index"},
              "zero computed player returns zero without float read");
    }
}

}  // namespace

int main() {
    try {
        AxisBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
