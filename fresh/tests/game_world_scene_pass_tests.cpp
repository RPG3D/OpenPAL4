#include "pal4/game_world_scene_pass.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Op = pal4::GameWorldScenePassOperation;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    std::string name;
    std::uint32_t first;
    std::uint32_t second = 0u;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IGameWorldScenePassRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Event> events;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back({"read", address});
        return words[address];
    }
    std::uint32_t Call(Op operation, std::uint32_t first,
                       std::uint32_t second) override {
        const char* name = operation == Op::RenderClump
                               ? "render"
                               : operation == Op::UpdateArenaCache ? "cache"
                                                                   : "sort";
        events.push_back({name, first, second});
        return operation == Op::SortGeometry ? first + 0x10u : 0xdeadu;
    }
};

void GateAndFullPass() {
    Runtime closed;
    Check(pal4::GameWorldScenePass::Process(0x5000u, 0x777u, closed) ==
              0u && closed.events == std::vector<Event>{
                  {"read", 0x5030u}},
          "zero world+48 exits before clump or cache work");

    Runtime runtime;
    runtime.words[0x5030u] = 1u;
    runtime.words[0x5084u] = 0x9000u;
    runtime.words[0x5070u] = 0x8000u;
    Check(pal4::GameWorldScenePass::Process(0x5000u, 0x777u,
                                              runtime) == 0x512cu &&
              runtime.events == std::vector<Event>{
                  {"read", 0x5030u},
                  {"read", 0x5084u},
                  {"render", 0x9000u},
                  {"cache", 0x51c0u, 0x777u},
                  {"read", 0x5070u},
                  {"render", 0x8000u},
                  {"sort", 0x510cu},
                  {"sort", 0x512cu},
                  {"sort", 0x511cu},
              },
          "scene pass renders secondary then primary and sorts 268/300/284");
}

void OptionalClumpsAndDirectEntries() {
    Runtime runtime;
    runtime.words[0x5030u] = 1u;
    Check(pal4::GameWorldScenePass::Process(0x5000u, 0x777u,
                                              runtime) == 0x512cu &&
              runtime.events == std::vector<Event>{
                  {"read", 0x5030u},
                  {"read", 0x5084u},
                  {"cache", 0x51c0u, 0x777u},
                  {"read", 0x5070u},
                  {"sort", 0x510cu},
                  {"sort", 0x512cu},
                  {"sort", 0x511cu},
              },
          "null clumps skip rendering but retain cache and sort stages");

    Runtime direct;
    Check(pal4::GameWorldScenePass::RenderPrimaryClump(
              0x5000u, direct) == 0u &&
              pal4::GameWorldScenePass::RenderSecondaryClump(
                  0x5000u, direct) == 0u &&
              direct.events == std::vector<Event>{
                  {"read", 0x5070u}, {"read", 0x5084u}},
          "direct render entries return zero for absent clumps");
    direct.events.clear();
    direct.words[0x5070u] = 0x8000u;
    direct.words[0x5084u] = 0x9000u;
    Check(pal4::GameWorldScenePass::RenderPrimaryClump(
              0x5000u, direct) == 0xdeadu &&
              pal4::GameWorldScenePass::RenderSecondaryClump(
                  0x5000u, direct) == 0xdeadu &&
              direct.events == std::vector<Event>{
                  {"read", 0x5070u}, {"render", 0x8000u},
                  {"read", 0x5084u}, {"render", 0x9000u}},
          "direct render entries forward clump handles and result");
}

void NativeStub() {
    pal4::GameWorldScenePassRuntimeStub runtime;
    Check(pal4::GameWorldScenePass::Process(0x5000u, 0x777u,
                                              runtime) == 0u,
          "native RenderWare stub links and exits on absent scene");
}

}  // namespace

int main() {
    try {
        GateAndFullPass();
        OptionalClumpsAndDirectEntries();
        NativeStub();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
