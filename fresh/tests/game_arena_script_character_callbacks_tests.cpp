#include "pal4/game_arena_script_character_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Op = pal4::GameArenaScriptCharacterCall;
using Callbacks = pal4::GameArenaScriptCharacterCallbacks;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::string R(std::uint32_t address) {
    return "R" + std::to_string(address);
}
std::string C(Op operation) {
    return "C" + std::to_string(static_cast<unsigned>(operation));
}

struct Invocation {
    Op operation;
    std::vector<std::uint32_t> arguments;
};

struct Runtime final : pal4::IGameArenaScriptCharacterRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x5010u, 0x8500u},
        {0x5194u, 0x9000u}, {0x61f0u, 1u},
        {0x6018u, 0u}, {0x6014u, 0x8000u},
        {0x600cu, 0x55u}, {0xa000u, 0xb000u}};
    std::vector<std::string> events;
    std::vector<Invocation> calls;
    std::int32_t property = 2;
    bool mutate = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(R(address));
        return words[address];
    }
    std::uint32_t Call(Op operation,
                       std::initializer_list<std::uint32_t> args) override {
        events.push_back(C(operation));
        calls.push_back({operation, args});
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        if (operation == Op::GetCharacterProperty) {
            words[0x5194u] = 0x9010u;
            return static_cast<std::uint32_t>(property);
        }
        if (mutate && operation == Op::AddClump)
            words[0x6014u] = 0x8010u;
        if (mutate && operation == Op::RemoveClump)
            words[0x6014u] = 0x8020u;
        return 0u;
    }
};

void Gates() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::Enter(0x5000u, 0x6000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "entry stops when world is absent");
    runtime = Runtime{};
    Check(Callbacks::LeaveWithVirtual68(0x5000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "extra-virtual exit checks world before null character");
    runtime = Runtime{};
    Check(Callbacks::Leave(0x5000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "ordinary exit checks world before null character");
}

void Enter() {
    Runtime runtime;
    runtime.mutate = true;
    Check(Callbacks::Enter(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), R(0x61f0u), R(0x6018u),
                  R(0x6014u), C(Op::AddClump), R(0x6014u),
                  C(Op::GetMovementAdapter), R(0xa000u),
                  C(Op::SetClumpProperties),
                  C(Op::SetAnimationFlag), R(0x5010u),
                  C(Op::RenderCamera), R(0x600cu),
                  C(Op::Virtual72), R(0x6014u),
                  C(Op::ProcessClumpAtomics), R(0x6014u),
                  C(Op::ProcessAtomics), R(0x6014u),
                  C(Op::ProcessAtomicsFinal), R(0x5194u),
                  C(Op::GetCharacterProperty), R(0x5194u),
                  C(Op::InitializeProperties)}),
          "script character entry preserves IDA call and read order");
    Check(runtime.calls[0].arguments ==
              (std::vector<std::uint32_t>{0x5000u, 0x8000u}) &&
              runtime.calls[2].arguments ==
              (std::vector<std::uint32_t>{
                  0xb000u, 0x8010u, 1u, 0u, 0u, 0u}) &&
              runtime.calls[5].arguments ==
              (std::vector<std::uint32_t>{
                  0x6000u, 0x55u, 0xffffffffu}) &&
              runtime.calls[6].arguments ==
              (std::vector<std::uint32_t>{0x8010u, 0x512cu}) &&
              runtime.calls.back().arguments ==
              (std::vector<std::uint32_t>{0x6000u, 0x9010u}),
          "entry forwards all clump, virtual and property arguments");

    runtime = Runtime{};
    runtime.words[0x61f0u] = 0u;
    runtime.words[0x5194u] = 0u;
    Check(Callbacks::Enter(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.empty(),
          "inactive character and missing world property do no setup");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.property = -1;
    Check(Callbacks::Enter(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.size() == 9u &&
              runtime.calls.front().operation == Op::GetMovementAdapter &&
              runtime.calls.back().operation == Op::GetCharacterProperty,
          "attached clump and negative property skip optional calls");
}

void Leave(bool extraVirtual) {
    Runtime runtime;
    runtime.mutate = true;
    const auto result = extraVirtual
        ? Callbacks::LeaveWithVirtual68(0x5000u, 0x6000u, runtime)
        : Callbacks::Leave(0x5000u, 0x6000u, runtime);
    Check(result == 1u, "script character exit returns one");
    std::vector<std::string> expected{
        R(0x5030u), C(Op::CleanupCamera),
        C(Op::GetObjectProperty), C(Op::SetStateFlag),
        C(Op::CleanupObject)};
    if (extraVirtual) expected.push_back(C(Op::Virtual68));
    const std::vector<std::string> tail{
        C(Op::Virtual76), R(0x6014u),
        C(Op::GetMovementAdapter), R(0xa000u),
        C(Op::SetClumpRenderCallback), R(0x6018u),
        R(0x6014u), C(Op::RemoveClump),
        R(0x6014u), C(Op::ResetAtomicCallbacks),
        R(0x6014u), C(Op::ResetMaterialCallbacks),
        R(0x6014u), C(Op::ProcessClumpAtomicsWrapper)};
    expected.insert(expected.end(), tail.begin(), tail.end());
    Check(runtime.events == expected,
          "exit preserves virtual difference and clump cleanup order");
    const auto offset = extraVirtual ? 1u : 0u;
    Check(runtime.calls[2].arguments ==
              (std::vector<std::uint32_t>{0x6000u, 0u}) &&
              runtime.calls[6u + offset].operation ==
                  Op::SetClumpRenderCallback &&
              runtime.calls[6u + offset].arguments ==
                  (std::vector<std::uint32_t>{0xb000u, 0x8000u}) &&
              runtime.calls[7u + offset].arguments ==
                  (std::vector<std::uint32_t>{0x5000u, 0x8000u}) &&
              runtime.calls[8u + offset].arguments ==
                  (std::vector<std::uint32_t>{0x8020u}) &&
              runtime.calls.back().arguments ==
                  (std::vector<std::uint32_t>{0x8020u}),
          "exit arguments include changed clump after world removal");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    const auto attached = extraVirtual
        ? Callbacks::LeaveWithVirtual68(0x5000u, 0x6000u, runtime)
        : Callbacks::Leave(0x5000u, 0x6000u, runtime);
    Check(attached == 1u &&
              runtime.calls.back().operation ==
                  Op::ProcessClumpAtomicsWrapper &&
              runtime.calls.size() == (extraVirtual ? 11u : 10u),
          "attached clump skips only world removal");
}

}  // namespace

int main() {
    try {
        Gates();
        Enter();
        Leave(true);
        Leave(false);
        pal4::GameArenaScriptCharacterRuntimeStub stub;
        Check(Callbacks::Enter(0u, 0u, stub) == 0u &&
                  Callbacks::LeaveWithVirtual68(0u, 0u, stub) == 0u &&
                  Callbacks::Leave(0u, 0u, stub) == 0u,
              "native script character runtime stub links");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
