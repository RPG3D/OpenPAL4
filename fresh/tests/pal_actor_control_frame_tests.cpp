#include "pal4/pal_actor_control_frame.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
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

void SetFloat(pal4::PalActorControlState& state,
              std::uint32_t index, float value) {
    state.words[index] = std::bit_cast<std::uint32_t>(value);
}

float GetFloat(const pal4::PalActorControlState& state,
               std::uint32_t index) {
    return std::bit_cast<float>(state.words[index]);
}

bool Has(const std::vector<std::string>& events,
         std::string_view target) {
    return std::find(events.begin(), events.end(), target) != events.end();
}

struct Runtime : pal4::IPalActorControlFrameRuntime {
    pal4::PalActorControlState* state = nullptr;
    std::vector<std::string> events;
    std::uint32_t arena_mode = 1u;
    bool console_active = false;
    std::uint32_t camera = 0x200u;
    bool dialog_dirty = false;
    bool mode_switch = false;
    bool timer_frame = false;
    bool scenario_visible = false;
    float follow_value = 0.0f;
    bool scene_ready = false;
    std::uint32_t yaw_input = 0u;
    bool camera_input = false;
    bool special_result = false;
    bool component_result = false;
    bool busy = false;
    bool animation_flag = false;
    std::string animation_name = "C01";
    pal4::ActorControlMouseInput mouse{};
    pal4::ActorControlCursorPoint cursor{};
    float last_yaw = 0.0f;
    float last_pitch = 0.0f;
    float component_delta = 0.0f;
    bool effect_output_initial = true;

    std::uint32_t ArenaMode() override {
        events.emplace_back("arena mode");
        return arena_mode;
    }
    bool ConsoleActive() override {
        events.emplace_back("console");
        return console_active;
    }
    std::uint32_t ActiveCamera() override {
        events.emplace_back("camera");
        return camera;
    }
    bool DialogNeedsUpdate(std::uint32_t* dialog) override {
        Check(dialog == state->words.data() + 186u,
              "status dialog offset");
        events.emplace_back("dialog dirty");
        return dialog_dirty;
    }
    void UpdateDialog(std::uint32_t*) override {
        events.emplace_back("dialog update");
    }
    void UpdateEffectProperties(std::uint32_t* dialog,
                                std::uint32_t actor,
                                bool& output) override {
        Check(dialog == state->words.data() + 186u &&
                  actor == 0x100u, "effect arguments");
        effect_output_initial = output;
        events.emplace_back("effect");
    }
    void UpdateTimersAndUi(pal4::PalActorControlState&,
                           float delta) override {
        Check(delta > 0.0f, "timer delta");
        events.emplace_back("timers");
    }
    bool HandleModeSwitch(pal4::PalActorControlState&,
                          float) override {
        events.emplace_back("mode switch");
        return mode_switch;
    }
    bool TimerFrameActive() override {
        events.emplace_back("timer frame");
        return timer_frame;
    }
    bool ScenarioDialogVisible() override {
        events.emplace_back("scenario");
        return scenario_visible;
    }
    float CameraFollowValue() override {
        events.emplace_back("follow value");
        return follow_value;
    }
    bool ArenaSceneReady() override {
        events.emplace_back("scene ready");
        return scene_ready;
    }
    std::uint32_t CameraInternal(std::uint32_t value) override {
        Check(value == camera, "camera internal source");
        events.emplace_back("camera internal");
        return 0x300u;
    }
    std::uint32_t HandleCameraYawInput(
        pal4::PalActorControlState&, float,
        std::uint32_t internal) override {
        Check(internal == 0x300u, "yaw input internal camera");
        events.emplace_back("yaw input");
        return yaw_input;
    }
    bool HandleCameraInput(pal4::PalActorControlState&,
                           float, std::uint32_t actor,
                           std::uint32_t internal) override {
        Check(actor == 0x100u && internal == 0x300u,
              "camera input arguments");
        events.emplace_back("camera input");
        return camera_input;
    }
    void RestoreCamera(std::uint32_t value, float yaw,
                       float pitch, bool look_at) override {
        Check(value == camera, "restore camera address");
        last_yaw = yaw;
        last_pitch = pitch;
        events.emplace_back(look_at ? "restore look" : "restore");
    }
    std::uint32_t UpdateYaw(pal4::PalActorControlState&,
                            std::uint32_t internal,
                            std::uint32_t actor, float step) override {
        Check(internal == 0x300u && actor == 0x100u &&
                  step == 0.5f, "follow yaw arguments");
        events.emplace_back("follow update");
        return 0u;
    }
    void FlushTailYaw(pal4::PalActorControlState&) override {
        events.emplace_back("flush yaw");
    }
    bool AdvanceSpecial(std::uint32_t special,
                        std::uint32_t actor, float delta) override {
        Check(special == 0x900u && actor == 0x100u &&
                  delta == 0.5f, "special control arguments");
        events.emplace_back("special");
        return special_result;
    }
    bool AdvanceComponent(std::uint32_t* component,
                          std::uint32_t actor, float delta) override {
        Check(component == state->words.data() + 23u &&
                  actor == 0x100u && delta > 0.0f,
              "component virtual slot arguments");
        component_delta = delta;
        events.emplace_back("component");
        return component_result;
    }
    void BindSubobject(std::uint32_t* component,
                       std::uint32_t actor) override {
        const auto offset = component - state->words.data();
        Check((offset == 35 || offset == 165) && actor == 0x100u,
              "bound subobject offset");
        events.emplace_back("bind " + std::to_string(offset));
    }
    pal4::ActorControlMouseInput MouseInput() override {
        events.emplace_back("mouse input");
        return mouse;
    }
    pal4::ActorControlCursorPoint CursorPosition() override {
        events.emplace_back("cursor");
        return cursor;
    }
    void QueryWindowRect() override {
        events.emplace_back("window rect");
    }
    pal4::ActorControlCameraAngles CameraAngles(
        std::uint32_t value) override {
        Check(value == camera, "camera angle address");
        events.emplace_back("angles");
        return {10.0f, 20.0f};
    }
    void SetCameraYaw(std::uint32_t value, float yaw) override {
        Check(value == camera, "yaw setter camera");
        last_yaw = yaw;
        events.emplace_back("set yaw");
    }
    void SetCameraPitch(std::uint32_t value, float pitch) override {
        Check(value == camera, "pitch setter camera");
        last_pitch = pitch;
        events.emplace_back("set pitch");
    }
    void AdjustCameraAngles(std::uint32_t value,
                            float yaw_delta,
                            float pitch_delta) override {
        Check(value == camera, "adjust camera address");
        last_yaw = yaw_delta;
        last_pitch = pitch_delta;
        events.emplace_back("adjust");
    }
    bool SubobjectBusy(std::uint32_t* component) override {
        const auto offset = component - state->words.data();
        events.emplace_back("busy " + std::to_string(offset));
        return busy;
    }
    void ReleaseDialog(pal4::PalActorControlState&) override {
        events.emplace_back("release dialog");
    }
    bool AnimationFlag(std::uint32_t actor) override {
        Check(actor == 0x100u, "animation flag actor");
        events.emplace_back("animation flag");
        return animation_flag;
    }
    std::string_view AnimationName(std::uint32_t actor) override {
        Check(actor == 0x100u, "animation name actor");
        events.emplace_back("animation name");
        return animation_name;
    }
    std::uint32_t SetAnimation(std::uint32_t actor,
                               std::string_view name) override {
        Check(actor == 0x100u, "animation setter actor");
        events.emplace_back("set " + std::string(name));
        animation_name = name;
        return name == "C12" ? 0x77u : 0x66u;
    }
    void SetAnimationFlag(std::uint32_t actor,
                          std::uint32_t value) override {
        Check(actor == 0x100u && value == 1u,
              "animation setter flag");
        events.emplace_back("set animation flag");
    }
};

pal4::PalActorControlState ReadyState() {
    pal4::PalActorControlState state{};
    state.words[0] = 1u;
    state.words[1] = 0x100u;
    SetFloat(state, 2u, 1.0f);
    return state;
}

void TestEntryGates() {
    auto state = ReadyState();
    Runtime runtime;
    runtime.state = &state;
    state.words[1] = 0u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(runtime.events.empty(), "missing actor stops before runtime calls");
    state.words[1] = 0x100u;
    state.words[0] = 0u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(runtime.events.empty(), "disabled controller stops early");
    state.words[0] = 1u;
    runtime.arena_mode = 2u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(runtime.events == std::vector<std::string>{"arena mode"},
          "arena mode gate");
    runtime.events.clear();
    runtime.arena_mode = 1u;
    runtime.console_active = true;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(runtime.events ==
              std::vector<std::string>{"arena mode", "console"},
          "console gate");
    runtime.events.clear();
    runtime.console_active = false;
    runtime.camera = 0u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(runtime.events == std::vector<std::string>{
              "arena mode", "console", "camera"},
          "missing active camera gate");
}

void TestStatusAndUiGates() {
    for (const float timer : {0.25f, 2.0f}) {
        auto state = ReadyState();
        state.words[184] = 1u;
        SetFloat(state, 185u, timer);
        Runtime runtime;
        runtime.state = &state;
        runtime.dialog_dirty = true;
        runtime.mode_switch = true;
        pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
        Check(Has(runtime.events, "dialog dirty") &&
                  Has(runtime.events, "timers") &&
                  Has(runtime.events, "mode switch") &&
                  !Has(runtime.events, "timer frame"),
              "status update precedes mode exit");
        if (timer > 0.5f) {
            Check(GetFloat(state, 185u) == 1.5f &&
                      state.words[184] == 1u &&
                      Has(runtime.events, "effect") &&
                      !runtime.effect_output_initial,
                  "active status updates effect with false output");
        } else {
            Check(state.words[185] == 0u &&
                      state.words[184] == 0u &&
                      Has(runtime.events, "dialog update"),
                  "expired status clears timer and refreshes dialog");
        }
    }
    {
        auto state = ReadyState();
        state.words[184] = 1u;
        SetFloat(state, 185u,
                 std::numeric_limits<float>::quiet_NaN());
        Runtime runtime;
        runtime.state = &state;
        runtime.dialog_dirty = true;
        runtime.mode_switch = true;
        pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
        Check(state.words[184] == 1u &&
                  Has(runtime.events, "effect") &&
                  !Has(runtime.events, "dialog update"),
              "unordered FPU comparison follows active status branch");
    }
    for (const bool timer_gate : {false, true}) {
        auto state = ReadyState();
        Runtime runtime;
        runtime.state = &state;
        runtime.timer_frame = timer_gate;
        runtime.scenario_visible = !timer_gate;
        pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
        Check(!Has(runtime.events, "follow value") &&
                  Has(runtime.events, "timer frame") &&
                  Has(runtime.events, "scenario") == !timer_gate,
              "timer and scenario gates stop before camera flow");
    }
}

void TestCameraAndSpecial() {
    auto state = ReadyState();
    state.words[3] = 1u;
    state.words[14] = 1u;
    state.words[183] = 0x900u;
    SetFloat(state, 16u, 10.0f);
    SetFloat(state, 15u, 20.0f);
    Runtime runtime;
    runtime.state = &state;
    runtime.scene_ready = true;
    runtime.follow_value = 1.0f;
    runtime.camera_input = true;
    runtime.special_result = false;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(Has(runtime.events, "yaw input") &&
              Has(runtime.events, "restore") &&
              Has(runtime.events, "follow update") &&
              Has(runtime.events, "flush yaw") &&
              Has(runtime.events, "special") &&
              Has(runtime.events, "animation name") &&
              !Has(runtime.events, "release dialog"),
          "special camera path callback sequence");
    Check(state.words[14] == 0u && state.words[183] == 0u &&
              state.words[4] == 0u && state.words[3] == 0u &&
              runtime.last_yaw == 10.0f && runtime.last_pitch == 20.0f,
          "special camera path state writes");

    state = ReadyState();
    runtime.state = &state;
    runtime.events.clear();
    runtime.yaw_input = 1u;
    state.words[3] = 1u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(state.words[3] == 0u &&
              !Has(runtime.events, "flush yaw"),
          "explicit yaw input suppresses automatic recenter");

    state = ReadyState();
    runtime.state = &state;
    runtime.events.clear();
    runtime.yaw_input = 0u;
    runtime.camera_input = true;
    state.words[4] = 9u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(state.words[4] == 9u &&
              !Has(runtime.events, "special"),
          "camera input without special controller preserves word +16");
}

void TestComponentAndMouse() {
    auto state = ReadyState();
    state.words[14] = 1u;
    SetFloat(state, 16u, 10.0f);
    SetFloat(state, 15u, 20.0f);
    Runtime runtime;
    runtime.state = &state;
    runtime.component_result = true;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(Has(runtime.events, "component") &&
              Has(runtime.events, "restore look") &&
              Has(runtime.events, "bind 35") &&
              Has(runtime.events, "bind 165") &&
              runtime.component_delta == 0.5f &&
              state.words[4] == 0u && state.words[14] == 0u,
          "component path restores camera and binds children");

    state = ReadyState();
    runtime.state = &state;
    runtime.events.clear();
    runtime.component_result = false;
    runtime.scene_ready = true;
    runtime.mouse.primary_count = 1;
    runtime.mouse.primary_mode = 1u;
    runtime.cursor = {10, 20};
    state.words[10] = 2u;
    state.words[11] = 3u;
    SetFloat(state, 12u, 1.0f);
    SetFloat(state, 13u, 1.0f);
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(state.words[4] == 1u && state.words[14] == 1u &&
              state.words[10] == 10u && state.words[11] == 20u &&
              GetFloat(state, 16u) == 10.0f &&
              GetFloat(state, 15u) == 20.0f &&
              Has(runtime.events, "cursor") &&
              Has(runtime.events, "window rect") &&
              Has(runtime.events, "angles"),
          "first drag snapshots camera and cursor");
    runtime.events.clear();
    runtime.cursor = {110, 70};
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(GetFloat(state, 18u) == -60.0f &&
              GetFloat(state, 17u) == 20.0f &&
              runtime.last_yaw == -50.0f &&
              runtime.last_pitch == 40.0f &&
              Has(runtime.events, "set yaw") &&
              Has(runtime.events, "set pitch"),
          "drag clamps relative yaw and pitch to 60/20 degrees");
    runtime.events.clear();
    runtime.mouse.primary_count = 0;
    runtime.mouse.primary_release = 1u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(state.words[14] == 0u && Has(runtime.events, "restore look"),
          "mouse release restores saved camera angles");
}

void TestIdleAnimation() {
    auto state = ReadyState();
    Runtime runtime;
    runtime.state = &state;
    runtime.animation_name = "idle";
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(Has(runtime.events, "set C01") &&
              Has(runtime.events, "set animation flag") &&
              GetFloat(state, 7u) == 0.5f,
          "idle begins C01 and accumulates timer");
    runtime.events.clear();
    pal4::PalActorControlFrame::Update(state, 5.5f, runtime);
    Check(Has(runtime.events, "set C12") &&
              state.words[8] == 0x77u && state.words[7] == 0u,
          "six-second idle switches to C12");
    runtime.events.clear();
    runtime.animation_flag = true;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(Has(runtime.events, "animation flag") &&
              state.words[7] == 0u && state.words[8] == 0u,
          "completed idle animation clears timer and token");

    state = ReadyState();
    runtime.state = &state;
    runtime.events.clear();
    runtime.busy = true;
    state.words[7] = 99u;
    state.words[8] = 88u;
    pal4::PalActorControlFrame::Update(state, 0.5f, runtime);
    Check(state.words[7] == 0u && state.words[8] == 0u &&
              Has(runtime.events, "busy 23") &&
              !Has(runtime.events, "busy 35") &&
              Has(runtime.events, "release dialog"),
          "busy component short-circuits remaining idle checks");
}

}  // namespace

int main() {
    TestEntryGates();
    TestStatusAndUiGates();
    TestCameraAndSpecial();
    TestComponentAndMouse();
    TestIdleAnimation();
}
