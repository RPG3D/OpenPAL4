#include "pal4/game_arena_npc_item_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Op = pal4::GameArenaNpcItemCall;
using Callbacks = pal4::GameArenaNpcItemCallbacks;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Read(std::uint32_t address) {
    return "R" + std::to_string(address);
}
std::string Call(Op operation) {
    return "C" + std::to_string(static_cast<unsigned>(operation));
}

struct Invocation {
    Op operation;
    std::vector<std::uint32_t> arguments;
};

struct Runtime final : pal4::IGameArenaNpcItemRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x5010u, 0x8000u}, {0x5194u, 0x9000u},
        {0x61f0u, 1u}, {0x6018u, 0u}, {0x6014u, 0x7000u},
        {0x600cu, 0x55u}, {0xa000u, 0xb000u}};
    std::vector<std::string> events;
    std::vector<Invocation> calls;
    std::int32_t property = 2;
    bool mutateClump = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(Read(address));
        return words[address];
    }
    std::uint32_t Call(Op operation,
                       std::initializer_list<std::uint32_t> arguments) override {
        events.push_back(::Call(operation));
        calls.push_back({operation, arguments});
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        if (operation == Op::GetCharacterProperty) {
            words[0x5194u] = 0x9010u;
            return static_cast<std::uint32_t>(property);
        }
        if (mutateClump && operation == Op::Virtual72)
            words[0x6014u] = 0x7010u;
        if (mutateClump && operation == Op::Virtual76)
            words[0x6014u] = 0x7020u;
        return 0u;
    }
};

void Gates() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::EnterNpc(0x6000u, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{Read(0x5030u)},
          "NPC entry stops at absent world");
    runtime = Runtime{};
    Check(Callbacks::LeaveNpc(0u, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{Read(0x5030u)},
          "NPC exit checks world before null object");
    runtime = Runtime{};
    Check(Callbacks::EnterItem(0u, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{Read(0x5030u)},
          "item entry checks world before null object");
    runtime = Runtime{};
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::LeaveItem(0x6000u, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{Read(0x5030u)},
          "item exit stops at absent world");
}

void Enter(bool npc) {
    Runtime runtime;
    runtime.mutateClump = true;
    const auto result = npc ? Callbacks::EnterNpc(0x6000u, 0x5000u, runtime)
                            : Callbacks::EnterItem(0x6000u, 0x5000u, runtime);
    Check(result == 0x6000u, "entry returns object");
    Check(runtime.events == (std::vector<std::string>{
              Read(0x5030u), Read(0x61f0u), Read(0x6018u),
              Read(0x6014u), Call(Op::AddClump), Read(0x6014u),
              Call(Op::GetMovementAdapter), Read(0xa000u),
              Call(Op::SetClumpProperties), Call(Op::SetAnimationFlag),
              Read(0x5010u), Call(Op::RenderCamera), Read(0x600cu),
              Call(Op::Virtual72), Read(0x6014u),
              Call(Op::ProcessClumpAtomics), Read(0x6014u),
              Call(Op::ProcessAtomics), Read(0x6014u),
              Call(Op::ProcessAtomicsFinal), Read(0x5194u),
              Call(Op::GetCharacterProperty), Read(0x5194u),
              Call(Op::InitializeProperties)}),
          "entry follows exact IDA read and call order");
    Check(runtime.calls[0].arguments ==
              (std::vector<std::uint32_t>{0x5000u, 0x7000u}) &&
              runtime.calls[2].arguments ==
              (std::vector<std::uint32_t>{
                  0xb000u, 0x7000u, 1u, 0u, 0u,
                  npc ? 0x66dcu : 0x66d4u}) &&
              runtime.calls[3].arguments ==
              (std::vector<std::uint32_t>{0x6170u, 1u}) &&
              runtime.calls[4].arguments ==
              (std::vector<std::uint32_t>{0x6000u, 1u, 0x8000u}) &&
              runtime.calls[5].arguments ==
              (std::vector<std::uint32_t>{0x6000u, 0x55u, 0xffffffffu}) &&
              runtime.calls[6].arguments ==
              (std::vector<std::uint32_t>{0x7010u, 0x512cu}) &&
              runtime.calls.back().arguments ==
              (std::vector<std::uint32_t>{0x6000u, 0x9010u}),
          "entry arguments preserve object kind and pointer rereads");

    runtime = Runtime{};
    runtime.words[0x61f0u] = 0u;
    runtime.words[0x5194u] = 0u;
    const auto inactive = npc ? Callbacks::EnterNpc(0x6000u, 0x5000u, runtime)
                              : Callbacks::EnterItem(0x6000u, 0x5000u, runtime);
    Check(inactive == 0x6000u && runtime.calls.empty(),
          "inactive object with missing world property skips both groups");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.property = 0;
    const auto attached = npc ? Callbacks::EnterNpc(0x6000u, 0x5000u, runtime)
                              : Callbacks::EnterItem(0x6000u, 0x5000u, runtime);
    Check(attached == 0x6000u && runtime.calls.size() == 9u &&
              runtime.calls.front().operation == Op::GetMovementAdapter &&
              runtime.calls.back().operation == Op::GetCharacterProperty,
          "attached clump and zero property skip optional work");
}

void Leave(bool npc) {
    Runtime runtime;
    runtime.mutateClump = true;
    const auto result = npc ? Callbacks::LeaveNpc(0x6000u, 0x5000u, runtime)
                            : Callbacks::LeaveItem(0x6000u, 0x5000u, runtime);
    Check(result == 0x6000u, "exit returns object");
    std::vector<std::string> expected{
        Read(0x5030u), Call(Op::CleanupCamera),
        Call(Op::GetObjectProperty), Call(Op::SetStateFlag),
        Call(Op::CleanupObject)};
    if (npc) expected.push_back(Call(Op::Virtual68));
    const std::vector<std::string> tail{
        Call(Op::Virtual76), Read(0x6014u),
        Call(Op::GetMovementAdapter), Read(0xa000u),
        Call(Op::SetClumpRenderCallback), Read(0x6014u),
        Call(Op::ResetAtomicCallbacks), Read(0x6014u),
        Call(Op::ResetMaterialCallbacks), Read(0x6014u),
        Call(Op::ProcessClumpAtomicsWrapper), Read(0x6018u),
        Read(0x6014u), Call(Op::RemoveClump)};
    expected.insert(expected.end(), tail.begin(), tail.end());
    Check(runtime.events == expected, "exit preserves cleanup and read order");
    const auto base = npc ? 5u : 4u;
    Check(runtime.calls[2].arguments ==
              (std::vector<std::uint32_t>{0x6000u, 0u}) &&
              runtime.calls[base + 2u].arguments ==
              (std::vector<std::uint32_t>{0xb000u, 0x7020u}) &&
              runtime.calls[base + 3u].arguments ==
              (std::vector<std::uint32_t>{0x7020u}) &&
              runtime.calls.back().arguments ==
              (std::vector<std::uint32_t>{0x5000u, 0x7020u}),
          "exit callback args use changed clump");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    const auto attached = npc ? Callbacks::LeaveNpc(0x6000u, 0x5000u, runtime)
                              : Callbacks::LeaveItem(0x6000u, 0x5000u, runtime);
    Check(attached == 0x6000u &&
              runtime.calls.back().operation ==
                  Op::ProcessClumpAtomicsWrapper,
          "attached object stays in world");
}

}  // namespace

int main() {
    try {
        Gates();
        Enter(true);
        Enter(false);
        Leave(true);
        Leave(false);
        pal4::GameArenaNpcItemRuntimeStub stub;
        Check(Callbacks::EnterNpc(0u, 0u, stub) == 0u &&
                  Callbacks::LeaveNpc(0u, 0u, stub) == 0u &&
                  Callbacks::EnterItem(0u, 0u, stub) == 0u &&
                  Callbacks::LeaveItem(0u, 0u, stub) == 0u,
              "native runtime stub links");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
