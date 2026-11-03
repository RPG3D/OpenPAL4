#include "pal4/dead_effect_task.h"

#include <array>
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

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kFacet = 0x6000u;

struct Runtime final : pal4::IDeadEffectTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 60u, kActor}};
    std::map<std::uint32_t, float> floats{{kFacet + 620u, 60.0f}};
    std::vector<std::string> calls;
    std::uint32_t facet = 0u;
    std::array<float, 3> scale{};
    std::uint32_t position = 0u;
    std::uint8_t position_mode = 255u;
    std::uint8_t scale_mode = 255u;
    bool throw_on_scale = false;
    int resets = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == kFacet + 620u, "facet scale at +620");
        calls.emplace_back("read_float");
        return floats[address];
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat");
        return 0x7000u;
    }
    std::uint32_t CombatCamera(std::uint32_t combat) override {
        Check(combat == 0x7000u, "camera receives combat singleton");
        calls.emplace_back("camera");
        return 0x7100u;
    }
    pal4::DeadEffectHandle CreateEffect(
        std::uint32_t camera, std::uint32_t name,
        std::uint32_t origin, std::uint32_t mode,
        std::uint32_t flags) override {
        Check(camera == 0x7100u &&
              name == pal4::DeadEffectTask::kEffectName &&
              origin == pal4::DeadEffectTask::kEffectPosition &&
              mode == 1u && flags == 0u,
              "EFF6 effect creation arguments");
        calls.emplace_back("create");
        return {0x11u, 0x22u};
    }
    std::uint32_t ChildPosition(std::uint32_t actor) override {
        Check(actor == kActor, "effect position uses task actor");
        calls.emplace_back("child_position");
        return 0x7200u;
    }
    std::uint32_t ActorFacet(std::uint32_t actor) override {
        Check(actor == kActor, "facet uses task actor");
        calls.emplace_back("facet");
        return facet;
    }
    void UpdatePosition(const pal4::DeadEffectHandle& effect,
                        std::uint32_t source,
                        std::uint8_t mode) override {
        Check(effect.first == 0x11u && effect.second == 0x22u,
              "position receives effect handle");
        calls.emplace_back("position");
        position = source;
        position_mode = mode;
    }
    void UpdateScale(const pal4::DeadEffectHandle& effect,
                     const std::array<float, 3>& value,
                     std::uint8_t mode) override {
        Check(effect.first == 0x11u, "scale receives effect handle");
        calls.emplace_back("scale");
        scale = value;
        scale_mode = mode;
        if (throw_on_scale) throw std::runtime_error("scale failure");
    }
    void FinalizeEffect(const pal4::DeadEffectHandle& effect) override {
        Check(effect.second == 0x22u, "finalize receives effect handle");
        calls.emplace_back("finalize");
    }
    void ResetTemporaryEffect(pal4::DeadEffectHandle& effect) override {
        Check(effect.first == 0x11u && effect.second == 0x22u,
              "reset receives temporary effect");
        calls.emplace_back("reset");
        ++resets;
    }
};

void DefaultScale() {
    Runtime runtime;
    Check(pal4::DeadEffectTask::Process(kTask, runtime) == 1u &&
          runtime.position == 0x7200u && runtime.position_mode == 0u &&
          runtime.scale == std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
          runtime.scale_mode == 0u && runtime.resets == 1 &&
          runtime.calls.front() == "combat" &&
          runtime.calls.back() == "reset",
          "dead effect uses unit scale without actor facet");
}

void FacetScaleAndException() {
    Runtime runtime;
    runtime.facet = kFacet;
    Check(pal4::DeadEffectTask::Process(kTask, runtime) == 1u &&
          runtime.scale == std::array<float, 3>{1.5f, 1.5f, 1.5f} &&
          runtime.resets == 1,
          "facet scale applies multiplier to all coordinates");

    Runtime failing;
    failing.facet = kFacet;
    failing.throw_on_scale = true;
    bool threw = false;
    try {
        (void)pal4::DeadEffectTask::Process(kTask, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.resets == 1 &&
          failing.calls.back() == "reset",
          "temporary effect resets when update fails");

    Runtime wrapped;
    wrapped.facet = 0xfffffda0u;
    Check(pal4::DeadEffectTask::Process(kTask, wrapped) == 1u &&
          wrapped.scale == std::array<float, 3>{1.0f, 1.0f, 1.0f},
          "wrapped facet plus 608 skips scale source");
}

}  // namespace

int main() {
    try {
        DefaultScale();
        FacetScaleAndException();
        std::cout << "dead_effect_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
