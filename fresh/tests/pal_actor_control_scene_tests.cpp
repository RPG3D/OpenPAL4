#include "pal4/pal_actor_control_scene.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IPalActorControlSceneRuntime {
    pal4::PalActorControlState* state = nullptr;
    std::string_view name;
    bool pet_active = false;
    std::vector<std::string> calls;

    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x7000u, "actor name address");
        calls.emplace_back("name");
        return name;
    }
    void SetActorAnimationFlag(std::uint32_t actor,
                               std::uint32_t value) override {
        Check(actor == 0x6000u && value == 0x3f800000u,
              "old actor flag");
        calls.emplace_back("old animation");
    }
    std::uint32_t GetPetManager() override {
        calls.emplace_back("pet get");
        return 0x9000u;
    }
    bool PetActive(std::uint32_t pet) override {
        Check(pet == 0x9000u, "pet manager address");
        calls.emplace_back("pet active");
        return pet_active;
    }
    std::uint32_t GetArenaManager() override {
        calls.emplace_back("arena get");
        return 0xa000u;
    }
    void ScriptPetOnRemove(std::uint32_t arena) override {
        Check(arena == 0xa000u, "remove arena");
        calls.emplace_back("pet script remove");
    }
    void PetVirtualRemove(std::uint32_t pet) override {
        Check(pet == 0x9000u, "remove pet");
        calls.emplace_back("pet remove");
    }
    std::uint32_t ResetPet(std::uint32_t pet) override {
        Check(pet == 0x9000u, "reset pet");
        calls.emplace_back("pet reset");
        return 0u;
    }
    void PetVirtualLoad(std::uint32_t pet, std::string_view group,
                        std::string_view id, std::uint32_t enabled) override {
        Check(pet == 0x9000u && group == "PAL4Pet" && id == "339" &&
                  enabled == 1u, "pet load constants");
        calls.emplace_back("pet load");
    }
    void InitializePetData(std::uint32_t pet,
                           std::uint32_t actor) override {
        Check(pet == 0x9000u && actor == 0x7000u, "pet actor");
        calls.emplace_back("pet data");
    }
    void SetPetStateFlag(std::uint32_t pet,
                         std::uint32_t value) override {
        Check(pet == 0x9000u && value == 1u, "pet state");
        calls.emplace_back("pet flag");
    }
    std::uint32_t ScriptPetOnAdd(std::uint32_t arena) override {
        Check(arena == 0xa000u, "add arena");
        calls.emplace_back("pet script add");
        return 0u;
    }
    void InitializeCameraSubsystem() override {
        calls.emplace_back("camera init");
    }
    std::uint32_t GetSceneCamera() override {
        calls.emplace_back("camera get");
        return 0xb000u;
    }
    void SetSceneParameter(std::uint32_t camera,
                           std::uint32_t parameter) override {
        Check(camera == 0xb000u && parameter == 0x71ecu,
              "scene parameter address");
        calls.emplace_back("camera scene");
    }
    void SetCameraLookAt(std::uint32_t camera,
                         std::uint32_t enabled) override {
        Check(camera == 0xb000u && enabled == 1u, "camera look at");
        calls.emplace_back("camera look");
    }
    void AddMinimapIcon(std::string_view icon,
                        std::string_view texture) override {
        Check(icon == "LeadingRole" && texture == "actor.png",
              "minimap icon constants");
        calls.emplace_back("icon add");
    }
    void SetActiveMinimapIcon(std::string_view icon) override {
        Check(icon == "LeadingRole", "active icon name");
        calls.emplace_back("icon active");
    }
    void FlushMainCameraTailYaw(
        pal4::PalActorControlState&) override {
        calls.emplace_back("yaw");
    }
    void BindSubobject(std::uint32_t* storage, std::uint32_t actor,
                       std::uint32_t slot) override {
        Check(actor == 0x7000u, "bind actor");
        const auto offset = storage - state->words.data();
        Check(slot == (offset == 209 || offset == 216 ? 12u : 16u),
              "bind virtual slot");
        calls.emplace_back("bind " + std::to_string(offset));
        Check(state->words[7] == 0u && state->words[3] == 1u &&
                  state->words[20] == 0u,
              "state reset precedes binding");
    }
    void AddOrReplaceAnimationMember(
        std::uint32_t animation_member,
        std::uint32_t* member) override {
        Check(animation_member == 0x7170u,
              "add animation uses actor animation member");
        calls.emplace_back("add animation " + std::to_string(
            member - state->words.data()));
    }
    void UpdateGameStateConditional(
        pal4::PalActorControlState&, std::uint32_t value) override {
        Check(value == state->words[33], "game state value");
        calls.emplace_back("state");
    }
    void SetMonsterStateParameter(std::uint32_t actor) override {
        Check(actor == 0x7000u, "monster actor");
        calls.emplace_back("monsters");
    }
    void UpdateGameStateAndUi(
        pal4::PalActorControlState&, std::uint32_t actor) override {
        Check(actor == 0x7000u, "state UI actor");
        calls.emplace_back("state ui");
    }
    std::uint32_t BottomHookAtomic(std::uint32_t actor,
                                   std::string_view hook) override {
        Check(actor == 0x7000u && hook == "hook_ef_bottom",
              "bottom hook query");
        calls.emplace_back("hook");
        return 0xb000u;
    }
};

const std::vector<std::string> kSceneTail{
    "camera init", "camera get", "camera scene", "camera look",
    "icon add", "icon active", "yaw", "bind 23", "bind 35",
    "bind 165", "bind 209", "bind 216", "add animation 205",
    "add animation 207", "add animation 209", "add animation 216", "state",
    "monsters", "state ui", "hook"};

void TestNullActor() {
    pal4::PalActorControlState state{};
    state.words.fill(0x5555u);
    Runtime runtime;
    runtime.state = &state;
    pal4::PalActorControlScene::SetActor(state, 0u, runtime);
    Check(runtime.calls.empty() && state.words[1] == 0x5555u,
          "null actor has no side effects");
}

void TestNamesAndState() {
    for (const auto& [name, expected] :
         {std::pair{"Player-TIANHE", 0u},
          {"player-linsha", 1u},
          {"Player-MENGLI", 2u},
          {"Player-ZIYING", 3u},
          {"unknown", 9u}}) {
        pal4::PalActorControlState state{};
        state.words.fill(0x5555u);
        state.words[1] = 0x6000u;
        state.words[19] = 9u;
        state.words[33] = 2u;
        Runtime runtime;
        runtime.state = &state;
        runtime.name = name;
        pal4::PalActorControlScene::SetActor(state, 0x7000u, runtime);
        std::vector<std::string> expected_calls{
            "old animation", "name", "pet get", "pet active"};
        expected_calls.insert(expected_calls.end(),
                              kSceneTail.begin(), kSceneTail.end());
        Check(runtime.calls == expected_calls && state.words[19] == expected,
              "name lookup and scene call order");
        Check(state.words[1] == 0x7000u && state.words[3] == 1u &&
                  state.words[4] == 1u && state.words[5] == 0u &&
                  state.words[7] == 0u && state.words[8] == 0u &&
                  state.words[20] == 0u && state.words[21] == 0u &&
                  state.words[22] == 0u && state.words[183] == 0u &&
                   state.words[223] == 0xb000u && state.words[224] == 0u,
              "observed direct state writes");
    }
}

void TestPetPaths() {
    for (const auto& [name, reload] :
         {std::pair{"Player-TIANHE", true},
          {"Player-LINSHA", false}}) {
        pal4::PalActorControlState state{};
        state.words[1] = 0x6000u;
        Runtime runtime;
        runtime.state = &state;
        runtime.name = name;
        runtime.pet_active = true;
        pal4::PalActorControlScene::SetActor(state, 0x7000u, runtime);
        std::vector<std::string> expected_calls{
            "name", "pet get", "pet active", "arena get",
            "pet script remove", "pet get", "pet remove",
            "pet get", "pet reset"};
        if (reload) {
            const std::vector<std::string> reload_calls{
                "pet get", "pet load", "pet get", "pet data",
                "pet get", "pet flag", "arena get", "pet script add"};
            expected_calls.insert(expected_calls.end(),
                                  reload_calls.begin(), reload_calls.end());
        }
        expected_calls.insert(expected_calls.end(),
                              kSceneTail.begin(), kSceneTail.end());
        Check(runtime.calls == expected_calls,
              "active pet removal and optional reload order");
    }
}

struct CameraRuntime : pal4::IPalActorControlSceneRuntime {
    std::vector<std::string> calls;
    std::uint32_t camera = 0x100u;
    std::uint32_t internal = 0x200u;
    float correction = 4.0f;
    float sign = -1.0f;
    float new_yaw = 0.0f;

    void InitializeCameraSubsystem() override {
        calls.emplace_back("init");
    }
    std::uint32_t GetSceneCamera() override {
        calls.emplace_back("camera");
        return camera;
    }
    std::uint32_t GetCameraInternal(std::uint32_t value) override {
        Check(value == camera, "camera internal lookup");
        calls.emplace_back("internal");
        return internal;
    }
    float ComputeYawCorrection(pal4::PalActorControlState&,
                               std::uint32_t value,
                               std::uint32_t actor,
                               float& signed_component) override {
        Check(value == internal && actor == 0x7000u,
              "yaw correction inputs");
        calls.emplace_back("compute");
        signed_component = sign;
        return correction;
    }
    float CurrentCameraYaw(std::uint32_t value) override {
        Check(value == internal, "yaw read address");
        calls.emplace_back("yaw");
        return 20.0f;
    }
    void SetCameraYaw(std::uint32_t value, float yaw,
                      std::uint32_t keep_look_at) override {
        Check(value == internal && keep_look_at == 1u,
              "yaw write arguments");
        calls.emplace_back("set");
        new_yaw = yaw;
    }
};

void TestFlushTailYaw() {
    pal4::PalActorControlState state{};
    CameraRuntime runtime;
    pal4::PalActorControlScene::FlushTailYaw(state, runtime);
    Check(runtime.calls.empty(), "no actor skips camera subsystem");
    state.words[1] = 0x7000u;
    runtime.camera = 0u;
    pal4::PalActorControlScene::FlushTailYaw(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"init", "camera"},
          "missing camera stops lookup");
    runtime.calls.clear();
    runtime.camera = 0x100u;
    runtime.internal = 0u;
    pal4::PalActorControlScene::FlushTailYaw(state, runtime);
    Check(runtime.calls ==
              std::vector<std::string>{"init", "camera", "internal"},
          "missing camera internal stops correction");
    runtime.internal = 0x200u;
    for (const auto& [correction, sign, expected_yaw] :
         {std::tuple{0.0f, -1.0f, 0.0f},
          {4.0f, -1.0f, 24.0f},
          {4.0f, 1.0f, 16.0f},
          {4.0f, 0.0f, 0.0f}}) {
        runtime.calls.clear();
        runtime.correction = correction;
        runtime.sign = sign;
        runtime.new_yaw = 0.0f;
        pal4::PalActorControlScene::FlushTailYaw(state, runtime);
        Check(runtime.calls == (correction == 0.0f
                  ? std::vector<std::string>{
                        "init", "camera", "internal", "compute", "yaw"}
                  : sign == 0.0f
                      ? std::vector<std::string>{
                            "init", "camera", "internal", "compute",
                            "yaw"}
                      : std::vector<std::string>{
                            "init", "camera", "internal", "compute",
                            "yaw", "set"}),
              "yaw branch call order");
        Check(runtime.new_yaw == expected_yaw,
              "signed yaw correction");
    }
}

struct ModeRuntime : pal4::IPalActorControlSceneRuntime {
    std::vector<std::string> calls;
    std::uint32_t frame = 0u;
    pal4::PalActorControlState* state = nullptr;
    void SetModeParameters(std::uint32_t* component,
                           std::uint32_t mode,
                           std::uint32_t actor) override {
        Check(component == state->words.data() + 23u &&
                  actor == 0x7000u && mode <= 2u,
              "mode component and actor");
        calls.emplace_back("mode " + std::to_string(mode));
    }
    std::uint32_t FindMovementFrame() override {
        calls.emplace_back("find");
        return frame;
    }
    void UpdateCharacterMovementState(std::uint32_t value) override {
        Check(value == frame, "movement frame");
        calls.emplace_back("movement");
    }
    void UpdateGameStateAndUi(pal4::PalActorControlState&,
                              std::uint32_t actor) override {
        Check(actor == 0x7000u, "mode UI actor");
        calls.emplace_back("ui");
    }
};

void TestModeConditional() {
    pal4::PalActorControlState state{};
    ModeRuntime runtime;
    runtime.state = &state;
    pal4::PalActorControlScene::SetModeConditional(state, 2u, runtime);
    Check(runtime.calls.empty(), "missing actor skips mode updates");
    state.words[1] = 0x7000u;
    pal4::PalActorControlScene::SetModeConditional(state, 2u, runtime);
    Check(runtime.calls == std::vector<std::string>{"mode 2", "ui"},
          "normal mode branch");
    state.words[195] = 1u;
    runtime.calls.clear();
    pal4::PalActorControlScene::SetModeConditional(state, 2u, runtime);
    Check(runtime.calls == std::vector<std::string>{"mode 0", "find"},
          "gated mode skips absent frame");
    runtime.frame = 0x5000u;
    runtime.calls.clear();
    pal4::PalActorControlScene::SetModeConditional(state, 2u, runtime);
    Check(runtime.calls ==
              std::vector<std::string>{"mode 0", "find", "movement"},
          "gated mode updates present frame");
}

}  // namespace

int main() {
    TestNullActor();
    TestNamesAndState();
    TestPetPaths();
    TestFlushTailYaw();
    TestModeConditional();
}
