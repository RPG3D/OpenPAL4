#include "pal4/game_arena_string_matcher.h"

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

struct Runtime final : pal4::IGameArenaStringRuntime {
    std::string arena = "M15";
    std::string area = "Q14B";
    std::vector<std::string> calls;
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x2000u;
    }
    std::string_view ReadText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        if (address == 0x20f0u) return arena;
        if (address == 0x20fcu) return area;
        throw std::runtime_error("unexpected string address");
    }
};

void Cases() {
    Runtime runtime;
    pal4::CrtExceptionFrameNoOp::Run();
    Check(pal4::GameArenaStringMatcher::Matches(
              {"m15", 0}, {"ignored", 0}, runtime) &&
          runtime.calls == std::vector<std::string>{
              "arena-manager", "text:8432"},
          "case-insensitive arena match omits area when flag clear");

    runtime.calls.clear();
    Check(!pal4::GameArenaStringMatcher::Matches(
              {"other", 0}, {"Q14B", 1}, runtime) &&
          runtime.calls == std::vector<std::string>{
              "arena-manager", "text:8432"},
          "arena mismatch short-circuits area lookup");

    runtime.calls.clear();
    Check(pal4::GameArenaStringMatcher::Matches(
              {"M15", 0}, {"q14b", 7}, runtime) &&
          runtime.calls == std::vector<std::string>{
              "arena-manager", "text:8432", "arena-manager", "text:8444"},
          "nonzero area flag compares second native field");

    runtime.calls.clear();
    Check(!pal4::GameArenaStringMatcher::Matches(
              {"M15", 0}, {"other", 1}, runtime),
          "area mismatch returns false");
}

void NullPointers() {
    Runtime runtime;
    runtime.arena.clear();
    runtime.area.clear();
    Check(pal4::GameArenaStringMatcher::Matches(
              {nullptr, 1}, {nullptr, 1}, runtime),
          "null native strings use empty text");
    runtime.area = "nonempty";
    Check(!pal4::GameArenaStringMatcher::Matches(
              {nullptr, 0}, {nullptr, 1}, runtime),
          "null area does not match nonempty value");
}

}  // namespace

int main() {
    try {
        Cases();
        NullPointers();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
