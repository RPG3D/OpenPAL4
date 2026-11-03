#include "pal4/actor_manager_combat_actor.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerObjectListCleanupRuntime {
    pal4::ActorManagerObjectPointerRange generated{};
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::uint32_t free_result = 17u;

    void CleanupObjectLists(std::uint32_t address) override {
        Check(address == actor, "list cleanup actor address");
        trace.emplace_back("cleanup");
    }
    void CollectObjects(
        std::uint32_t collection,
        pal4::ActorManagerObjectPointerRange& output) override {
        Check(collection == actor + 896u, "collection offset");
        Check(output.marker == 0u && output.begin == 0u &&
                  output.end == 0u && output.capacity_end == 0u,
              "temporary pointer range starts empty");
        output = generated;
        trace.emplace_back("collect");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void HandleObjectFade(std::uint32_t address,
                          std::uint32_t object,
                          std::uint32_t context) override {
        Check(address == actor && context == actor,
              "fade receives actor twice in native call");
        trace.emplace_back("fade:" + std::to_string(object));
    }
    std::uint32_t Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
        return free_result;
    }
};

void EmptyRange() {
    Runtime runtime;
    Check(pal4::ActorManagerCombatActor::CleanupObjectLists(
              actor, runtime) == 17u &&
              runtime.trace == std::vector<std::string>{
                  "cleanup", "collect", "free:0"},
          "empty range skips fade and frees null begin");
}

void MultipleObjects() {
    Runtime runtime;
    runtime.generated = {0x7fu, 0x3000u, 0x300cu, 0x3010u};
    runtime.words[0x3000u] = 0x6000u;
    runtime.words[0x3004u] = 0u;
    runtime.words[0x3008u] = 0x7000u;
    runtime.free_result = 0x55u;
    Check(pal4::ActorManagerCombatActor::CleanupObjectLists(
              actor, runtime) == 0x55u &&
              runtime.trace == std::vector<std::string>{
                  "cleanup", "collect", "read:12288", "fade:24576",
                  "read:12292", "fade:0", "read:12296", "fade:28672",
                  "free:12288"},
          "every pointer, including null, is passed to fade before one free");
}

}  // namespace

int main() {
    try {
        EmptyRange();
        MultipleObjects();
        std::cout << "actor_manager_object_list_cleanup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
