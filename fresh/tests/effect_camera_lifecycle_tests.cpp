#include "pal4/effect_camera_lifecycle.h"

#include <array>
#include <bit>
#include <cstdint>
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

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

std::uint8_t Byte(const pal4::EffectCameraObjectState& state,
                  std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void PutByte(pal4::EffectCameraObjectState& state, std::uint32_t offset,
             std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    state.words[offset / 4u] =
        (state.words[offset / 4u] & ~mask) |
        (static_cast<std::uint32_t>(value) << shift);
}

struct Runtime final : pal4::IEffectCameraLifecycleRuntime {
    std::vector<std::string> events;
    bool context_ready = true;
    bool has_target = true;
    std::array<std::uint32_t, 3> translated{};
    std::uint32_t applied_transform = 0u;

    void StopFacet(pal4::EffectCameraObjectState& state,
                   std::uint32_t offset) override {
        Check(offset == 24u && Byte(state, 4u) != 0u,
              "stop facet receives active +24 owner facet");
        events.push_back("stop-facet");
    }
    void ProcessEvent2(pal4::EffectCameraObjectState&) override {
        events.push_back("event2");
    }
    std::uint32_t Finalize(pal4::EffectCameraObjectState&,
                            std::uint32_t flag) override {
        Check(flag == 1u, "final virtual dispatch argument");
        events.push_back("finalize");
        return 0xabcdef12u;
    }
    void DestroyFrame(std::uint32_t frame) override {
        events.push_back("destroy:" + std::to_string(frame));
    }
    std::uint8_t UpdateTransform(
        pal4::EffectCameraObjectState&) override {
        events.push_back("transform");
        return 0x33u;
    }
    bool ContextReady(pal4::EffectCameraObjectState&) override {
        events.push_back("context");
        return context_ready;
    }
    bool HasTargetObject(pal4::EffectCameraObjectState& state) override {
        Check(Byte(state, 6u) == 0u && Byte(state, 7u) == 0u,
              "pause clears running flags before target lookup");
        events.push_back("has-target");
        return has_target;
    }
    std::uint32_t TargetFrame(
        pal4::EffectCameraObjectState&) override {
        events.push_back("target");
        return 0x9000u;
    }
    std::uint32_t CloneHierarchy(std::uint32_t frame) override {
        Check(frame == 0x9000u, "hierarchy clone source");
        events.push_back("clone");
        return 0xa000u;
    }
    void ProcessEvent(pal4::EffectCameraObjectState&) override {
        events.push_back("event");
    }
    void TranslateFrame(std::uint32_t frame,
                        const std::array<std::uint32_t, 3>& position,
                        std::uint32_t mode) override {
        Check(frame == 0x9000u && mode == 2u,
              "translation target and RenderWare mode");
        translated = position;
        events.push_back("translate");
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x9000u, "target frame identity");
        events.push_back("identity");
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x8000u, "source camera frame transform");
        events.push_back("object-transform");
        return 0xb000u;
    }
    void ApplyTransform(std::uint32_t frame,
                        std::uint32_t transform,
                        std::uint32_t mode) override {
        Check(frame == 0x9000u && mode == 0u,
              "native transform application mode");
        applied_transform = transform;
        events.push_back("apply");
    }
    std::uint8_t UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x9000u, "updated target frame");
        events.push_back("frame-update");
        return 0x5au;
    }
};

void TestStop() {
    Runtime runtime;
    pal4::EffectCameraObjectState state{};
    Check(pal4::EffectCameraLifecycle::Stop(state, runtime) ==
              0xabcdef12u &&
              runtime.events == std::vector<std::string>{"finalize"},
          "inactive stop still dispatches finalizer");
    runtime.events.clear();
    PutByte(state, 4u, 1u);
    PutByte(state, 6u, 1u);
    PutByte(state, 7u, 1u);
    PutByte(state, 44u, 1u);
    PutByte(state, 47u, 1u);
    for (std::uint32_t index = 22u; index <= 27u; ++index)
        state.words[index] = 0xffffffffu;
    state.words[12u] = 0xffffffffu;
    state.words[13u] = 0xffffffffu;
    state.words[14u] = 0xffffffffu;
    state.words[31u] = 0x8000u;
    state.words[32u] = 0xa000u;
    Check(pal4::EffectCameraLifecycle::Stop(state, runtime) ==
              0xabcdef12u &&
              runtime.events == std::vector<std::string>{
                  "stop-facet", "destroy:40960", "destroy:32768",
                  "event2", "finalize"} &&
              Byte(state, 4u) == 0u && Byte(state, 6u) == 0u &&
              Byte(state, 7u) == 0u && Byte(state, 44u) == 0u &&
              Byte(state, 47u) == 0u &&
              state.words[22u] == 0u && state.words[25u] == 0u &&
              state.words[26u] == Bits(1.0f) &&
              state.words[27u] == Bits(1.0f) &&
              state.words[12u] == 0u &&
              state.words[13u] == Bits(1.0f) &&
              state.words[14u] == Bits(1.0f) &&
              state.words[31u] == 0u && state.words[32u] == 0u,
          "active stop clears state and destroys clone before source frame");
    runtime.events.clear();
    PutByte(state, 4u, 1u);
    Check(pal4::EffectCameraLifecycle::Stop(state, runtime) ==
              0xabcdef12u &&
              runtime.events == std::vector<std::string>{
                  "stop-facet", "event2", "finalize"},
          "active stop skips absent frames");
}

void TestUpdate() {
    Runtime runtime;
    pal4::EffectCameraObjectState state{};
    Check(pal4::EffectCameraLifecycle::Update(state, runtime) == 0u &&
              runtime.events.empty(), "inactive update is gated");
    PutByte(state, 4u, 1u);
    Check(pal4::EffectCameraLifecycle::Update(state, runtime) == 0u &&
              runtime.events.empty(), "stopped update is gated");
    PutByte(state, 6u, 1u);
    state.words[31u] = 0x8000u;
    Check(pal4::EffectCameraLifecycle::Update(state, runtime) == 0x5au &&
              runtime.events == std::vector<std::string>{
                  "transform", "target", "identity", "object-transform",
                  "apply", "frame-update"} &&
              runtime.applied_transform == 0xb000u,
          "noninherit update applies source transform to target frame");

    runtime.events.clear();
    PutByte(state, 44u, 1u);
    state.words[25u] = Bits(3.0f);
    state.words[26u] = Bits(4.0f);
    state.words[27u] = Bits(5.0f);
    Check(pal4::EffectCameraLifecycle::Update(state, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "transform", "target", "translate"} &&
              runtime.translated == std::array<std::uint32_t, 3>{
                  Bits(3.0f), Bits(4.0f), Bits(5.0f)} &&
              state.words[25u] == 0u &&
              state.words[26u] == Bits(1.0f) &&
              state.words[27u] == Bits(1.0f),
          "inherit update translates then resets stored position");
}

void TestStartAndPause() {
    Runtime runtime;
    pal4::EffectCameraObjectState state{};
    Check(pal4::EffectCameraLifecycle::Start(state, runtime) == 0u &&
              runtime.events.empty(), "inactive start is gated");
    PutByte(state, 4u, 1u);
    Check(pal4::EffectCameraLifecycle::Start(state, runtime) == 0x33u &&
              runtime.events == std::vector<std::string>{
                  "event2", "context", "target", "clone", "event",
                  "transform"} &&
              state.words[32u] == 0xa000u &&
              Byte(state, 6u) == 1u && Byte(state, 7u) == 0u &&
              Byte(state, 80u) == 1u,
          "start clones target frame after context check and emits event");
    runtime.events.clear();
    Check(pal4::EffectCameraLifecycle::Start(state, runtime) == 1u &&
              runtime.events.empty(), "running camera is not started twice");

    pal4::EffectCameraLifecycle::Pause(state, runtime);
    Check(runtime.events == std::vector<std::string>{
              "has-target", "target", "identity", "apply",
              "frame-update", "destroy:40960"} &&
              runtime.applied_transform == 0xa010u &&
              state.words[32u] == 0u &&
              Byte(state, 6u) == 0u && Byte(state, 7u) == 0u &&
              Byte(state, 80u) == 0u,
          "pause applies cloned transform then destroys clone");
    runtime.events.clear();
    pal4::EffectCameraLifecycle::Pause(state, runtime);
    Check(runtime.events.empty(), "already paused camera does no work");

    pal4::EffectCameraObjectState unavailable{};
    PutByte(unavailable, 4u, 1u);
    runtime.context_ready = false;
    Check(pal4::EffectCameraLifecycle::Start(unavailable, runtime) ==
              0x33u &&
              runtime.events == std::vector<std::string>{
                  "event2", "context", "transform"} &&
              Byte(unavailable, 80u) == 0u &&
              Byte(unavailable, 6u) == 0u,
          "unavailable context skips clone but still updates transform");
    runtime.events.clear();
    PutByte(unavailable, 44u, 1u);
    Check(pal4::EffectCameraLifecycle::Start(unavailable, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "event2", "context"},
          "inherit flag skips transform when context is unavailable");
}

}  // namespace

int main() {
    TestStop();
    TestUpdate();
    TestStartAndPause();
}
