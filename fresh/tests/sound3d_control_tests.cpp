#include "pal4/sound3d_control.h"

#include <array>
#include <bit>
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

struct Runtime final : pal4::ISound3DControlRuntime {
    pal4::Sound3DObjectState* object = nullptr;
    std::vector<std::string> events;
    bool audio_active = false;
    std::uint8_t text_refs = 0u;
    std::uint8_t audio_position_result = 0x79u;
    std::array<float, 3> expected_position{3.0f, -2.0f, 7.5f};

    bool AudioFacetActive(pal4::Sound3DObjectState& state,
                          std::uint32_t offset) override {
        Check(&state == object && offset == 60u, "audio facet check");
        events.emplace_back("audio-check");
        return audio_active;
    }
    void HandleActiveAudio(pal4::Sound3DObjectState& state,
                           std::uint32_t offset) override {
        Check(&state == object && offset == 60u, "active audio facet");
        events.emplace_back("audio-handle");
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x5000u && object->words[10u] ==
                  std::bit_cast<std::uint32_t>(expected_position[0]) &&
                  ((object->words[2u] >> 24u) & 0xffu) == 0u,
              "position and playback restored before frame reset");
        events.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t frame,
                    const std::array<float, 3>& scale,
                    std::uint32_t mode) override {
        Check(frame == 0x5000u && scale == std::array<float, 3>{1, 1, 1} &&
                  mode == 0u, "unit scale replaces frame transform");
        events.emplace_back("scale");
    }
    void RotateFrame(std::uint32_t frame,
                     const std::array<float, 3>& axis,
                     float degrees, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        const auto first = audio_active ? 4u : 3u;
        const auto index = events.size() - first;
        Check(frame == 0x5000u && index < axes.size() &&
                  axis == axes[index] && degrees == 0.0f && mode == 2u,
              "ordered native zero rotations");
        events.emplace_back("rotate");
    }
    void TranslateFrame(std::uint32_t frame,
                        const std::array<float, 3>& position,
                        std::uint32_t mode) override {
        Check(frame == 0x5000u && position == expected_position && mode == 2u,
              "frame translated by cached position");
        events.emplace_back("translate");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x3fffu, "text reference address");
        events.emplace_back("ref-read");
        return text_refs;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x3fffu, "text reference write address");
        text_refs = value;
        events.emplace_back("ref-write");
    }
    void FreeText(std::uint32_t allocation) override {
        Check(allocation == 0x3fffu, "text allocation address");
        events.emplace_back("free-text");
    }
    void DestroyFrame(std::uint32_t frame) override {
        Check(frame == 0x5000u && object->words[13u] == frame &&
                  object->words[17u] == 0u,
              "frame destroyed after sound text reset");
        events.emplace_back("frame-destroy");
    }
    void ProcessEvent(std::uint32_t system,
                      pal4::Sound3DObjectState& state) override {
        Check(system == 0x7000u && &state == object &&
                  state.words[13u] == 0u,
              "EffectSystem notified after frame cleared");
        events.emplace_back("process-event");
    }
    std::uint32_t DeleteObject(pal4::Sound3DObjectState& state,
                               std::uint32_t flags) override {
        Check(&state == object && flags == 1u,
              "deleting destructor receives flag one");
        events.emplace_back("delete-object");
        return 0x12345678u;
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x5000u, "sound frame transform input");
        events.emplace_back("object-transform");
        return 0x6000u;
    }
    std::uint8_t SetAudioPosition(pal4::Sound3DObjectState& state,
                                  std::uint32_t facet_offset,
                                  std::uint32_t position_address) override {
        Check(&state == object && facet_offset == 60u &&
                  position_address == 0x6030u,
              "audio receives frame transform translation address");
        events.emplace_back("set-audio-position");
        return audio_position_result;
    }
};

void TestReset(bool audio_active) {
    pal4::Sound3DObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[1u] = 0xaaaaaa01u;
    state.words[2u] = 0x65abcdefu;
    state.words[13u] = 0x5000u;
    Runtime runtime;
    runtime.object = &state;
    runtime.audio_active = audio_active;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[20u + i] =
            std::bit_cast<std::uint32_t>(runtime.expected_position[i]);
    Check(pal4::Sound3DControl::Reset(state, runtime) == 1u,
          "active reset succeeds");
    for (std::uint32_t i = 0u; i < 3u; ++i)
        Check(state.words[10u + i] == state.words[20u + i],
              "all cached positions restored");
    Check(state.words[1u] == 0xaaaaaa01u &&
              state.words[2u] == 0x00abcdefu,
          "playback byte cleared without changing neighbors");
    const std::vector<std::string> expected = audio_active
        ? std::vector<std::string>{"audio-check", "audio-handle", "identity",
                                   "scale", "rotate", "rotate", "rotate",
                                   "translate"}
        : std::vector<std::string>{"audio-check", "identity", "scale",
                                   "rotate", "rotate", "rotate", "translate"};
    Check(runtime.events == expected, "audio and frame call order");
}

void TestStop(std::uint32_t text, std::uint8_t refs) {
    pal4::Sound3DObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[1u] = 0xffaaff01u;
    state.words[4u] = 0x7000u;
    state.words[13u] = 0x5000u;
    state.words[17u] = text;
    Runtime runtime;
    runtime.object = &state;
    runtime.text_refs = refs;
    Check(pal4::Sound3DControl::Stop(state, runtime) == 0x12345678u,
          "stop returns deleting destructor result");
    Check(state.words[1u] == 0xff00ff00u && state.words[10u] == 0u &&
              state.words[11u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[12u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[13u] == 0u &&
              state.words[14u] == 0xaaaa0000u &&
              state.words[17u] == 0u && state.words[18u] == 0u &&
              state.words[19u] == 0u,
          "stop clears activity, sound and frame fields");
    std::vector<std::string> expected;
    if (text != 0u) {
        expected.emplace_back("ref-read");
        expected.emplace_back(refs == 0u || refs == 0xffu
                                  ? "free-text" : "ref-write");
        if (refs != 0u && refs != 0xffu)
            Check(runtime.text_refs == static_cast<std::uint8_t>(refs - 1u),
                  "shared text reference decremented");
    }
    expected.insert(expected.end(),
                    {"frame-destroy", "process-event", "delete-object"});
    Check(runtime.events == expected,
          "text, frame, EffectSystem and deletion order");
}

void TestAudioPosition() {
    pal4::Sound3DObjectState state{};
    Runtime runtime;
    runtime.object = &state;
    state.words[13u] = 0x5000u;
    Check(pal4::Sound3DControl::UpdateAudioPosition(
              state, 0x1111u, 0x2222u, runtime) == 0u &&
              runtime.events.empty(), "inactive position callback stops");
    state.words[1u] = 1u;
    Check(pal4::Sound3DControl::UpdateAudioPosition(
              state, 0u, 0u, runtime) == 0u && runtime.events.empty(),
          "nonplaying sound stops before audio query");
    state.words[14u] = 0x00005500u;
    Check(pal4::Sound3DControl::UpdateAudioPosition(
              state, 0u, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"audio-check"},
          "missing audio object stops before transform");
    runtime.events.clear();
    runtime.audio_active = true;
    state.words[14u] = 0x0000557eu;
    Check(pal4::Sound3DControl::UpdateAudioPosition(
              state, 0u, 0u, runtime) == 0x7eu &&
              runtime.events == std::vector<std::string>{"audio-check"},
          "2D sound returns mode byte without position update");
    runtime.events.clear();
    state.words[14u] = 0x00005500u;
    Check(pal4::Sound3DControl::UpdateAudioPosition(
              state, 0x1234u, 0x5678u, runtime) == 0x79u &&
              runtime.events == std::vector<std::string>{
                  "audio-check", "object-transform", "set-audio-position"},
          "3D sound uses transform translation and returns callback byte");
}

}  // namespace

int main() {
    pal4::Sound3DObjectState inactive{};
    Runtime runtime;
    runtime.object = &inactive;
    Check(pal4::Sound3DControl::Reset(inactive, runtime) == 0u &&
              runtime.events.empty(), "inactive reset has no callbacks");
    TestReset(false);
    TestReset(true);
    pal4::Sound3DObjectState stopped{};
    Runtime stop_runtime;
    stop_runtime.object = &stopped;
    Check(pal4::Sound3DControl::Stop(stopped, stop_runtime) ==
              0x12345678u &&
              stop_runtime.events == std::vector<std::string>{"delete-object"},
          "inactive stop only deletes object");
    TestStop(0u, 0u);
    TestStop(0x4000u, 0u);
    TestStop(0x4000u, 0xffu);
    TestStop(0x4000u, 3u);
    TestAudioPosition();
}
