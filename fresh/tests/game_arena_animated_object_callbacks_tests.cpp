#include "pal4/game_arena_animated_object_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

using Op = pal4::GameArenaAnimatedObjectCall;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Invocation {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
};

struct Runtime final : pal4::IGameArenaAnimatedObjectRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x6184u, 1u}, {0x6170u, 1u},
        {0x6018u, 0u}, {0x6014u, 0x7000u},
        {0x61a8u, 1u}, {0x6174u, 0u}};
    std::vector<Invocation> calls;
    std::vector<std::uint32_t> reads;
    bool camera_member = true;
    bool mutate_clump = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second) override {
        calls.push_back({operation, first, second});
        if (operation == Op::Virtual140) return camera_member ? 1u : 0u;
        if (operation == Op::GetCameraSubsystem) return 0x9000u;
        if (mutate_clump) {
            if (operation == Op::ProcessAtomics ||
                operation == Op::ResetAtomicCallbacks)
                words[0x6014u] = 0x7010u;
            if (operation == Op::ProcessAtomicsOptional ||
                operation == Op::ResetMaterialCallbacks)
                words[0x6014u] = 0x7020u;
            if (operation == Op::ProcessAtomicsFinal ||
                operation == Op::ProcessAtomicsWrapper)
                words[0x6014u] = 0x7030u;
            if (operation == Op::RemoveClump)
                words[0x6014u] = 0x7040u;
        }
        return 0u;
    }
};

std::vector<Op> Operations(const Runtime& runtime) {
    std::vector<Op> result;
    for (const auto& call : runtime.calls) result.push_back(call.operation);
    return result;
}

void GatesAndFallback() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0u && runtime.calls.empty(),
          "enter stops without world");
    runtime = Runtime{};
    Check(pal4::GameArenaAnimatedObjectCallbacks::Leave(
              0u, 0x5000u, runtime) == 0u && runtime.calls.empty(),
          "leave stops without object");

    runtime = Runtime{};
    runtime.words[0x6184u] = 0u;
    runtime.words[0x6174u] = 1u;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == std::vector<Op>{Op::CleanupObject} &&
              runtime.reads.back() == 0x6170u,
          "inactive object with animation flag exits without fallback call");

    runtime = Runtime{};
    runtime.words[0x6170u] = 0u;
    runtime.words[0x6174u] = 1u;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::CleanupObject, Op::Virtual72}),
          "inactive animation uses optional fallback virtual call");
}

void EnterFull() {
    Runtime runtime;
    runtime.mutate_clump = true;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::CleanupObject, Op::AddClump, Op::Virtual72,
                  Op::ProcessAtomics, Op::ProcessAtomicsOptional,
                  Op::ProcessAtomicsFinal, Op::Virtual140,
                  Op::GetCameraSubsystem, Op::AddCameraClump}),
          "active animated object follows complete setup path");
    Check(runtime.calls[1].first == 0x5000u &&
              runtime.calls[1].second == 0x7000u &&
              runtime.calls[3].first == 0x7000u &&
              runtime.calls[3].second == 0x512cu &&
              runtime.calls[4].first == 0x7010u &&
              runtime.calls[5].first == 0x7020u &&
              runtime.calls.back().first == 0x9000u &&
              runtime.calls.back().second == 0x7030u,
          "setup rereads clump after each native operation");

    runtime = Runtime{};
    runtime.words[0x6018u] = 1u;
    runtime.words[0x61a8u] = 0u;
    runtime.camera_member = false;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::CleanupObject, Op::Virtual72, Op::ProcessAtomics,
                  Op::ProcessAtomicsFinal, Op::Virtual140}),
          "attached clump, optional material and camera gates");
}

void LeaveFull() {
    Runtime runtime;
    runtime.mutate_clump = true;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Leave(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::Virtual156, Op::CleanupObject, Op::Virtual76,
                  Op::ResetAtomicCallbacks, Op::ResetMaterialCallbacks,
                  Op::ProcessAtomicsWrapper, Op::RemoveClump,
                  Op::Virtual140, Op::GetCameraSubsystem,
                  Op::RemoveCameraClump}),
          "leave resets object, clump and camera state in IDA order");
    Check(runtime.calls[0].first == 0x6000u &&
              runtime.calls[0].second == 0u &&
              runtime.calls[3].first == 0x7000u &&
              runtime.calls[4].first == 0x7010u &&
              runtime.calls[5].first == 0x7020u &&
              runtime.calls[6].first == 0x5000u &&
              runtime.calls[6].second == 0x7030u &&
              runtime.calls.back().first == 0x9000u &&
              runtime.calls.back().second == 0x7040u,
          "leave rereads clump after cleanup and removal callbacks");

    runtime = Runtime{};
    runtime.words[0x61a8u] = 0u;
    runtime.words[0x6018u] = 1u;
    runtime.camera_member = false;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Leave(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::Virtual156, Op::CleanupObject, Op::Virtual76,
                  Op::ResetAtomicCallbacks, Op::ProcessAtomicsWrapper,
                  Op::Virtual140}),
          "optional material, world clump and camera cleanup skip correctly");

    pal4::GameArenaAnimatedObjectRuntimeStub stub;
    Check(pal4::GameArenaAnimatedObjectCallbacks::Enter(0u, 0u, stub) == 0u &&
              pal4::GameArenaAnimatedObjectCallbacks::Leave(0u, 0u, stub) == 0u,
          "native animated-object stub links and exits safely");
}

}  // namespace

int main() {
    try {
        GatesAndFallback();
        EnterFull();
        LeaveFull();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
