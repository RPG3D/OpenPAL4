#include "pal4/fade_off_task.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x5000u;
constexpr std::uint32_t kActor = 0x7100u;
constexpr std::uint32_t kFacet = 0x8000u;

struct Runtime final : pal4::IFadeOffTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::uint32_t facet = 0u;
    std::uint32_t state_actor = 0u;
    std::uint32_t state = 0u;
    std::uint32_t state_mode = 0u;
    std::uint32_t color = 0u;
    std::uint32_t color_facet = 0u;
    std::uint32_t color_result = 0x12345678u;
    float converted = 0.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write_word");
        words[address] = value;
        if (address == kTask + 64u && value == 0u)
            floats[address] = 0.0f;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read_float");
        return floats[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.emplace_back("write_float");
        floats[address] = value;
    }
    void SetActorState(std::uint32_t actor, std::uint32_t new_state,
                       std::uint32_t mode) override {
        calls.emplace_back("state");
        state_actor = actor;
        state = new_state;
        state_mode = mode;
    }
    std::int32_t FloatToLong(float value) override {
        calls.emplace_back("ftol");
        converted = value;
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t ActorFacet(std::uint32_t actor) override {
        calls.emplace_back("facet");
        Check(actor == kActor, "facet lookup receives actor");
        return facet;
    }
    std::uint32_t SetFacetColor(std::uint32_t target,
                                std::uint32_t new_color) override {
        calls.emplace_back("color");
        color_facet = target;
        color = new_color;
        return color_result;
    }
};

void StartClampsAndInitializes() {
    for (const float duration : {0.05f, 0.5f}) {
        Runtime runtime;
        runtime.words[kTask + 60u] = kActor;
        runtime.floats[kTask + 20u] = duration;
        Check(pal4::FadeOffTask::Start(kTask, runtime) == 1u &&
              runtime.floats[kTask + 20u] ==
                  (duration < 0.1f ? 0.1f : duration) &&
              runtime.floats[kTask + 64u] == 255.0f &&
              runtime.floats[kTask + 68u] == 512.0f &&
              runtime.floats[kTask + 72u] == 1.0f &&
              std::fabs(runtime.floats[kTask + 76u] -
                        200.0f / runtime.floats[kTask + 20u]) < 0.001f &&
              runtime.state_actor == kActor && runtime.state == 3u &&
              runtime.state_mode == 1u && runtime.calls.back() == "state",
              "fade start clamps duration, seeds values and sets actor state");
    }
}

void AlphaWithFacet() {
    Runtime runtime;
    runtime.words[kTask + 60u] = kActor;
    runtime.floats[kTask + 64u] = 255.0f;
    runtime.floats[kTask + 68u] = 512.0f;
    runtime.floats[kTask + 72u] = 1.0f;
    runtime.floats[kTask + 76u] = 400.0f;
    runtime.facet = kFacet;
    Check(pal4::FadeOffTask::UpdateAlpha(kTask, 0.1f, runtime) ==
              0x12345601u &&
          std::fabs(runtime.floats[kTask + 64u] - 203.8f) < 0.0001f &&
          runtime.converted == runtime.floats[kTask + 64u] &&
          runtime.color_facet == kFacet && runtime.color == 0xcbffffffu &&
          runtime.floats[kTask + 72u] == 41.0f &&
          runtime.calls.back() == "write_float",
          "facet receives truncated alpha and progress advances");
}

void AlphaWithoutFacetAndClamp() {
    Runtime runtime;
    runtime.words[kTask + 60u] = kActor;
    runtime.floats[kTask + 64u] = 20.0f;
    runtime.floats[kTask + 68u] = 512.0f;
    runtime.floats[kTask + 72u] = 1.0f;
    Check(pal4::FadeOffTask::UpdateAlpha(kTask, 0.1f, runtime) ==
              ((kActor & 0xffffff00u) | 1u) &&
          runtime.floats[kTask + 64u] == 0.0f &&
          runtime.converted == 0.0f &&
          runtime.floats[kTask + 72u] == 1.0f &&
          runtime.color_facet == 0u &&
          runtime.calls.back() == "facet",
          "negative alpha clamps and missing facet leaves progress unchanged");
}

}  // namespace

int main() {
    try {
        StartClampsAndInitializes();
        AlphaWithFacet();
        AlphaWithoutFacetAndClamp();
        std::cout << "fade_off_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
