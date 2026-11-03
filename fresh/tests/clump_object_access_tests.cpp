#include "pal4/clump_object.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct AccessRuntime final : pal4::IClumpObjectAccessRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::uint32_t> reads;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return memory.at(address);
    }
};

struct PlaybackRuntime final : pal4::IClumpObjectPlaybackRuntime {
    std::vector<int> events;
    std::int32_t task_value = 8;
    double duration = 2.0;

    void QueryTaskValues(pal4::ClumpObjectState& state,
                         std::uint32_t facet_offset,
                         std::array<std::int32_t, 4>& values) override {
        events.push_back(1);
        Check(facet_offset == 36u &&
                  (state.words[2u] >> 24u) == 1u &&
                  values == std::array<std::int32_t, 4>{},
              "mode byte changes before zeroed task value query");
        values[3u] = task_value;
    }
    double Duration(pal4::ClumpObjectState&) override {
        events.push_back(2);
        return duration;
    }
};

void TestAccess() {
    pal4::ClumpObjectState state{};
    state.words[18u] = 0x1000u;
    AccessRuntime runtime;
    Check(!pal4::ClumpObject::IsActive(state, runtime) &&
              pal4::ClumpObject::ObjectHandle(state, runtime) == 0u &&
              pal4::ClumpObject::Object(state, runtime) == 0u &&
              runtime.reads.empty(),
          "inactive object avoids pointer reads");
    state.words[1u] = 0x00000001u;
    runtime.memory[0x1028u] = 0u;
    Check(!pal4::ClumpObject::IsActive(state, runtime),
          "zero nested count is inactive");
    runtime.memory[0x1028u] = 0xffffffffu;
    Check(!pal4::ClumpObject::IsActive(state, runtime),
          "nested count is compared as signed");
    runtime.memory[0x1028u] = 1u;
    Check(pal4::ClumpObject::IsActive(state, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1028u, 0x1028u, 0x1028u},
          "active state reads only nested count at +40");

    runtime.reads.clear();
    runtime.memory[0x1014u] = 0u;
    Check(pal4::ClumpObject::Object(state, runtime) == 0u &&
              pal4::ClumpObject::ObjectHandle(state, runtime) == 0u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1014u, 0x1014u},
          "null embedded object skips handle dereference");
    runtime.reads.clear();
    runtime.memory[0x1014u] = 0x2000u;
    runtime.memory[0x2004u] = 0xabcdef12u;
    Check(pal4::ClumpObject::Object(state, runtime) == 0x2000u &&
              pal4::ClumpObject::ObjectHandle(state, runtime) ==
                  0xabcdef12u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1014u, 0x1014u, 0x2004u},
          "object and handle follow native nested offsets");
    Check(pal4::ClumpObject::ClassName() == "ClumpObject",
          "class name comes from native vtable method");
}

void TestTimedMode() {
    pal4::ClumpObjectState state{};
    PlaybackRuntime runtime;
    pal4::ClumpObject::SetTimedMode(state, 1u, runtime);
    Check(runtime.events.empty() && state.words[43u] == 0u,
          "inactive object does not change timed mode");

    state.words[1u] = 1u;
    pal4::ClumpObject::SetTimedMode(state, 1u, runtime);
    Check(runtime.events == std::vector<int>{1, 2} &&
              (state.words[2u] >> 24u) == 1u &&
              state.words[43u] == std::bit_cast<std::uint32_t>(4.0f),
          "new mode queries facet +36 and stores task-to-duration ratio");
    runtime.events.clear();
    pal4::ClumpObject::SetTimedMode(state, 0x101u, runtime);
    Check(runtime.events.empty() &&
              state.words[43u] == std::bit_cast<std::uint32_t>(4.0f),
          "identical low byte leaves ratio unchanged");

    pal4::ClumpObject::SetTimedMode(state, 0u, runtime);
    Check(runtime.events.empty() &&
              (state.words[2u] >> 24u) == 0u &&
              state.words[43u] == std::bit_cast<std::uint32_t>(4.0f),
          "clearing mode preserves the previous ratio");
    runtime.task_value = -3;
    pal4::ClumpObject::SetTimedMode(state, 1u, runtime);
    Check(runtime.events == std::vector<int>{1, 2} &&
              state.words[43u] == std::bit_cast<std::uint32_t>(-1.5f),
          "signed task output is converted before division");
}

void TestManagerVectors() {
    pal4::ClumpObjectState state{};
    state.words[18u] = 0x1000u;
    AccessRuntime runtime;
    std::array<std::uint32_t, 3> output{7u, 8u, 9u};
    Check(pal4::ClumpObject::GetPosition(state, output, runtime) == 0u &&
              pal4::ClumpObject::GetRotation(state, output, runtime) == 0u &&
              pal4::ClumpObject::GetScale(state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              runtime.reads.empty(),
          "inactive getters preserve all three outputs");

    state.words[1u] = 1u;
    runtime.memory[0x10a0u] = 0x11111111u;
    runtime.memory[0x10a4u] = 0x22222222u;
    runtime.memory[0x10a8u] = 0x333333a1u;
    runtime.memory[0x10b8u] = 0x44444444u;
    runtime.memory[0x10bcu] = 0x55555555u;
    runtime.memory[0x10c0u] = 0x666666b2u;
    runtime.memory[0x10acu] = 0x77777777u;
    runtime.memory[0x10b0u] = 0x88888888u;
    runtime.memory[0x10b4u] = 0x999999c3u;
    Check(pal4::ClumpObject::GetPosition(state, output, runtime) == 0xc3u &&
              output == std::array<std::uint32_t, 3>{
                  0x77777777u, 0x88888888u, 0x999999c3u} &&
              pal4::ClumpObject::GetRotation(state, output, runtime) == 0xa1u &&
              output == std::array<std::uint32_t, 3>{
                  0x11111111u, 0x22222222u, 0x333333a1u} &&
              pal4::ClumpObject::GetScale(state, output, runtime) == 0xb2u &&
              output == std::array<std::uint32_t, 3>{
                  0x44444444u, 0x55555555u, 0x666666b2u} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x10acu, 0x10b0u, 0x10b4u,
                  0x10a0u, 0x10a4u, 0x10a8u,
                  0x10b8u, 0x10bcu, 0x10c0u},
          "getters read manager position, rotation and scale as raw words");
}

}  // namespace

int main() {
    TestAccess();
    TestTimedMode();
    TestManagerVectors();
}
