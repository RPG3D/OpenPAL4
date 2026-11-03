#include "pal4/game_object_animation_event_array.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationEventCopyRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> values;
    std::vector<std::string> calls;

    void DestroyEvent(std::uint32_t address) override {
        calls.emplace_back("destroy:" + std::to_string(address));
    }
    void AssignEvent(std::uint32_t destination,
                     std::uint32_t source) override {
        Check(values.contains(source), "copy source exists");
        calls.emplace_back("assign:" + std::to_string(destination) + ":" +
                           std::to_string(source));
        values[destination] = values.at(source);
    }
};

void ForwardAndFill() {
    Runtime runtime;
    runtime.values[0x2000u] = 1;
    runtime.values[0x214cu] = 2;
    runtime.values[0x2298u] = 3;
    Check(pal4::GameObjectAnimationEventArray::CopyRangeForward(
              0x2000u, 0x2000u, 0x3000u, runtime) == 0x3000u &&
              runtime.calls.empty(), "empty forward range returns destination");
    Check(pal4::GameObjectAnimationEventArray::CopyRangeForward(
              0x2000u, 0x23e4u, 0x3000u, runtime) == 0x33e4u &&
              runtime.calls == std::vector<std::string>{
                  "assign:12288:8192", "assign:12620:8524",
                  "assign:12952:8856"} &&
              runtime.values[0x3000u] == 1 &&
              runtime.values[0x314cu] == 2 &&
              runtime.values[0x3298u] == 3,
          "forward helper copies every event and returns destination end");

    runtime.calls.clear();
    runtime.values[0x9000u] = 9;
    pal4::GameObjectAnimationEventArray::FillRange(
        0x4000u, 0x4000u + 3u * 0x14cu, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "assign:16384:36864", "assign:16716:36864",
              "assign:17048:36864"} &&
              runtime.values[0x4000u] == 9 &&
              runtime.values[0x414cu] == 9 &&
              runtime.values[0x4298u] == 9,
          "fill helper assigns one template across the range");
}

void BackwardAndInvalidRanges() {
    Runtime runtime;
    runtime.values[0x2000u] = 1;
    runtime.values[0x214cu] = 2;
    runtime.values[0x2298u] = 3;
    Check(pal4::GameObjectAnimationEventArray::CopyRangeBackward(
              0x2000u, 0x2000u, 0x3000u, runtime) == 0x3000u &&
              runtime.calls.empty(), "empty backward range returns destination");
    Check(pal4::GameObjectAnimationEventArray::CopyRangeBackward(
              0x2000u, 0x2298u, 0x23e4u, runtime) == 0x214cu &&
              runtime.calls == std::vector<std::string>{
                  "assign:8856:8524", "assign:8524:8192"} &&
              runtime.values[0x2000u] == 1 &&
              runtime.values[0x214cu] == 1 &&
              runtime.values[0x2298u] == 2,
          "backward copy preserves overlapping source values");

    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventArray::CopyRangeForward(
              0x214cu, 0x2000u, 0x3000u, runtime) == 0x3000u &&
              pal4::GameObjectAnimationEventArray::CopyRangeBackward(
                  0x2000u, 0x2001u, 0x3000u, runtime) == 0x3000u,
          "invalid copy ranges return destination without calls");
    pal4::GameObjectAnimationEventArray::FillRange(
        0x4000u, 0x4001u, 0x9000u, runtime);
    Check(runtime.calls.empty(), "invalid fill range is inert");
    pal4::GameObjectAnimationEventCopyRuntimeStub stub;
    pal4::GameObjectAnimationEventArray::FillRange(
        0x4000u, 0x414cu, 0x9000u, stub);
}

}  // namespace

int main() {
    ForwardAndFill();
    BackwardAndInvalidRanges();
    std::cout << "game_object_animation_event_copy_tests passed\n";
}
