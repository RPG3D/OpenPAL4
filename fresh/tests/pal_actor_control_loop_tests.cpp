#include "pal4/pal_actor_control_loop.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IPalActorControlLoopRuntime {
    pal4::PalActorControlState* state = nullptr;
    std::vector<std::string> calls;
    bool active = false;
    bool dialog_dirty = false;
    int pet_gets = 0;

    void SetCursor(std::string_view name) override {
        Check(name == "default.cur", "default cursor name");
        calls.emplace_back("cursor");
    }
    void ProcessInput(pal4::PalActorControlState&) override {
        calls.emplace_back("input");
        Check(state->words[0] <= 1u && state->words[32] == 0u &&
                  state->words[14] == 0u && state->words[22] == 0u,
              "fields cleared before input processing");
    }
    void ProcessInputAndClearVector(
        pal4::PalActorControlState&) override {
        calls.emplace_back("input ui");
    }
    void InitializeInputState() override { calls.emplace_back("input init"); }
    void ProcessMonsters() override { calls.emplace_back("monsters process"); }
    void ClearMonsterFlags() override { calls.emplace_back("monsters clear"); }
    void SetActorAnimationFlag(std::uint32_t actor,
                               std::uint32_t value) override {
        Check(actor == 0x7000u && value == 0x3f800000u,
              "animation receives actor and 1.0f bits");
        calls.emplace_back("animation");
    }
    bool DialogNeedsUpdate(std::uint32_t* dialog) override {
        const auto offset = dialog - state->words.data();
        Check(offset == 279 || offset == 186 || offset == 203,
              "dialog offset");
        calls.emplace_back("needs " + std::to_string(offset));
        return dialog_dirty;
    }
    void UpdateDialog(std::uint32_t* dialog) override {
        calls.emplace_back("dialog " + std::to_string(
            dialog - state->words.data()));
    }
    std::uint8_t UpdateDialogResult(std::uint32_t* dialog) override {
        calls.emplace_back("dialog result " + std::to_string(
            dialog - state->words.data()));
        return 0x5au;
    }
    void UpdateGameStateConditional(
        pal4::PalActorControlState&, std::uint32_t value) override {
        Check(value == 2u, "state value");
        calls.emplace_back("state");
    }
    void UpdateGameStateAndUi(
        pal4::PalActorControlState&, std::uint32_t actor) override {
        Check(actor == 0x7000u, "state actor");
        calls.emplace_back("state ui");
    }
    void UpdateDialogStatus(pal4::PalActorControlState&,
                            std::uint32_t value) override {
        Check(std::bit_cast<float>(value) > 0.0f,
              "only positive dialog status reaches callback");
        calls.emplace_back("status");
    }
    std::uint32_t GetPetManager() override {
        ++pet_gets;
        calls.emplace_back("pet get");
        return 0x9000u;
    }
    bool PetActive(std::uint32_t pet) override {
        Check(pet == 0x9000u, "pet address");
        calls.emplace_back("pet active");
        return active;
    }
    std::uint32_t GetArenaManager() override {
        calls.emplace_back("arena get");
        return 0xa000u;
    }
    void ScriptPetOnRemove(std::uint32_t arena) override {
        Check(arena == 0xa000u, "remove arena");
        calls.emplace_back("remove script");
    }
    void PetVirtualRemove(std::uint32_t pet) override {
        Check(pet == 0x9000u, "remove pet");
        calls.emplace_back("pet remove");
    }
    std::uint32_t ResetPet(std::uint32_t pet) override {
        Check(pet == 0x9000u, "reset pet");
        calls.emplace_back("pet reset");
        return 0x1111u;
    }
    void PetVirtualLoad(std::uint32_t pet, std::string_view group,
                        std::string_view id, std::uint32_t enabled) override {
        Check(pet == 0x9000u && group == "PAL4Pet" && id == "339" &&
                  enabled == 1u, "pet load arguments");
        calls.emplace_back("pet load");
    }
    void InitializePetData(std::uint32_t pet,
                           std::uint32_t actor) override {
        Check(pet == 0x9000u && actor == state->words[1],
              "pet data arguments");
        calls.emplace_back("pet data");
    }
    void SetPetStateFlag(std::uint32_t pet,
                         std::uint32_t value) override {
        Check(pet == 0x9000u && value == 1u, "pet flag arguments");
        calls.emplace_back("pet flag");
    }
    std::uint32_t ScriptPetOnAdd(std::uint32_t arena) override {
        Check(arena == 0xa000u, "add arena");
        calls.emplace_back("add script");
        return 0x2222u;
    }
};

void TestNormalPath() {
    pal4::PalActorControlState state{};
    state.words.fill(0x5555u);
    state.words[1] = 0x7000u;
    state.words[33] = 2u;
    state.words[184] = 1u;
    state.words[185] = std::bit_cast<std::uint32_t>(-4.0f);
    Runtime runtime;
    runtime.state = &state;
    runtime.dialog_dirty = true;
    Check(pal4::PalActorControlLoop::ResetAndSync(
              state, false, runtime) == 0x9000u,
          "normal path returns pet instance");
    Check(runtime.calls == std::vector<std::string>{
              "cursor", "input", "input ui", "input init",
              "monsters process", "animation", "needs 279",
              "dialog 279", "needs 186", "dialog 186",
              "needs 186", "dialog 186", "pet get", "pet active"},
          "normal path call order and repeated dialog update");
    Check(state.words[0] == 0u && state.words[184] == 0u &&
              state.words[185] == 0u && state.words[19] == 0x5555u,
          "normal path clears status but preserves pet gate");
}

void TestPetRemoval() {
    pal4::PalActorControlState state{};
    state.words[19] = 0u;
    Runtime runtime;
    runtime.state = &state;
    runtime.active = true;
    Check(pal4::PalActorControlLoop::ResetAndSync(
              state, false, runtime) == 0x1111u && runtime.pet_gets == 3,
          "normal pet removal returns reset result");
    Check(runtime.calls == std::vector<std::string>{
              "cursor", "input", "input ui", "input init",
              "monsters process", "needs 279", "needs 186",
              "pet get", "pet active", "arena get", "remove script",
              "pet get", "pet remove", "pet get", "pet reset"},
          "pet removal order");
}

void TestClearPath() {
    for (float status : {-1.0f, 0.0f, 1.0f}) {
        pal4::PalActorControlState state{};
        state.words[1] = 0x7000u;
        state.words[33] = 2u;
        state.words[184] = 1u;
        state.words[185] = std::bit_cast<std::uint32_t>(status);
        state.words[19] = 1u;
        Runtime runtime;
        runtime.state = &state;
        runtime.active = true;
        Check(pal4::PalActorControlLoop::ResetAndSync(
                  state, true, runtime) == 0x9000u,
              "locked pet skips rebuild");
        Check(runtime.calls == (status > 0.0f
                  ? std::vector<std::string>{
                        "cursor", "input", "input ui", "input init",
                        "monsters clear", "state", "state ui", "status",
                        "pet get", "pet active"}
                  : std::vector<std::string>{
                        "cursor", "input", "input ui", "input init",
                        "monsters clear", "state", "state ui",
                        "pet get", "pet active"}),
              "dialog status requires positive float");
        Check(state.words[0] == 1u && state.words[184] == 1u,
              "clear path preserves status flag");
    }
}

void TestPetRebuild() {
    pal4::PalActorControlState state{};
    state.words[19] = 0u;
    state.words[1] = 0x7000u;
    Runtime runtime;
    runtime.state = &state;
    runtime.active = true;
    Check(pal4::PalActorControlLoop::ResetAndSync(
              state, true, runtime) == 0x2222u && runtime.pet_gets == 6,
          "clear path rebuilds pet and returns script result");
    Check(runtime.calls == std::vector<std::string>{
              "cursor", "input", "input ui", "input init",
              "monsters clear", "pet get", "pet active", "arena get",
              "remove script", "pet get", "pet remove", "pet get",
              "pet reset", "pet get", "pet load", "pet get", "pet data",
              "pet get", "pet flag", "arena get", "add script"},
          "pet rebuild preserves virtual and script order");
}

void TestClearActor() {
    for (const bool dirty : {false, true}) {
        pal4::PalActorControlState state{};
        state.words.fill(0x5555u);
        state.words[0] = 0u;
        state.words[14] = 0u;
        state.words[22] = 0u;
        state.words[32] = 0u;
        Runtime runtime;
        runtime.state = &state;
        runtime.dialog_dirty = dirty;
        Check(pal4::PalActorControlLoop::ClearActor(state, runtime) ==
                  (dirty ? 0x5au : 0u),
              "last dialog returns update result or false");
        Check(runtime.calls == (dirty
                  ? std::vector<std::string>{
                        "input", "input ui", "input init", "needs 203",
                        "dialog 203",
                        "needs 186", "dialog 186", "needs 279",
                        "dialog result 279"}
                  : std::vector<std::string>{
                        "input", "input ui", "input init", "needs 203",
                        "needs 186", "needs 279"}),
              "clear actor callback order");
        for (const auto index : {1u, 6u, 183u, 184u, 185u,
                                 188u, 195u, 202u,
                                 20u, 21u, 22u, 223u, 224u})
            Check(state.words[index] == 0u,
                  "clear actor direct writes");
        Check(state.words[19] == 0x5555u &&
                  state.words[279] == 0x5555u,
              "clear actor preserves other fields");
    }
}

void TestUnboundActiveEffect() {
    pal4::IPalActorControlLoopRuntime runtime;
    std::uint32_t handle[2]{};
    Check(!runtime.DialogNeedsUpdate(handle),
          "empty effect handle needs no effect system");
    handle[1] = 1u;
    bool rejected = false;
    try {
        (void)runtime.DialogNeedsUpdate(handle);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    Check(rejected, "active effect handle requires a bound effect system");
}

}  // namespace

int main() {
    TestNormalPath();
    TestPetRemoval();
    TestClearPath();
    TestPetRebuild();
    TestClearActor();
    TestUnboundActiveEffect();
}
