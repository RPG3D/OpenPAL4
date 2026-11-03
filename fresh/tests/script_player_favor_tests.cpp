#include "pal4/script_player_favor.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerFavorRuntime {
    std::vector<std::string> calls;
    std::int32_t first_index = -1;
    std::int32_t second_index = 3;
    std::uint32_t first = 0x5000u;
    std::uint32_t second = 0x6000u;
    std::uint32_t role = 2u;
    std::uint32_t current = 5u;
    std::uint32_t written = 0xffffffffu;
    std::uint32_t managers = 0u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u + managers++;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        if (manager == 0x1000u) {
            Check(index == first_index, "first signed player index");
            calls.emplace_back("first");
            return first;
        }
        Check(manager == 0x1001u && index == second_index,
              "second signed player index");
        calls.emplace_back("second");
        return second;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == second + 0xa48u) {
            calls.emplace_back("role");
            return role;
        }
        Check(address == first + 0xa38u + role * 4u,
              "relation slot address");
        calls.emplace_back("favor");
        return current;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == first + 0xa38u + role * 4u,
              "relation write address");
        calls.emplace_back("write");
        written = value;
    }
};

void GatesAndSlots() {
    Runtime missing_first;
    missing_first.first = 0u;
    Check(pal4::ScriptPlayerFavor::Add(-1, 3, 2, missing_first) == 0u &&
              missing_first.calls == std::vector<std::string>{
                  "manager", "first"},
          "first lookup miss stops second lookup");

    Runtime missing_second;
    missing_second.second = 0u;
    Check(pal4::ScriptPlayerFavor::Add(-1, 3, 2, missing_second) == 0u &&
              missing_second.calls == std::vector<std::string>{
                  "manager", "first", "manager", "second"},
          "second lookup miss stops role access");

    for (const auto role : {4u, 0xffffffffu}) {
        Runtime invalid;
        invalid.role = role;
        Check(pal4::ScriptPlayerFavor::Add(-1, 3, 2, invalid) == role &&
                  invalid.calls == std::vector<std::string>{
                      "manager", "first", "manager", "second", "role"} &&
                  invalid.written == 0xffffffffu,
              "invalid signed role returns raw value without relation access");
    }

    for (std::uint32_t role = 0u; role < 4u; ++role) {
        Runtime valid;
        valid.role = role;
        Check(pal4::ScriptPlayerFavor::Add(-1, 3, 2, valid) == role &&
                  valid.written == 7u &&
                  valid.calls == std::vector<std::string>{
                      "manager", "first", "manager", "second", "role",
                      "favor", "write"},
              "all four relation slots and call order");
    }
}

void Arithmetic() {
    Runtime negative;
    Check(pal4::ScriptPlayerFavor::Add(-1, 3, -9, negative) == 2u &&
              negative.written == 0u,
          "negative relation clamps to zero");
    Runtime high;
    high.current = 0x7ffffff0u;
    Check(pal4::ScriptPlayerFavor::Add(-1, 3, 15, high) == 2u &&
              high.written == 0x7fffffffu,
          "positive relation has no upper clamp");
    Runtime wrapped;
    wrapped.current = 0x7fffffffu;
    Check(pal4::ScriptPlayerFavor::Add(-1, 3, 1, wrapped) == 2u &&
              wrapped.written == 0u,
          "32-bit signed overflow follows sign-bit clamp");
}

}  // namespace

int main() {
    try {
        GatesAndSlots();
        Arithmetic();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
