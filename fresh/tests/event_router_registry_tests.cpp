#include "pal4/event_router_registry.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kRouter = 0x1000u;
constexpr std::uint32_t kGroup = 0x2000u;
constexpr std::uint32_t kHead = 0x3000u;
constexpr std::uint32_t kEvent = 0x4000u;
constexpr std::uint32_t kNode = 0x5000u;

struct Runtime final : pal4::IEventRouterRegistryRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kGroup + 20u, kHead}, {kGroup + 24u, 0u},
        {kHead, kHead}, {kHead + 4u, kHead}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t group = kGroup;
    std::uint32_t event = kEvent;
    std::uint32_t position = kHead;
    std::uint32_t created_mode = 0xffffffffu;

    std::uint32_t FindGroup(std::uint32_t router,
                            std::uint32_t key) override {
        Check(router == kRouter && key == 0x7000u, "group lookup arguments");
        calls.emplace_back("find-group");
        return group;
    }
    std::uint32_t CreateGroup(std::uint32_t router,
                              std::uint32_t key,
                              std::uint8_t mode) override {
        Check(router == kRouter && key == 0x7000u, "group creation arguments");
        created_mode = mode;
        calls.emplace_back("create-group");
        return kGroup;
    }
    std::uint32_t FindPosition(std::uint32_t first,
                               std::uint32_t head,
                               std::uint32_t mode) override {
        Check(first == words.at(kHead) && head == kHead && mode == 0x102u,
              "list insertion search arguments");
        calls.emplace_back("find-position");
        return position;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u || bytes == 20u, "event/list allocation size");
        calls.emplace_back(bytes == 12u ? "event-allocate" : "node-allocate");
        return bytes == 12u ? event : kNode;
    }
    std::uint32_t CreateScriptEvent(std::uint32_t owner,
                                     std::uint32_t callback,
                                     std::uint32_t context,
                                     std::uint32_t output_slot) override {
        Check(owner == 0x8000u && callback == 0x9000u &&
                  context == 0xa000u && output_slot == kEvent + 8u &&
                  words.at(kEvent) == pal4::EventRouterRegistry::kEventVtable &&
                  words.at(kEvent + 4u) == 0u &&
                  words.at(kEvent + 8u) == 0u,
              "script event sees initialized object and output slot");
        words[output_slot] = 0xb000u;
        calls.emplace_back("script-event");
        return 0xc000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected registry read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
};

std::uint32_t Register(Runtime& runtime) {
    return pal4::EventRouterRegistry::Register(
        kRouter, 0x102u, 0x7000u, 0x8000u, 0x9000u, 0xa000u, runtime);
}

void NewGroupAndOwnedEvent() {
    Runtime runtime;
    runtime.group = 0u;
    Check(Register(runtime) == kEvent &&
              runtime.created_mode == 2u &&
              runtime.words.at(kEvent) ==
                  pal4::EventRouterRegistry::kEventVtable &&
              runtime.words.at(kEvent + 4u) == 0xc000u &&
              runtime.words.at(kEvent + 8u) == 0xb000u &&
              runtime.words.at(kNode) == kHead &&
              runtime.words.at(kNode + 4u) == kHead &&
              runtime.words.at(kHead) == kNode &&
              runtime.words.at(kHead + 4u) == kNode &&
              runtime.bytes.at(kNode + 8u) == 1u &&
              runtime.words.at(kNode + 12u) == kEvent &&
              runtime.words.at(kNode + 16u) == 0x102u &&
              runtime.words.at(kGroup + 24u) == 1u,
          "registration creates group, event and owned list node");
    Check(runtime.calls == std::vector<std::string>{
              "find-group", "create-group", "event-allocate",
              "script-event", "find-position", "node-allocate"},
          "registration preserves lookup, construction and insertion order");
}

void ExistingGroupAndAllocationFailure() {
    Runtime runtime;
    runtime.words[kGroup + 24u] = 3u;
    runtime.words[kHead] = 0x6000u;
    runtime.words[0x6004u] = 0x6100u;
    runtime.words[0x6100u] = 0x6000u;
    runtime.position = 0x6000u;
    runtime.event = 0u;
    Check(Register(runtime) == 0u &&
              runtime.created_mode == 0xffffffffu &&
              runtime.words.at(kNode) == 0x6000u &&
              runtime.words.at(kNode + 4u) == 0x6100u &&
              runtime.words.at(0x6004u) == kNode &&
              runtime.words.at(0x6100u) == kNode &&
              runtime.bytes.at(kNode + 8u) == 0u &&
              runtime.words.at(kNode + 12u) == 0u &&
              runtime.words.at(kGroup + 24u) == 4u,
          "failed event allocation still inserts an empty owned slot");
    Check(runtime.calls == std::vector<std::string>{
              "find-group", "event-allocate", "find-position",
              "node-allocate"},
          "existing group bypasses creation and failed event constructor");
}

}  // namespace

int main() {
    try {
        NewGroupAndOwnedEvent();
        ExistingGroupAndAllocationFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
