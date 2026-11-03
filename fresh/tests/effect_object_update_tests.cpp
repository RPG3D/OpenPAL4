#include "pal4/effect_object_update.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint8_t Byte(const pal4::EffectObjectState& state,
                  const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

void SetByte(pal4::EffectObjectState& state, const std::uint32_t offset,
             const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

pal4::EffectObjectState ActiveState() {
    pal4::EffectObjectState state{};
    state.words[3u] = 0x5000u;
    state.words[4u] = 0x6000u;
    state.words[23u] = 0x3000u;
    state.words[24u] = 0x4000u;
    SetByte(state, 4u, 1u);
    SetByte(state, 6u, 1u);
    SetByte(state, 7u, 1u);
    SetByte(state, 88u, 1u);
    return state;
}

struct Runtime final : pal4::IEffectObjectUpdateRuntime {
    std::vector<std::string> calls;
    std::uint32_t resource = 0u;
    std::uint32_t child = 0x7000u;
    std::uint32_t frame_parent = 1u;
    std::uint32_t prepare = 1u;
    std::uint32_t activate = 0x123401u;
    std::uint8_t expected_flag_at_frame = 0u;
    pal4::EffectObjectState* state = nullptr;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back(address == 0x3008u ? "resource" :
                           address == 0x30acu ? "child" : "frame_parent");
        if (address == 0x3008u) return resource;
        if (address == 0x30acu) return child;
        Check(address == 0x4004u, "frame parent address");
        if (state != nullptr)
            Check(Byte(*state, 6u) == expected_flag_at_frame,
                  "failure branch flag-clearing order");
        return frame_parent;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == 0x5058u, "owner color byte address");
        calls.emplace_back("read_owner");
        return 0x7bu;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x300cu && value == 0x6000u,
              "owner system forwarded");
        calls.emplace_back("write_system");
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Check((address == 0x3004u && value == 1u) ||
                  (address == 0x3050u && value == 0u) ||
                  (address == 0x3058u && value == 0x7bu),
              "instance byte writes");
        calls.emplace_back(address == 0x3004u ? "active" :
                           address == 0x3050u ? "clear" : "owner_byte");
    }
    std::uint32_t Advance(const std::uint32_t instance,
                          const float elapsed) override {
        Check(instance == 0x3000u && elapsed == 1.25f,
              "double elapsed converts to float");
        calls.emplace_back("advance");
        return 0x87654321u;
    }
    std::uint32_t Prepare(const std::uint32_t instance) override {
        Check(instance == 0x3000u, "prepare instance");
        calls.emplace_back("prepare");
        return prepare;
    }
    std::uint32_t Activate(const std::uint32_t instance) override {
        Check(instance == 0x3000u, "activate instance");
        calls.emplace_back("activate");
        return activate;
    }
    void AddFrameChild(const std::uint32_t parent,
                       const std::uint32_t input_child) override {
        Check(parent == 0x4000u && input_child == child,
              "add native frame child");
        calls.emplace_back("add_child");
    }
    void RemoveFrameChild(const std::uint32_t frame) override {
        Check(frame == 0x4000u, "remove native frame child");
        calls.emplace_back("remove_child");
    }
    std::uint32_t DestroyFrame(const std::uint32_t frame) override {
        Check(frame == 0x4000u, "destroy native frame");
        calls.emplace_back("destroy_frame");
        return 0xbbbbu;
    }
    void CleanupInstance(const std::uint32_t instance,
                         const std::uint32_t context) override {
        Check(instance == 0x3000u && context == 0x9000u,
              "cleanup receives native context");
        calls.emplace_back("cleanup");
    }
    void DestroyInstance(const std::uint32_t instance,
                         const std::uint32_t context) override {
        Check(instance == 0x3000u && context == 0x9000u,
              "destruction receives native context");
        calls.emplace_back("destroy_instance");
    }
};

std::uint32_t Update(pal4::EffectObjectState& state, Runtime& runtime) {
    runtime.state = &state;
    return pal4::EffectObjectUpdater::Update(
        state, 1.25, 0x99u, 0x9000u, runtime);
}

void TestGatesAndAdvance() {
    auto state = ActiveState();
    Runtime runtime;
    SetByte(state, 4u, 0u);
    Check(Update(state, runtime) == 0x99u && runtime.calls.empty(),
          "inactive keeps entry EAX");
    SetByte(state, 4u, 1u);
    SetByte(state, 6u, 0u);
    Check(Update(state, runtime) == 0x99u && runtime.calls.empty(),
          "not started keeps entry EAX");
    SetByte(state, 6u, 1u);
    SetByte(state, 11u, 1u);
    Check(Update(state, runtime) == 0x99u && runtime.calls.empty(),
          "paused keeps entry EAX");
    SetByte(state, 11u, 0u);
    runtime.resource = 1u;
    Check(Update(state, runtime) == 0x87654321u &&
              runtime.calls == std::vector<std::string>{"resource", "advance"},
          "loaded resource advances using float elapsed");
}

void TestPendingAndSuccess() {
    auto state = ActiveState();
    Runtime runtime;
    SetByte(state, 88u, 0u);
    Check(Update(state, runtime) == 0x99u &&
              runtime.calls == std::vector<std::string>{"resource"} &&
              Byte(state, 6u) == 0u && Byte(state, 7u) == 0u,
          "missing pending flag clears playback state");

    state = ActiveState();
    runtime.calls.clear();
    Check(Update(state, runtime) == runtime.activate &&
              runtime.calls == std::vector<std::string>{
                  "resource", "write_system", "active", "clear", "prepare",
                  "child", "add_child", "read_owner", "owner_byte", "activate"} &&
              Byte(state, 6u) == 1u && Byte(state, 7u) == 1u,
          "pending resource activation order and return");
}

void TestFailures() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.prepare = 0u;
    Check(Update(state, runtime) == 0xbbbbu &&
              runtime.calls == std::vector<std::string>{
                  "resource", "write_system", "active", "clear", "prepare",
                  "cleanup", "destroy_instance", "frame_parent",
                  "remove_child", "destroy_frame"} &&
              state.words[23u] == 0u && state.words[24u] == 0u &&
              Byte(state, 4u) == 0u && Byte(state, 6u) == 0u &&
              Byte(state, 7u) == 0u,
          "prepare failure releases resources and clears flags early");

    state = ActiveState();
    runtime.calls.clear();
    runtime.prepare = 1u;
    runtime.activate = 0x123400u;
    runtime.expected_flag_at_frame = 1u;
    runtime.frame_parent = 0u;
    runtime.child = 0u;
    state.words[3u] = 0u;
    Check(Update(state, runtime) == 0xbbbbu &&
              runtime.calls == std::vector<std::string>{
                  "resource", "write_system", "active", "clear", "prepare",
                  "child", "activate", "cleanup", "destroy_instance",
                  "frame_parent", "destroy_frame"} &&
              state.words[23u] == 0u && state.words[24u] == 0u &&
              Byte(state, 4u) == 0u && Byte(state, 6u) == 0u,
          "activation failure skips absent child and clears flags late");
}

void TestStart() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.state = &state;
    pal4::EffectObjectUpdater::Start(state, 0x55u, 0x9000u, runtime);
    Check(runtime.calls.empty() && Byte(state, 88u) == 1u,
          "already started facet has no effects");
    SetByte(state, 6u, 0u);
    SetByte(state, 4u, 0u);
    pal4::EffectObjectUpdater::Start(state, 0x55u, 0x9000u, runtime);
    Check(runtime.calls.empty(), "inactive facet cannot start");

    SetByte(state, 4u, 1u);
    runtime.resource = 1u;
    pal4::EffectObjectUpdater::Start(state, 0x55u, 0x9000u, runtime);
    Check(Byte(state, 6u) == 1u && Byte(state, 7u) == 1u &&
              Byte(state, 88u) == 0x55u &&
              runtime.calls == std::vector<std::string>{
                  "resource", "child", "add_child", "read_owner",
                  "owner_byte", "activate"},
          "loaded instance starts without preparation");

    state = ActiveState();
    runtime.state = &state;
    runtime.calls.clear();
    runtime.resource = 0u;
    SetByte(state, 6u, 0u);
    pal4::EffectObjectUpdater::Start(state, 0x33u, 0x9000u, runtime);
    Check(Byte(state, 88u) == 0x33u &&
              runtime.calls == std::vector<std::string>{
                  "resource", "prepare", "child", "add_child", "read_owner",
                  "owner_byte", "activate"},
          "unloaded instance prepares before activation");
}

void TestStartFailures() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.state = &state;
    runtime.prepare = 0u;
    SetByte(state, 6u, 0u);
    pal4::EffectObjectUpdater::Start(state, 1u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "resource", "prepare", "cleanup", "destroy_instance",
              "frame_parent", "remove_child", "destroy_frame"} &&
              state.words[23u] == 0u && state.words[24u] == 0u &&
              Byte(state, 4u) == 0u && Byte(state, 6u) == 0u &&
              Byte(state, 7u) == 0u,
          "start preparation failure releases instance and frame");

    state = ActiveState();
    runtime.state = &state;
    runtime.calls.clear();
    runtime.prepare = 1u;
    runtime.activate = 0u;
    runtime.child = 0u;
    runtime.frame_parent = 0u;
    state.words[3u] = 0u;
    SetByte(state, 6u, 0u);
    pal4::EffectObjectUpdater::Start(state, 2u, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "resource", "prepare", "child", "activate", "cleanup",
              "destroy_instance", "frame_parent", "destroy_frame"} &&
              Byte(state, 4u) == 0u && Byte(state, 6u) == 0u,
          "start activation failure clears flags before frame release");
}

}  // namespace

int main() {
    TestGatesAndAdvance();
    TestPendingAndSuccess();
    TestFailures();
    TestStart();
    TestStartFailures();
}
