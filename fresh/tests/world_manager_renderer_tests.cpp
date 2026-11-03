#include "pal4/world_manager_renderer.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kContextPointer = 0x6000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerRenderRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t context_reads = 0u;
    std::uint32_t actor_managers = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kContextPointer) {
            calls.emplace_back("context:" + std::to_string(context_reads));
            return 0x7000u + context_reads++;
        }
        calls.emplace_back("read:" + std::to_string(address - kManager));
        return words[address];
    }
    void RenderWorld(std::uint32_t world,
                     std::uint32_t context) override {
        Check(world == kManager + 116u && context == 0x7000u,
              "world render receives first context value");
        calls.emplace_back("render-world");
    }
    std::uint32_t ActorManager() override {
        calls.emplace_back("actor-manager:" +
                           std::to_string(actor_managers));
        return 0x8000u + actor_managers++;
    }
    void ProcessActorUpdates(std::uint32_t actor_manager,
                             std::uint32_t context) override {
        Check(actor_manager == 0x8000u && context == 0x7001u,
              "actor update receives first singleton and second context");
        calls.emplace_back("process-actors");
    }
    void RenderClump(std::uint32_t clump) override {
        calls.emplace_back("clump:" + std::to_string(clump));
    }
    void SortAtomicGeometries(std::uint32_t list) override {
        calls.emplace_back("sort:" + std::to_string(list - kManager));
    }
    std::uint32_t UpdateActorComponents(std::uint32_t actor_manager,
                                        std::uint32_t context) override {
        Check(actor_manager == 0x8001u && context == 0x7002u,
              "component update receives second singleton and third context");
        calls.emplace_back("update-components");
        return 0x55u;
    }
};

void BothClumpsAndRepeatedReads() {
    Runtime runtime;
    runtime.words[kManager + 248u] = 0x9000u;
    runtime.words[kManager + 228u] = 0xa000u;
    Check(pal4::WorldManagerRenderer::Render(
              kManager, kContextPointer, runtime) == 0x55u &&
              runtime.context_reads == 3u &&
              runtime.actor_managers == 2u,
          "renderer forwards final result with three context reads");
    Check(runtime.calls == (std::vector<std::string>{
              "context:0", "render-world", "context:1",
              "actor-manager:0", "process-actors", "read:248",
              "clump:36864", "read:228", "clump:40960",
              "sort:328", "sort:312", "context:2",
              "actor-manager:1", "update-components"}),
          "render and update operations retain exact IDA order");
    Runtime scene;
    scene.words=runtime.words;
    Check(pal4::WorldManagerRenderer::RenderScene(kManager+116u,kContextPointer,scene)==0x55u &&
          scene.calls==runtime.calls,"embedded scene entry preserves manager rendering sequence without fabricated manager storage");
}

void MissingClumpsStillSortAndUpdate() {
    Runtime runtime;
    Check(pal4::WorldManagerRenderer::Render(
              kManager, kContextPointer, runtime) == 0x55u &&
              std::none_of(runtime.calls.begin(), runtime.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("clump:");
                           }),
          "null clumps skip RenderWare calls");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "sort:328") <
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "sort:312") &&
              runtime.calls.back() == "update-components",
          "empty clumps preserve list sort and final update");
}

}  // namespace

int main() {
    try {
        BothClumpsAndRepeatedReads();
        MissingClumpsStillSortAndUpdate();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
