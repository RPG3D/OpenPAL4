#include "pal4/game_object_behavior_setup.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::uint32_t kAddress = 0x10000u;
constexpr std::uint32_t kNpc = 0x9000u;

struct Runtime final : pal4::GameObjectBehaviorSetupRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t document{0xa000u};
    std::uint32_t child{0xb000u};
    bool expect_child_before_virtual{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == kNpc + 0x4cu, "NPC behavior document offset");
        calls.emplace_back("read-document");
        return document;
    }
    void ClearProcessorEvents(std::uint32_t processor) override {
        Check(processor == kAddress + 0x658u,
              "event cleanup processor address");
        calls.emplace_back("clear-events");
    }
    void CleanupScriptCollection(std::uint32_t script) override {
        Check(script == kAddress + 0x688u,
              "script collection cleanup address");
        calls.emplace_back("cleanup-script");
    }
    void SetProcessorValue(std::uint32_t processor,
                           std::uint32_t callback) override {
        Check(processor == kAddress + 0x658u && callback == 0x594e00u,
              "processor callback address");
        calls.emplace_back("set-value");
    }
    void SetProcessorFlag(std::uint32_t processor,
                          std::uint32_t flag) override {
        Check(processor == kAddress + 0x658u && flag == 1u,
              "processor flag");
        calls.emplace_back("set-flag");
    }
    std::uint32_t GetChildByIndex(std::uint32_t source,
                                  std::uint32_t& index) override {
        Check(source == document && index == 0u, "first behavior child");
        calls.emplace_back("get-child");
        index = 3u;
        return child;
    }
    void ProcessNpcBehaviorConfig(pal4::GameObjectState& state,
                                  std::uint32_t address,
                                  std::uint32_t value) override {
        Check(address == kAddress && value == child &&
              state.words[0x6d8u / 4u] == child,
              "child is stored before configuration processing");
        calls.emplace_back("process-config");
    }
    std::uint32_t CallObjectVirtual(pal4::GameObjectState& state,
                                    std::uint32_t address,
                                    std::uint32_t slot,
                                    std::uint32_t source,
                                    std::uint32_t flag) override {
        Check(address == kAddress && flag == 0u &&
              ((slot == 4u && source == kNpc + 0x24u) ||
               (slot == 8u && source == kNpc + 0x30u)),
              "optional object virtual arguments");
        if (expect_child_before_virtual)
            Check(state.words[0x6d8u / 4u] == 0x7777u,
                  "virtual callback precedes child clear");
        calls.push_back("slot-" + std::to_string(slot));
        return 0x4321u;
    }
};

}  // namespace

int main() {
    pal4::GameObjectState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    pal4::GameObjectBehaviorSetup::Setup(
        state, kAddress, 0u, 1u, runtime);
    Check(state.words[0x654u / 4u] == 0u &&
          state.words[0x6d8u / 4u] == 0xdeadbeefu &&
          runtime.calls.empty(), "null NPC only updates source field");

    runtime.document = 0u;
    pal4::GameObjectBehaviorSetup::Setup(
        state, kAddress, kNpc, 1u, runtime);
    Check(state.words[0x654u / 4u] == kNpc &&
          state.words[0x6d8u / 4u] == 0xdeadbeefu &&
          runtime.calls == std::vector<std::string>{"read-document"},
          "null behavior document exits before processor setup");

    runtime.document = 0xa000u;
    runtime.calls.clear();
    pal4::GameObjectBehaviorSetup::Setup(
        state, kAddress, kNpc, 0u, runtime);
    Check(state.words[0x6d8u / 4u] == 0xb000u &&
          runtime.calls == std::vector<std::string>{
              "read-document", "set-value", "set-flag",
              "get-child", "process-config"},
          "NPC behavior setup without field attachment");

    runtime.calls.clear();
    runtime.child = 0u;
    pal4::GameObjectBehaviorSetup::Setup(
        state, kAddress, kNpc, 1u, runtime);
    Check(state.words[0x6d8u / 4u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "read-document", "set-value", "set-flag", "get-child",
              "process-config", "slot-4", "slot-8"},
          "null child still processes config and optional fields");

    runtime.calls.clear();
    runtime.child = 0xb000u;
    pal4::GameObjectBehaviorSetup::ProcessEventsAndClear(
        state, kAddress, kNpc, 1u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read-document", "clear-events", "cleanup-script",
              "set-value", "set-flag", "get-child", "process-config",
              "slot-4", "slot-8"},
          "event-reset variant cleans old data before configuration");

    runtime.calls.clear();
    runtime.document = 0u;
    pal4::GameObjectBehaviorSetup::ProcessEventsAndClear(
        state, kAddress, kNpc, 1u, runtime);
    Check(runtime.calls == std::vector<std::string>{"read-document"},
          "event-reset variant leaves old data when document is null");

    runtime.calls.clear();
    state.words[0x654u / 4u] = 0u;
    state.words[0x6d8u / 4u] = 0x7777u;
    Check(pal4::GameObjectBehaviorSetup::CleanupEventsAndScripts(
              state, kAddress, 1u, runtime) == 0u &&
          state.words[0x6d8u / 4u] == 0x7777u &&
          runtime.calls.empty(), "inactive cleanup leaves child untouched");

    state.words[0x654u / 4u] = kNpc;
    Check(pal4::GameObjectBehaviorSetup::CleanupEventsAndScripts(
              state, kAddress, 0u, runtime) == 0u &&
          state.words[0x6d8u / 4u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "clear-events", "cleanup-script"},
          "non-detaching cleanup clears event state and child");

    runtime.calls.clear();
    state.words[0x6d8u / 4u] = 0x7777u;
    runtime.expect_child_before_virtual = true;
    Check(pal4::GameObjectBehaviorSetup::CleanupEventsAndScripts(
              state, kAddress, 1u, runtime) == 0x4321u &&
          state.words[0x6d8u / 4u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "clear-events", "cleanup-script", "slot-4"},
          "detaching cleanup returns virtual result before child clear");
}
