#include "pal4/player_manager_operations.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t SlotIndex(std::uint32_t slot) {
    Check(slot >= 0x5000u && (slot - 0x5000u) % 0xb14u == 0u,
          "runtime receives a player slot base");
    return (slot - 0x5000u) / 0xb14u;
}

struct Runtime final : pal4::PlayerManagerOperationsRuntimeStub {
    std::vector<std::string> events;

    std::uint32_t UpdateInventory(std::uint32_t slot,
                                  std::uint32_t enabled) override {
        Check(enabled <= 1u, "inventory receives binary enabled flag");
        events.push_back("inventory" + std::to_string(SlotIndex(slot)) +
                         ":" + std::to_string(enabled));
        return enabled != 0u ? 0xfeedu : 0u;
    }
    void ProcessGameData(std::uint32_t slot) override {
        events.push_back("data" + std::to_string(SlotIndex(slot)));
    }
    void CallScriptIndex(std::uint32_t script,
        std::uint32_t virtual_slot, std::uint32_t index) override {
        Check(virtual_slot == 0x18u &&
                  script == 0x5000u + index * 0xb14u + 0x654u,
              "loaded-data script call uses +0x654 and slot +0x18");
        events.push_back("script" + std::to_string(index));
    }
    std::uint32_t SetStateFlag(std::uint32_t slot,
                               std::uint32_t value) override {
        Check(value == 0u, "state reset writes zero");
        const auto index = SlotIndex(slot);
        events.push_back("state" + std::to_string(index));
        return index + 10u;
    }
    void ApplyMaterialTexture(std::uint32_t slot) override {
        events.push_back("texture1-" + std::to_string(SlotIndex(slot)));
    }
    std::uint32_t ApplyMaterialTexture2(std::uint32_t slot) override {
        const auto index = SlotIndex(slot);
        events.push_back("texture2-" + std::to_string(index));
        return index + 20u;
    }
    void CallSlotReset(std::uint32_t slot,
                       std::uint32_t virtual_slot) override {
        Check(virtual_slot == 0x2cu, "reset uses virtual slot +0x2c");
        events.push_back("reset" + std::to_string(SlotIndex(slot)));
    }
    std::uint32_t ActorControl() override {
        events.emplace_back("control");
        return 0x7000u;
    }
    std::uint32_t ClearActor(std::uint32_t actor_control) override {
        Check(actor_control == 0x7000u,
              "clear receives actor-control singleton");
        events.emplace_back("clear");
        return 0xbeefu;
    }
    std::uint32_t ForAllPlayers(std::uint32_t manager,
        std::uint32_t callback, std::uint32_t context) override {
        Check(manager == 0x5000u && callback == 0x8100u &&
                  context == 0x8200u,
              "for-all forwards manager, callback and context");
        events.emplace_back("for-all");
        return 0xcafeu;
    }
    void CallSlotRender(std::uint32_t slot,
        std::uint32_t virtual_slot, std::uint32_t argument) override {
        Check(virtual_slot == 0x34u && argument == 0x9000u,
              "render uses virtual slot +0x34 and argument");
        events.push_back("render" + std::to_string(SlotIndex(slot)));
    }
    void RenderDebug(std::uint32_t actor_control) override {
        Check(actor_control == 0x7000u,
              "debug render receives actor-control singleton");
        events.emplace_back("debug");
    }
    std::uint32_t CallSlotUpdate(std::uint32_t slot,
        std::uint32_t virtual_slot, std::uint32_t argument) override {
        Check(virtual_slot == 0x24u && argument == 0x9000u,
              "update uses virtual slot +0x24 and argument");
        const auto index = SlotIndex(slot);
        events.push_back("update" + std::to_string(index));
        return index + 100u;
    }
    std::uint32_t IterateRelated(std::uint32_t subobject,
                                  std::uint32_t argument) override {
        Check(subobject >= 0x5000u + 0xaa8u &&
                  argument == 0x9000u,
              "related iterator receives subobject and argument");
        const auto index = SlotIndex(subobject - 0xaa8u);
        events.push_back("related" + std::to_string(index));
        return index + 200u;
    }
    void CallSlotProcess(std::uint32_t slot,
        std::uint32_t virtual_slot, float elapsed,
        std::uint32_t argument) override {
        Check(virtual_slot == 0x38u && elapsed == 1.25f &&
                  argument == 0x9000u,
              "game-element call forwards both arguments to slot +0x38");
        events.push_back("element" + std::to_string(SlotIndex(slot)));
    }
    std::uint32_t UpdateControlFrame(std::uint32_t actor_control,
                                      float elapsed) override {
        Check(actor_control == 0x7000u && elapsed == 1.25f,
              "frame update receives actor control and elapsed float");
        events.emplace_back("frame");
        return 0xf00du;
    }
};

using Operations = pal4::PlayerManagerOperations;

void TestLoadAndResetLoops() {
    Runtime runtime;
    Check(Operations::ProcessLoadedData(0x5000u, runtime) == 0xfeedu,
          "loaded-data processor returns final inventory result");
    const std::vector<std::string> expected{
        "inventory0:0", "data0", "script0",
        "inventory1:0", "data1", "script1",
        "inventory2:0", "data2", "script2",
        "inventory3:0", "data3", "script3", "inventory0:1"};
    Check(runtime.events == expected,
          "loaded-data processor visits every slot in observed order");
    runtime.events.clear();
    Check(Operations::ResetData(0x5000u, runtime) == 13u &&
              runtime.events == std::vector<std::string>(
                  {"state0", "state1", "state2", "state3"}),
          "state reset returns fourth slot result");
    runtime.events.clear();
    Check(Operations::UpdateData(0x5000u, runtime) == 23u &&
              runtime.events == std::vector<std::string>(
                  {"texture1-0", "texture2-0", "texture1-1", "texture2-1",
                   "texture1-2", "texture2-2", "texture1-3", "texture2-3"}),
          "texture update returns fourth secondary call result");
}

void TestControlAndCallback() {
    Runtime runtime;
    Check(Operations::ResetAllPlayerStates(0x5000u, runtime) == 0xbeefu &&
              runtime.events == std::vector<std::string>(
                  {"reset0", "reset1", "reset2", "reset3", "control", "clear"}),
          "all slots reset before actor control clear");
    runtime.events.clear();
    Check(Operations::ForAllPlayers(
              0x5000u, 0x8100u, 0x8200u, runtime) == 0xcafeu &&
              runtime.events == std::vector<std::string>{"for-all"},
          "for-all wrapper forwards arguments and return");
}

void TestActiveSlotLoops() {
    pal4::PlayerManagerObjectState state{};
    state.Word((1u * 0xb14u + 0x1f0u) / 4u) = 1u;
    state.Word((3u * 0xb14u + 0x1f0u) / 4u) = 2u;
    Runtime runtime;
    Operations::UpdateAndRender(state, 0x5000u, 0x9000u, runtime);
    Check(runtime.events == std::vector<std::string>(
              {"render1", "render3", "control", "debug"}),
          "render loop skips inactive slots and always renders debug data");
    runtime.events.clear();
    Check(Operations::ProcessPlayerUpdates(
              state, 0x5000u, 0x9000u, runtime) == 103u &&
              runtime.events ==
                  std::vector<std::string>({"update1", "update3"}),
          "update loop returns final active virtual result");
    runtime.events.clear();
    Check(Operations::ProcessPlayerRelatedData(
              state, 0x5000u, 0x9000u, runtime) == 203u &&
              runtime.events ==
                  std::vector<std::string>({"related1", "related3"}),
          "related loop uses +0xaa8 and returns final iterator result");
    runtime.events.clear();
    Check(Operations::ProcessGameElements(
              state, 0x5000u, 1.25f, 0x9000u, runtime) == 0xf00du &&
              runtime.events == std::vector<std::string>(
                  {"element1", "element3", "control", "frame"}),
          "game-element loop runs active slots before frame update");

    state.Word((3u * 0xb14u + 0x1f0u) / 4u) = 0u;
    runtime.events.clear();
    Check(Operations::ProcessPlayerUpdates(
              state, 0x5000u, 0x9000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"update1"},
          "inactive final slot replaces prior return with zero flag");
    runtime.events.clear();
    Check(Operations::ProcessPlayerRelatedData(
              state, 0x5000u, 0x9000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"related1"},
          "related loop likewise returns final inactive flag");
    runtime.events.clear();
    Check(Operations::ProcessGameElements(
              state, 0x5000u, 1.25f, 0x9000u, runtime) == 0xf00du &&
              runtime.events == std::vector<std::string>(
                  {"element1", "control", "frame"}),
          "game-element loop still updates frame with final slot inactive");
}

}  // namespace

int main() {
    TestLoadAndResetLoops();
    TestControlAndCallback();
    TestActiveSlotLoops();
}
