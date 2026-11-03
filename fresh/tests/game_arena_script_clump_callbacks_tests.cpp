#include "pal4/game_arena_script_clump_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Op = pal4::GameArenaScriptClumpCall;
using Callbacks = pal4::GameArenaScriptClumpCallbacks;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::string R(std::uint32_t address) {
    return "R" + std::to_string(address);
}
std::string W(std::uint32_t address) {
    return "W" + std::to_string(address);
}
std::string C(Op operation) {
    return "C" + std::to_string(static_cast<unsigned>(operation));
}

struct Invocation {
    Op operation;
    std::vector<std::uint32_t> arguments;
};

struct Runtime final : pal4::IGameArenaScriptClumpRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x6044u, 0x8000u}, {0x6004u, 0u},
        {0x6040u, 0x9000u}, {0xa000u, 0xb000u},
        {0xa004u, 0xb010u}, {0x6184u, 1u},
        {0x6170u, 1u}, {0x6018u, 0u},
        {0x6014u, 0x8100u}, {0x61a8u, 1u}};
    std::vector<std::string> events;
    std::vector<Invocation> calls;
    bool mutate = false;
    std::uint32_t virtual140 = 1u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(R(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back(W(address));
        words[address] = value;
    }
    std::uint32_t Call(Op operation,
                       std::initializer_list<std::uint32_t> args) override {
        events.push_back(C(operation));
        calls.push_back({operation, args});
        if (operation == Op::GetCameraSubsystem) return 0xc000u;
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        if (operation == Op::GetResourceProperty)
            return args.begin()[0] == 0x9000u ? 11u : 12u;
        if (operation == Op::Virtual140) return virtual140;
        if (mutate && operation == Op::SetClumpProperties) {
            words[0x6040u] = 0x9010u;
            words[0x6044u] = 0x8010u;
        }
        if (mutate && operation == Op::RestoreClumpRenderCallback)
            words[0x6044u] = 0x8010u;
        if (mutate && operation == Op::AddClump)
            words[0x6014u] = 0x8110u;
        if (mutate && operation == Op::RemoveClump)
            words[0x6014u] = 0x8120u;
        return 0u;
    }
};

void Gates() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::ActivateClump(0x5000u, 0x6000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "clump activation stops at missing world");
    runtime = Runtime{};
    Check(Callbacks::DeactivateClump(0x5000u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{R(0x5030u)},
          "clump deactivation checks world before null object");
    runtime = Runtime{};
    runtime.words[0x6044u] = 0u;
    Check(Callbacks::ActivateClump(0x5000u, 0x6000u, runtime) == 0u &&
              runtime.words[0x604cu] == 0u,
          "activation requires non-null clump");
    runtime = Runtime{};
    runtime.words[0x6044u] = 0u;
    Check(Callbacks::DeactivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), R(0x6044u)}),
          "deactivation accepts null clump without writes");
    runtime = Runtime{};
    runtime.words[0x5030u] = 0u;
    Check(Callbacks::AddAnimatedObject(0x5000u, 0x6000u, runtime) == 0u &&
              Callbacks::RemoveAnimatedObject(0x5000u, 0x6000u,
                                              runtime) == 0u,
          "animated callbacks require active world");
}

void Clump() {
    Runtime runtime;
    runtime.mutate = true;
    Check(Callbacks::ActivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), R(0x6044u), R(0x6004u), W(0x604cu),
                  R(0x6040u), C(Op::GetMovementAdapter),
                  R(0xa000u), C(Op::GetResourceProperty),
                  C(Op::SetClumpProperties), R(0x6004u),
                  R(0x6040u), R(0x6044u),
                  C(Op::GetMovementAdapter), R(0xa004u),
                  C(Op::GetResourceProperty),
                  C(Op::SetClumpProperties)}),
          "type-zero activation configures both adapter slots in IDA order");
    Check(runtime.words[0x604cu] == 1u &&
              runtime.calls[1].arguments ==
                  (std::vector<std::uint32_t>{0x9000u}) &&
              runtime.calls[2].arguments ==
                  (std::vector<std::uint32_t>{
                      0xb000u, 0x8000u, 11u, 1u, 1u, 0x9000u}) &&
              runtime.calls.back().arguments ==
                  (std::vector<std::uint32_t>{
                      0xb010u, 0x8010u, 12u, 1u, 1u, 0x9010u}),
          "six-argument properties retain resource and clump rereads");

    runtime = Runtime{};
    runtime.words[0x6004u] = 4u;
    Check(Callbacks::ActivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.size() == 2u &&
              runtime.calls[0].operation == Op::GetCameraSubsystem &&
              runtime.calls[1].arguments ==
                  (std::vector<std::uint32_t>{0xc000u, 0x8000u}),
          "type four adds clump to camera subsystem");
    runtime = Runtime{};
    runtime.words[0x6004u] = 5u;
    Check(Callbacks::ActivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.empty() && runtime.words[0x604cu] == 1u,
          "type five only sets active flag");

    runtime = Runtime{};
    runtime.mutate = true;
    Check(Callbacks::DeactivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), R(0x6044u), R(0x6004u), W(0x604cu),
                  C(Op::GetMovementAdapter), R(0xa000u),
                  C(Op::RestoreClumpRenderCallback), R(0x6004u),
                  R(0x6044u), C(Op::GetMovementAdapter),
                  R(0xa004u), C(Op::RestoreClumpRenderCallback)}),
          "type-zero deactivation restores both callbacks in order");
    Check(runtime.words[0x604cu] == 0u &&
              runtime.calls[1].arguments ==
                  (std::vector<std::uint32_t>{0xb000u, 0x8000u}) &&
              runtime.calls.back().arguments ==
                  (std::vector<std::uint32_t>{0xb010u, 0x8010u}),
          "deactivation rereads clump for second adapter");
    runtime = Runtime{};
    runtime.words[0x6004u] = 4u;
    Check(Callbacks::DeactivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.back().operation == Op::RemoveCameraClump,
          "type four removes camera clump");
    runtime = Runtime{};
    runtime.words[0x6004u] = 5u;
    Check(Callbacks::DeactivateClump(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.empty(),
          "type five only clears active flag");
}

void Animated() {
    Runtime runtime;
    runtime.mutate = true;
    Check(Callbacks::AddAnimatedObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::CleanupObject), R(0x6184u),
                  R(0x6170u), R(0x6018u), R(0x6014u),
                  C(Op::AddClump), C(Op::Virtual72), R(0x6014u),
                  C(Op::ProcessClumpAtomics), R(0x61a8u),
                  R(0x6014u), C(Op::ProcessAtomics), R(0x6014u),
                  C(Op::ProcessAtomicsFinal), C(Op::Virtual140),
                  R(0x6014u), C(Op::GetCameraSubsystem),
                  C(Op::AddCameraClump)}),
          "animated add follows full clump/material/camera path");
    Check(runtime.calls[1].arguments ==
              (std::vector<std::uint32_t>{0x5000u, 0x8100u}) &&
              runtime.calls[3].arguments ==
              (std::vector<std::uint32_t>{0x8110u, 0x512cu}) &&
              runtime.calls.back().arguments ==
              (std::vector<std::uint32_t>{0xc000u, 0x8110u}),
          "animated add uses changed clump for later atomics and camera");

    runtime = Runtime{};
    runtime.words[0x6184u] = 0u;
    Check(Callbacks::AddAnimatedObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::CleanupObject), R(0x6184u)}),
          "inactive animation stops after cleanup without fallback call");
    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.words[0x61a8u] = 0u;
    runtime.virtual140 = 0u;
    Check(Callbacks::AddAnimatedObject(0x5000u, 0x6000u, runtime) == 1u &&
              runtime.calls.size() == 5u &&
              runtime.calls.back().operation == Op::Virtual140,
          "attached clump and absent material/camera skip optional add work");

    runtime = Runtime{};
    runtime.mutate = true;
    Check(Callbacks::RemoveAnimatedObject(0x5000u, 0x6000u,
                                          runtime) == 1u &&
              runtime.events == (std::vector<std::string>{
                  R(0x5030u), C(Op::Virtual156),
                  C(Op::CleanupObject), C(Op::Virtual76),
                  R(0x6018u), R(0x6014u), C(Op::RemoveClump),
                  R(0x6014u), C(Op::ResetAtomicCallbacks),
                  R(0x61a8u), R(0x6014u),
                  C(Op::ResetMaterialCallbacks), R(0x6014u),
                  C(Op::ProcessClumpAtomicsWrapper),
                  C(Op::Virtual140), R(0x6014u),
                  C(Op::GetCameraSubsystem), C(Op::RemoveCameraClump)}),
          "animated remove preserves cleanup and pointer reread order");
    Check(runtime.calls[0].arguments ==
              (std::vector<std::uint32_t>{0x6000u, 0u}) &&
              runtime.calls[3].arguments ==
              (std::vector<std::uint32_t>{0x5000u, 0x8100u}) &&
              runtime.calls[4].arguments ==
              (std::vector<std::uint32_t>{0x8120u}) &&
              runtime.calls.back().arguments ==
              (std::vector<std::uint32_t>{0xc000u, 0x8120u}),
          "animated removal uses changed clump after world removal");
    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.words[0x61a8u] = 0u;
    runtime.virtual140 = 0u;
    Check(Callbacks::RemoveAnimatedObject(0x5000u, 0x6000u,
                                          runtime) == 1u &&
              runtime.calls.back().operation == Op::Virtual140 &&
              runtime.calls.size() == 6u,
          "attached object skips clump removal and material/camera cleanup");

    pal4::GameArenaScriptClumpRuntimeStub stub;
    Check(Callbacks::ActivateClump(0u, 0u, stub) == 0u &&
              Callbacks::DeactivateClump(0u, 0u, stub) == 0u &&
              Callbacks::AddAnimatedObject(0u, 0u, stub) == 0u &&
              Callbacks::RemoveAnimatedObject(0u, 0u, stub) == 0u,
          "native clump callback stub links");
}

}  // namespace

int main() {
    try {
        Gates();
        Clump();
        Animated();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
