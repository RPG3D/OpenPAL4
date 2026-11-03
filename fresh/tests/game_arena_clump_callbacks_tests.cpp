#include "pal4/game_arena_clump_callbacks.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Op = pal4::GameArenaClumpCall;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Invocation {
    Op operation;
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t third;
    std::uint32_t fourth;
    std::uint32_t fifth;
    std::uint32_t sixth;
};

struct Runtime final : pal4::IGameArenaClumpRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5030u, 1u}, {0x6044u, 0x7000u}, {0x6004u, 0u},
        {0x6040u, 0x8000u}, {0xa000u, 0xb000u},
        {0xa004u, 0xb004u}};
    std::vector<Invocation> calls;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    bool mutate_on_first = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second, std::uint32_t third,
                       std::uint32_t fourth, std::uint32_t fifth,
                       std::uint32_t sixth) override {
        calls.push_back({operation, first, second, third, fourth,
                         fifth, sixth});
        if (operation == Op::GetMovementAdapter) return 0xa000u;
        if (operation == Op::GetCameraSubsystem) return 0xc000u;
        if (operation == Op::GetResourceProperty) return first + 1u;
        if (mutate_on_first &&
            (operation == Op::SetClumpProperties ||
             operation == Op::RestoreRenderCallback)) {
            words[0x6044u] = 0x7010u;
            words[0x6040u] = 0x8010u;
        }
        return 0u;
    }
};

std::vector<Op> Operations(const Runtime& runtime) {
    std::vector<Op> result;
    for (const auto& call : runtime.calls) result.push_back(call.operation);
    return result;
}

void GatesAndSpecialTypes() {
    Runtime runtime;
    runtime.words[0x5030u] = 0u;
    Check(pal4::GameArenaClumpCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0u &&
              runtime.reads == std::vector<std::uint32_t>{0x5030u},
          "world gate returns before object access");
    runtime = Runtime{};
    runtime.words[0x6044u] = 0u;
    Check(pal4::GameArenaClumpCallbacks::Leave(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.calls.empty() && runtime.writes.empty(),
          "null clump preserves active field");

    for (const auto type : {4u, 5u}) {
        runtime = Runtime{};
        runtime.words[0x6004u] = type;
        Check(pal4::GameArenaClumpCallbacks::Enter(
                  0x6000u, 0x5000u, runtime) == 0x6000u &&
                  runtime.writes ==
                      (std::vector<std::pair<std::uint32_t,
                                             std::uint32_t>>{
                          {0x604cu, 1u}}),
              "enter sets active flag for nonnull clump");
        if (type == 4u) {
            Check(Operations(runtime) == (std::vector<Op>{
                      Op::GetCameraSubsystem, Op::AddCameraClump}) &&
                      runtime.calls.back().first == 0xc000u &&
                      runtime.calls.back().second == 0x7000u,
                  "type 4 adds clump to camera collection");
        } else {
            Check(runtime.calls.empty(),
                  "type 5 skips movement/camera services");
        }

        runtime.calls.clear();
        runtime.writes.clear();
        Check(pal4::GameArenaClumpCallbacks::Leave(
                  0x6000u, 0x5000u, runtime) == 0x6000u &&
                  runtime.writes ==
                      (std::vector<std::pair<std::uint32_t,
                                             std::uint32_t>>{
                          {0x604cu, 0u}}),
              "leave clears active flag");
        Check(type == 4u
                  ? Operations(runtime) == (std::vector<Op>{
                        Op::GetCameraSubsystem, Op::RemoveCameraClump})
                  : runtime.calls.empty(),
              "type-specific leave path");
    }
}

void OrdinaryType() {
    Runtime runtime;
    runtime.mutate_on_first = true;
    Check(pal4::GameArenaClumpCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::GetMovementAdapter, Op::GetResourceProperty,
                  Op::SetClumpProperties, Op::GetMovementAdapter,
                  Op::GetResourceProperty, Op::SetClumpProperties}),
          "type zero applies properties to both movement adapter slots");
    Check(runtime.calls[2].first == 0xb000u &&
              runtime.calls[2].second == 0x7000u &&
              runtime.calls[2].third == 0x8001u &&
              runtime.calls[2].fourth == 1u &&
              runtime.calls[2].fifth == 1u &&
              runtime.calls[2].sixth == 0x8000u &&
              runtime.calls[5].first == 0xb004u &&
              runtime.calls[5].second == 0x7010u &&
              runtime.calls[5].third == 0x8011u &&
              runtime.calls[5].sixth == 0x8010u,
          "second property call rereads clump and resource after first callback");

    runtime = Runtime{};
    runtime.words[0x6004u] = 2u;
    Check(pal4::GameArenaClumpCallbacks::Enter(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.calls.size() == 3u,
          "nonzero ordinary type uses only primary adapter slot");

    runtime = Runtime{};
    runtime.mutate_on_first = true;
    Check(pal4::GameArenaClumpCallbacks::Leave(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              Operations(runtime) == (std::vector<Op>{
                  Op::GetMovementAdapter, Op::RestoreRenderCallback,
                  Op::GetMovementAdapter, Op::RestoreRenderCallback}) &&
              runtime.calls[1].first == 0xb000u &&
              runtime.calls[1].second == 0x7000u &&
              runtime.calls[3].first == 0xb004u &&
              runtime.calls[3].second == 0x7010u,
          "leave restores both callbacks and rereads clump");

    pal4::GameArenaClumpRuntimeStub stub;
    Check(pal4::GameArenaClumpCallbacks::Enter(0u, 0u, stub) == 0u &&
              pal4::GameArenaClumpCallbacks::Leave(0u, 0u, stub) == 0u,
          "native clump runtime stub links");
}

}  // namespace

int main() {
    try {
        GatesAndSpecialTypes();
        OrdinaryType();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
