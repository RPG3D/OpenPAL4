#include "pal4/actor_manager_main_event_handlers.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kGroup = 0x4000u;
constexpr std::uint32_t kHead = 0x5000u;
constexpr std::uint32_t kNode = 0x6000u;
constexpr std::array<std::uint32_t, 10> kGlobals{
    0x8b8b84u, 0x8b8b88u, 0x8b8b8cu, 0x8b62d4u, 0x8b62d8u,
    0x8b5960u, 0x8b5970u, 0x8b5974u, 0x8b5988u, 0x8b5f8cu};
constexpr std::array<std::uint32_t, 10> kCallbacks{
    0x53cf20u, 0x57b8d0u, 0x53d3a0u, 0x53d3f0u, 0x53d650u,
    0x53d680u, 0x53d900u, 0x53d830u, 0x53da70u, 0x53d9a0u};

struct Registration {
    std::uint32_t router;
    std::uint32_t mode;
    std::uint32_t key;
    std::uint32_t owner;
    std::uint32_t callback;
};

struct Runtime final : pal4::IActorManagerLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> groups;
    std::vector<std::string> trace;
    std::vector<Registration> registrations;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t EventRouter() override {
        trace.push_back("router");
        return 0x3000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        trace.push_back("register:" + std::to_string(key));
        registrations.push_back({router, mode, key, owner, callback});
        return 0xb000u + 4u *
            static_cast<std::uint32_t>(registrations.size() - 1u);
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x3000u, "lookup router");
        trace.push_back("group:" + std::to_string(key));
        return groups[key];
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        trace.push_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        trace.push_back("free:" + std::to_string(node));
    }
    void InitializeGameObject(std::uint32_t manager) override {
        Check(manager == kManager, "game object initialization owner");
        trace.push_back("game-init");
    }
    std::uint32_t CleanupGameObject(std::uint32_t manager) override {
        Check(manager == kManager, "game object cleanup owner");
        trace.push_back("game-cleanup");
        return 0x55u;
    }
    void ConstructNameNode(std::uint32_t node) override {
        trace.push_back("name-construct:" + std::to_string(node));
    }
    void ResetNameNode(std::uint32_t node) override {
        trace.push_back("name-reset:" + std::to_string(node));
    }
    std::uint32_t ResetEffectHandle(std::uint32_t& first,
                                    std::uint32_t& second) override {
        trace.push_back("effect-reset:" + std::to_string(first) + ":" +
                        std::to_string(second));
        first = 0xffffffffu;
        second = 0u;
        return 0x77u;
    }
};

void SubscribeAll() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < kGlobals.size(); ++index)
        runtime.words[kGlobals[index]] = 0xa000u + index;

    const auto result = pal4::ActorManagerMainEventHandlers::Subscribe(
        kManager, runtime);
    Check(result == 0xb024u && runtime.registrations.size() == 10u,
          "subscription returns the tenth handle");
    for (std::uint32_t index = 0u; index < kGlobals.size(); ++index) {
        const auto slot = kManager + 0x130u + 8u * index;
        const auto& registration = runtime.registrations[index];
        Check(registration.router == 0x3000u &&
                  registration.mode == 1u &&
                  registration.key == 0xa000u + index &&
                  registration.owner == kManager &&
                  registration.callback == kCallbacks[index] &&
                  runtime.words[slot] == 0xb000u + 4u * index &&
                  runtime.words[slot + 4u] == registration.key,
              "all event globals, callbacks and slot pairs match IDA");
    }
    const auto final_slot = kManager + 0x130u + 8u * 9u;
    Check(runtime.trace[runtime.trace.size() - 2u] ==
              "write:" + std::to_string(final_slot + 4u) + ":40969" &&
              runtime.trace.back() ==
                  "write:" + std::to_string(final_slot) + ":45092",
          "last registration writes key before handle");
}

void CleanupAndReplace() {
    Runtime runtime;
    const auto first = kManager + 0x130u;
    const auto second = first + 8u;
    const auto empty = second + 8u;
    runtime.words[first] = 0xb000u;
    runtime.words[first + 4u] = 0xa000u;
    runtime.words[second] = 0xb004u;
    runtime.words[second + 4u] = 0xa001u;
    runtime.words[empty + 4u] = 0xdeadu;
    runtime.groups[0xa000u] = kGroup;
    runtime.groups[0xa001u] = kGroup + 0x100u;
    runtime.words[kGroup + 4u] = kHead;
    runtime.words[kGroup + 8u] = 2u;
    runtime.words[kHead] = kNode;
    runtime.words[kHead + 4u] = kNode;
    runtime.words[kNode] = kHead;
    runtime.words[kNode + 4u] = kHead;
    runtime.words[kNode + 12u] = 0xb000u;
    runtime.words[kGroup + 0x100u + 4u] = kHead + 0x100u;
    runtime.words[kHead + 0x100u] = kHead + 0x100u;

    Check(pal4::ActorManagerMainEventHandlers::Cleanup(
              kManager, runtime) == 0u &&
              runtime.words[first] == 0u &&
              runtime.words[first + 4u] == 0u &&
              runtime.words[second] == 0u &&
              runtime.words[second + 4u] == 0u &&
              runtime.words[empty + 4u] == 0xdeadu &&
              runtime.words[kHead] == kHead &&
              runtime.words[kHead + 4u] == kHead &&
              runtime.words[kGroup + 8u] == 1u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "release:24584") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "free:24576"),
          "cleanup unlinks matching handle and preserves empty-slot key");

    runtime.trace.clear();
    runtime.words[first] = 0xb010u;
    runtime.words[first + 4u] = 0xa000u;
    runtime.words[kHead] = kHead;
    for (std::uint32_t index = 0u; index < kGlobals.size(); ++index)
        runtime.words[kGlobals[index]] = 0xa100u + index;
    Check(pal4::ActorManagerMainEventHandlers::Subscribe(
              kManager, runtime) == 0xb024u &&
              runtime.trace.front() == "read:" + std::to_string(first) &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "write:" + std::to_string(first) + ":0") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "register:41216"),
          "subscription removes old handles before registering replacements");
}

void Lifecycle() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < kGlobals.size(); ++index)
        runtime.words[kGlobals[index]] = 0xa000u + index;
    runtime.words[kManager + 0x190u] = 0xdeadbeefu;
    Check(pal4::ActorManagerLifecycle::Construct(kManager, runtime) ==
              kManager &&
              runtime.trace.front() == "game-init" &&
              runtime.words[kManager] ==
                  pal4::ActorManagerLifecycle::kVtable &&
              runtime.words[kManager + 0x190u] == 0xdeadbeefu &&
              runtime.registrations.size() == 10u,
          "constructor initializes base, slots and ten subscriptions");
    const auto first_name = "name-construct:" +
        std::to_string(kManager + 384u);
    const auto second_name = "name-construct:" +
        std::to_string(kManager + 392u);
    Check(std::find(runtime.trace.begin(), runtime.trace.end(),
                    first_name) <
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        second_name) &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        second_name) <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "register:40960"),
          "constructor builds name nodes before event registration");

    runtime.trace.clear();
    Check(pal4::ActorManagerLifecycle::Destroy(kManager, runtime) == 0x55u &&
              runtime.trace.front() ==
                  "write:4096:" + std::to_string(
                      pal4::ActorManagerLifecycle::kVtable) &&
              runtime.trace[runtime.trace.size() - 3u] ==
                  "name-reset:" + std::to_string(kManager + 392u) &&
              runtime.trace[runtime.trace.size() - 2u] ==
                  "name-reset:" + std::to_string(kManager + 384u) &&
              runtime.trace.back() == "game-cleanup" &&
              runtime.words[kManager + 0x130u] == 0u,
          "destructor clears subscriptions then resets name nodes in reverse");

    runtime.trace.clear();
    Check(pal4::ActorManagerLifecycle::ResetEffectArgumentPair(
              0xaabbu, 0xccddu, 0xeeffu, runtime) == 0x77u &&
              runtime.trace == std::vector<std::string>{
                  "effect-reset:43707:52445"},
          "effect wrapper resets a local pair and ignores the third word");
}

}  // namespace

int main() {
    try {
        SubscribeAll();
        CleanupAndReplace();
        Lifecycle();
        std::cout << "actor_manager_main_event_handlers_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
