#include "pal4/pal_actor_control_effect_selector.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlEffectSelectorRuntime {
    std::vector<pal4::ActorControlEffectPoint> vertices;
    std::vector<std::string> events;
    pal4::ActorControlEffectPoint actor{1.0f, 2.0f, 3.0f};
    float tolerance = 1.0f;
    std::uint32_t selected = 0u;

    pal4::ActorControlEffectPoint Vertex(
        pal4::ActorControlEffectObject&, std::int32_t index) override {
        events.emplace_back("vertex " + std::to_string(index));
        return vertices.at(static_cast<std::size_t>(index));
    }
    pal4::ActorControlEffectPoint ActorPosition() override {
        events.emplace_back("actor");
        return actor;
    }
    float VerticalTolerance() override {
        events.emplace_back("tolerance");
        return tolerance;
    }
    float Magnitude(
        pal4::ActorControlEffectPoint offset) override {
        Check(offset.y == 0.0f, "distance uses planar offset");
        events.emplace_back("distance");
        return IActorControlEffectSelectorRuntime::Magnitude(offset);
    }
    void SelectEffect(pal4::ActorControlEffectObject&) override {
        events.emplace_back("select");
        ++selected;
    }
};

pal4::ActorControlEffectObject ReadyEffect(std::uint32_t count) {
    pal4::ActorControlEffectObject effect{};
    effect.words[1] = 5u;
    effect.words[19] = 1u;
    effect.words[21] = 0x1000u;
    effect.words[22] = 0x1000u + count * 80u;
    return effect;
}

void TestGates() {
    Runtime runtime;
    float best = 100.0f;
    auto effect = ReadyEffect(1u);
    effect.words[1] = 4u;
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect && runtime.events.empty(),
          "type gate returns original pointer");
    effect.words[1] = 5u;
    effect.words[19] = 0u;
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect && runtime.events.empty(),
          "enabled gate prevents traversal");
    effect.words[19] = 1u;
    effect.words[21] = 0u;
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect && runtime.events.empty(),
          "missing vertex range prevents traversal");
    effect = ReadyEffect(0u);
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect && runtime.events.empty(),
          "empty vertex range prevents traversal");
}

void TestNearestPlanarVertex() {
    Runtime runtime;
    runtime.vertices = {
        {2.0f, 4.0f, 3.0f}, // vertical distance excludes this vertex
        {4.0f, 2.0f, 7.0f}, // planar magnitude5
        {1.0f, 3.0f, 5.0f}, // planar magnitude2
    };
    auto effect = ReadyEffect(3u);
    float best = 30.0f;
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect && best == 2.0f &&
              runtime.selected == 1u &&
              runtime.events == std::vector<std::string>{
                  "vertex 0", "actor", "tolerance",
                  "vertex 1", "actor", "tolerance", "distance",
                  "vertex 2", "actor", "tolerance", "distance",
                  "select"},
          "closest qualifying vertex updates threshold and selects once");
}

void TestStrictComparisonAndUnorderedHeight() {
    Runtime runtime;
    runtime.vertices = {{1.0f, 2.0f, 8.0f}};
    auto effect = ReadyEffect(1u);
    float best = 5.0f;
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect,
          "equal-distance callback returns original pointer");
    Check(best == 5.0f && runtime.selected == 0u,
          "equal magnitude does not select");
    runtime.events.clear();
    runtime.tolerance = std::numeric_limits<float>::quiet_NaN();
    best = 6.0f;
    Check(pal4::PalActorControlEffectSelector::Visit(
              effect, best, runtime) == &effect,
          "unordered-height callback returns original pointer");
    Check(best == 5.0f && runtime.selected == 1u,
          "unordered height comparison follows x87 processing branch");
}

void TestOriginalGuestBedRange() {
    Runtime runtime;
    runtime.actor={-12.8032846f,-0.2f,-24.8555794f};
    runtime.tolerance=78.4972916f;
    runtime.vertices={{-24.8961163f,-0.2000427f,-62.7652512f},
                      {-26.1361198f,20.1999588f,-67.6952057f}};
    auto effect=ReadyEffect(2u);
    float best=78.4972916f;
    (void)pal4::PalActorControlEffectSelector::Visit(effect,best,runtime);
    Check(runtime.selected==1u && best>39.0f && best<40.0f,
        "original guest bed must be selected within78-unit range;668530 returns magnitude, not squared magnitude");
}

}  // namespace

int main() {
    TestOriginalGuestBedRange();
    TestGates();
    TestNearestPlanarVertex();
    TestStrictComparisonAndUnorderedHeight();
}
