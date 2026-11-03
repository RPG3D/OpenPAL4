#include "pal4/game_arena_player_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Op = pal4::GameArenaPlayerCall;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string R(std::uint32_t address) {
    return "R" + std::to_string(address);
}
std::string C(Op operation) {
    return "C" + std::to_string(static_cast<unsigned>(operation));
}
std::string W(std::uint32_t address) {
    return "W" + std::to_string(address);
}

struct Invocation {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t third;
};

struct Runtime final : pal4::IGameArenaPlayerRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x61f0u, 1u}, {0x6018u, 0u},
        {0x6014u, 0x7000u}, {0x5010u, 0x8000u},
        {0x5194u, 0x9000u}, {0x6b10u, 1u},
        {0xa000u, 0xb000u}};
    std::vector<std::string> events;
    std::vector<Invocation> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::int32_t property = 2;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(R(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back(W(address));
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        events.push_back(C(operation));
        calls.push_back({operation, first, second, third});
        if (operation == Op::GetCharacterProperty) {
            words[0x5194u] = 0x9010u;
            return static_cast<std::uint32_t>(property);
        }
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        if (operation == Op::RemoveClump) words[0x6014u] = 0x7010u;
        return 0u;
    }
};

void Gate() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(pal4::GameArenaPlayerCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "enter stops when world is absent");
    runtime = Runtime{};
    Check(pal4::GameArenaPlayerCallbacks::Leave(
              0u, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "leave stops on null player after world read");
}

void Enter() {
    Runtime runtime;
    Check(pal4::GameArenaPlayerCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), R(0x61f0u), R(0x6018u), R(0x6014u),
                  C(Op::AddClump), C(Op::SetAnimationFlag), R(0x5010u),
                  C(Op::RenderCamera), R(0x6014u),
                  C(Op::ProcessClumpAtomics), R(0x5194u),
                  C(Op::GetCharacterProperty), R(0x5194u),
                  C(Op::InitializeProperties)}),
          "enter callback follows exact IDA reads and calls");
    Check(runtime.calls.size() == 6u &&
              runtime.calls[0].first == 0x5000u &&
              runtime.calls[0].second == 0x7000u &&
              runtime.calls[1].first == 0x6170u &&
              runtime.calls[1].second == 1u &&
              runtime.calls[2].first == 0x6000u &&
              runtime.calls[2].second == 1u &&
              runtime.calls[2].third == 0x8000u &&
              runtime.calls[3].first == 0x7000u &&
              runtime.calls[3].second == 0x512cu &&
              runtime.calls[5].second == 0x9010u,
          "enter service offsets and property pointer reread");

    runtime = Runtime{};
    runtime.words[0x61f0u] = 0u;
    runtime.words[0x5194u] = 0u;
    Check(pal4::GameArenaPlayerCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.calls.empty(),
          "inactive player and absent property skip both work groups");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.property = -1;
    Check(pal4::GameArenaPlayerCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.calls.size() == 4u &&
              runtime.calls[0].operation == Op::SetAnimationFlag &&
              runtime.calls.back().operation == Op::GetCharacterProperty,
          "existing clump and nonpositive property skip optional calls");
}

void Leave() {
    Runtime runtime;
    Check(pal4::GameArenaPlayerCallbacks::Leave(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::CleanupCamera), C(Op::GetObjectProperty),
                  C(Op::SetStateFlag), C(Op::CleanupObject), R(0x6018u),
                  R(0x6014u), C(Op::RemoveClump), R(0x6b10u),
                  R(0x6014u), C(Op::GetMovementAdapter), R(0xa000u),
                  C(Op::SetClumpRenderCallback), W(0x6b10u),
                  R(0x6014u), C(Op::ProcessClumpAtomicsWrapper)}),
          "leave callback preserves cleanup and pointer reread order");
    Check(runtime.calls[2].first == 0x6000u &&
              runtime.calls[2].second == 0u &&
              runtime.calls[4].second == 0x7000u &&
              runtime.calls[6].first == 0xb000u &&
              runtime.calls[6].second == 0x7010u &&
              runtime.calls.back().first == 0x7010u &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x6b10u, 0u}}),
          "leave callback arguments, changed clump and callback flag reset");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.words[0x6b10u] = 0u;
    Check(pal4::GameArenaPlayerCallbacks::Leave(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.calls.size() == 5u && runtime.writes.empty() &&
              runtime.calls.back().operation ==
                  Op::ProcessClumpAtomicsWrapper,
          "attached clump and unset callback flag skip optional cleanup");

    pal4::GameArenaPlayerRuntimeStub stub;
    Check(pal4::GameArenaPlayerCallbacks::Enter(0u, 0u, stub) == 0u &&
              pal4::GameArenaPlayerCallbacks::Leave(0u, 0u, stub) == 0u,
          "native player runtime stub links and exits safely");
}

}  // namespace

int main() {
    try {
        Gate();
        Enter();
        Leave();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
