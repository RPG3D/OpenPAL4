#include "pal4/actor_control_database.h"
#include "pal4/actor_control_input_component.h"
#include "pal4/pal_actor_control.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IActorControlInputComponentRuntime {
    std::int32_t config = 3;
    pal4::ActorControlInputComponentState* observed = nullptr;
    unsigned config_reads = 0;
    unsigned animation_calls = 0;
    std::uint32_t animation = 0;
    std::uint32_t value = 0;
    unsigned event_calls = 0;
    unsigned update_calls = 0;
    std::uint32_t callback_result = 0;

    std::int32_t GameConfigPrimary() override {
        ++config_reads;
        Check(observed->words[0] == pal4::ActorControlInputComponent::kVtable &&
                  observed->words[1] == 0x4329e666u &&
                  observed->words[2] == 0x42fdcccdu,
              "constructor reads config after the first three writes");
        return config;
    }
    void SetAnimationFlag(std::uint32_t target,
                          std::uint32_t flag) override {
        ++animation_calls;
        animation = target;
        value = flag;
        Check(observed->words[4] == 0u && observed->words[9] == 0u,
              "reset clears fields before calling animation");
    }
    void HandleInputEvents(pal4::ActorControlInputComponentState& state,
                           std::uint32_t actor, float delta) override {
        ++event_calls;
        Check(&state == observed && actor == 0x123400u &&
                  std::bit_cast<std::uint32_t>(delta) == 0x3e800000u &&
                  state.words[4] == 0u,
              "event branch sees original arguments and cleared status");
        state.words[4] = callback_result;
    }
    void UpdateInputObject(pal4::ActorControlInputComponentState& state,
                           std::uint32_t actor, float delta) override {
        ++update_calls;
        Check(&state == observed && actor == 0x123400u &&
                  std::bit_cast<std::uint32_t>(delta) == 0x3e800000u &&
                  state.words[4] == 0u,
              "object branch sees original arguments and cleared status");
        state.words[4] = callback_result;
    }
};

struct ControllerRuntime final : pal4::PalActorControlRuntimeStub {
    std::int32_t GameConfig(std::uint32_t index) override {
        Check(index == 0u, "component reads primary config");
        return 9;
    }
};

void CheckConstruct(std::int32_t config, std::uint32_t result_bits) {
    pal4::ActorControlInputComponentState state;
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    runtime.config = config;
    runtime.observed = &state;
    auto& result = pal4::ActorControlInputComponent::Construct(state, runtime);
    Check(&result == &state && runtime.config_reads == 1u,
          "constructor returns storage and reads config once");
    Check(state.words == std::array<std::uint32_t, 12>{
              0x842700u, 0x4329e666u, 0x42fdcccdu, result_bits,
              0u, 1u, 0u, 0u, 0u, 0u, 1u, 0x3f800000u},
          "constructor writes the complete 48-byte object");
}

}  // namespace

int main() {
    CheckConstruct(3, std::bit_cast<std::uint32_t>(90.0f));
    CheckConstruct(7, std::bit_cast<std::uint32_t>(
        static_cast<float>(270.0 / 7.0)));
    CheckConstruct(-4, std::bit_cast<std::uint32_t>(-67.5f));
    CheckConstruct(0, 0x7f800000u);

    pal4::ActorControlInputComponentState state;
    state.words.fill(0xccccccccu);
    Runtime runtime;
    runtime.observed = &state;
    Check(pal4::ActorControlInputComponent::Initialize(state) == 1u &&
              state.words[4] == 0u && state.words[9] == 0xccccccccu,
          "initialize clears only active status");
    state.words[4] = 42u;
    Check(pal4::ActorControlInputComponent::ResetActor(
              state, 0u, runtime) == 1u &&
              state.words[4] == 0u && state.words[9] == 0u &&
              runtime.animation_calls == 0u,
          "null actor clears status without animation call");
    state.words[4] = 9u;
    state.words[9] = 8u;
    Check(pal4::ActorControlInputComponent::ResetActor(
              state, 0x123400u, runtime) == 1u &&
              runtime.animation_calls == 1u &&
              runtime.animation == 0x123570u &&
              runtime.value == 0x3f800000u,
          "actor reset passes animation subobject and one-float bits");
    state.words[4] = 7u;
    state.words[9] = 6u;
    Check(pal4::ActorControlInputComponent::ResetActorWithoutAnimation(
              state, 0x123400u) == 1u &&
              state.words[4] == 0u && state.words[9] == 0u &&
              runtime.animation_calls == 1u,
          "second reset ignores actor and does not animate");
    for (std::size_t index = 0; index < state.words.size(); ++index) {
        if (index != 4u && index != 9u)
            Check(state.words[index] == 0xccccccccu,
                  "reset preserves unrelated fields");
    }
    state.words[5] = 1u;
    state.words[4] = 99u;
    runtime.callback_result = 0x123u;
    Check(pal4::ActorControlInputComponent::ProcessInput(
              state, 0x123400u, 0.25f, runtime) == 0x123u &&
              runtime.event_calls == 1u && runtime.update_calls == 0u,
          "nonzero mode chooses event handler and returns its status field");
    state.words[5] = 0u;
    state.words[4] = 99u;
    runtime.callback_result = 0x456u;
    Check(pal4::ActorControlInputComponent::ProcessInput(
              state, 0x123400u, 0.25f, runtime) == 0x456u &&
              runtime.event_calls == 1u && runtime.update_calls == 1u,
          "zero mode chooses object update and returns its status field");

    std::array<std::uint32_t, 3> base{0xdeadbeefu, 0x12345678u,
                                       0x87654321u};
    pal4::ActorControlBase::RestoreVtable(base.data());
    Check(base == std::array<std::uint32_t, 3>{0x8425d4u, 0x12345678u,
                                                0x87654321u},
          "shared base restore only writes vtable");

    ControllerRuntime controller;
    std::array<std::uint32_t, 12> storage;
    storage.fill(0xdeadbeefu);
    controller.ConstructGameComponent(storage.data());
    Check(storage[0] == 0x842700u &&
              storage[3] == std::bit_cast<std::uint32_t>(30.0f),
          "default controller composes component construction");
    storage[4] = 2u;
    controller.InitializeComponent(storage.data());
    Check(storage[4] == 0u, "default controller composes initialize");
    storage[4] = 2u;
    storage[9] = 3u;
    controller.ProcessComponentInput(storage.data(), 0u);
    Check(storage[4] == 0u && storage[9] == 0u,
          "default controller composes actor reset");
    storage[4] = 2u;
    storage[9] = 3u;
    controller.ProcessComponentInputWithUi(storage.data(), 0u);
    Check(storage[4] == 0u && storage[9] == 0u,
          "default controller composes alternate reset");

    for (const auto mode : {0u, 1u, 2u}) {
        pal4::ActorControlInputComponentState mode_state;
        mode_state.words.fill(0xababababu);
        const auto old_words = mode_state.words;
        Check(pal4::ActorControlInputComponent::SetMovementMode(
                  mode_state, mode, 0xdeadbeefu) == 0u &&
                  mode_state.words[10] == mode,
              "valid mode is stored and returns zero");
        const auto multiplier_bits =
            mode == 0u ? 0x3ecccccdU :
            mode == 1u ? 0x3f800000U : 0x3fc00000U;
        const auto expected_speed = static_cast<float>(
            static_cast<double>(std::bit_cast<float>(multiplier_bits)) *
            static_cast<double>(std::bit_cast<float>(0x4329e666u)));
        Check(mode_state.words[11] == multiplier_bits &&
                  mode_state.words[1] ==
                      std::bit_cast<std::uint32_t>(expected_speed),
              "valid mode selects exact multiplier and base speed");
        for (std::size_t index = 0; index < mode_state.words.size(); ++index)
            if (index != 1u && index != 10u && index != 11u)
                Check(mode_state.words[index] == old_words[index],
                      "mode setter preserves other fields");
    }
    pal4::ActorControlInputComponentState unknown_mode;
    unknown_mode.words.fill(0xcdcdcdcdu);
    unknown_mode.words[11] = std::bit_cast<std::uint32_t>(2.0f);
    Check(pal4::ActorControlInputComponent::SetMovementMode(
              unknown_mode, 3u, 0u) == 1u &&
              unknown_mode.words[11] ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              unknown_mode.words[1] == std::bit_cast<std::uint32_t>(
                  static_cast<float>(2.0 *
                      static_cast<double>(
                          std::bit_cast<float>(0x4329e666u)))),
          "unknown mode preserves multiplier and returns mode minus two");
    Check(pal4::ActorControlInputComponent::SetMovementMode(
              unknown_mode, 0xffffffffu, 0x1234u) == 0xfffffffdu &&
              unknown_mode.words[10] == 0xffffffffu &&
              unknown_mode.words[11] ==
                  std::bit_cast<std::uint32_t>(2.0f),
          "negative signed mode wraps its observed return and ignores arg two");
}
