#include "pal4/game_object_orientation_updater.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Vector = pal4::BehaviorVector3;

struct Runtime final : pal4::GameObjectOrientationRuntimeStub {
    std::vector<std::string> calls;
    double next_property{2.0};

    double GetDoubleProperty(std::uint32_t manager) override {
        Check(manager == 0x5000u, "manager property source");
        calls.emplace_back("property");
        return next_property;
    }
    float ReadSubjectVectorComponent(std::uint32_t subject,
                                     std::uint32_t slot,
                                     std::uint32_t component) override {
        Check(subject == 0x6000u && component < 3u,
              "subject vector read arguments");
        calls.push_back("read:" + std::to_string(slot) + ":" +
                        std::to_string(component));
        const auto base = slot == 0x18u ? 1u : slot == 0x10u ? 4u
                        : slot == 0u ? 7u : 10u;
        return static_cast<float>(base + component);
    }
    void InitializeMovementAdapter() override {
        calls.emplace_back("adapter");
    }
    void CheckObjectIntersection(Vector& candidate,
        float first, float second, std::uint32_t a,
        std::uint32_t b) override {
        Check(candidate == Vector{1.0f, 2.0f, 3.0f} &&
              first == 0.7f && second == 5.06f &&
              a == 0u && b == 0u,
              "collision receives source vector and cached limits");
        calls.emplace_back("collision");
        candidate = {11.0f, 12.0f, 13.0f};
    }
    void UpdateSubjectPosition(std::uint32_t subject,
                               const Vector& candidate) override {
        Check(subject == 0x6000u &&
              candidate == Vector{11.0f, 12.0f, 13.0f},
              "subject receives corrected vector");
        calls.emplace_back("position");
    }
    void UpdateOrientation(std::uint32_t manager,
        const Vector& first, const Vector& second,
        const Vector& third, const Vector& candidate) override {
        Check(manager == 0x5000u &&
              first == Vector{4.0f, 5.0f, 6.0f} &&
              second == Vector{7.0f, 8.0f, 9.0f} &&
              third == Vector{10.0f, 11.0f, 12.0f} &&
              candidate == Vector{11.0f, 12.0f, 13.0f},
              "orientation receives three bases and corrected point");
        calls.emplace_back("orientation");
    }
};

pal4::GameObjectState Owner() {
    pal4::GameObjectState owner{};
    owner.words[0x24u / 4u] = 0x5000u;
    return owner;
}

}  // namespace

int main() {
    const auto owner = Owner();
    pal4::GameObjectOrientationSubjectState subject{};
    pal4::GameObjectOrientationGlobals globals{};
    Runtime runtime;
    pal4::GameObjectOrientationUpdater::UpdateVtable3(
        owner, subject, 0x6000u, globals, runtime);
    Check(globals.guard == 3u && globals.first_limit == 0.7f &&
          globals.second_limit == 5.06f &&
          runtime.calls == std::vector<std::string>{
              "property", "property"},
          "first inactive update initializes both independent caches");

    runtime.calls.clear();
    runtime.next_property = 99.0;
    pal4::GameObjectOrientationUpdater::UpdateVtable3(
        owner, subject, 0x6000u, globals, runtime);
    Check(runtime.calls.empty() && globals.first_limit == 0.7f &&
          globals.second_limit == 5.06f,
          "cached limits are reused on inactive calls");

    globals.guard = 1u;
    runtime.calls.clear();
    runtime.next_property = 2.0;
    pal4::GameObjectOrientationUpdater::UpdateVtable3(
        owner, subject, 0x6000u, globals, runtime);
    Check(runtime.calls == std::vector<std::string>{"property"} &&
          globals.guard == 3u && globals.first_limit == 0.7f,
          "partially initialized guard computes only the missing scale");

    subject.bytes[0xb9u] = 1u;
    runtime.calls.clear();
    pal4::GameObjectOrientationUpdater::UpdateVtable3(
        owner, subject, 0x6000u, globals, runtime);
    Check(globals.candidate == Vector{11.0f, 12.0f, 13.0f} &&
          globals.first_basis == Vector{4.0f, 5.0f, 6.0f} &&
          globals.second_basis == Vector{7.0f, 8.0f, 9.0f} &&
          globals.third_basis == Vector{10.0f, 11.0f, 12.0f},
          "global vector caches reflect each virtual read");
    Check(runtime.calls == std::vector<std::string>{
              "read:24:0", "read:24:1", "read:24:2", "adapter",
              "collision", "position", "read:16:0", "read:16:1",
              "read:16:2", "read:0:0", "read:0:1", "read:0:2",
              "read:8:0", "read:8:1", "read:8:2", "orientation"},
          "active path preserves twelve virtual reads and call order");

    auto owner_four = Owner();
    owner_four.words[0x24u / 4u] = 0xdeadbeefu;
    owner_four.words[0x1cu / 4u] = 0x5000u;
    pal4::GameObjectOrientationGlobals globals_four{};
    runtime.calls.clear();
    pal4::GameObjectOrientationUpdater::UpdateVtable4(
        owner_four, subject, 0x6000u, globals_four, runtime);
    Check(globals_four.guard == 3u &&
          globals_four.candidate == Vector{11.0f, 12.0f, 13.0f} &&
          runtime.calls.size() == 18u &&
          runtime.calls[0] == "property" &&
          runtime.calls[1] == "property" &&
          runtime.calls.back() == "orientation" &&
          globals.guard == 3u,
          "Vtable 4 uses +0x1c manager and independent caches");

    auto owner_five = Owner();
    owner_five.words[0x24u / 4u] = 0xdeadbeefu;
    owner_five.words[0x1cu / 4u] = 0xdeadbeefu;
    owner_five.words[0x0cu / 4u] = 0x5000u;
    pal4::GameObjectOrientationGlobals globals_five{};
    runtime.calls.clear();
    pal4::GameObjectOrientationUpdater::UpdateVtable5(
        owner_five, subject, 0x6000u, globals_five, runtime);
    Check(globals_five.guard == 3u &&
          globals_five.candidate == Vector{11.0f, 12.0f, 13.0f} &&
          runtime.calls.size() == 18u &&
          runtime.calls[0] == "property" &&
          runtime.calls[1] == "property" &&
          runtime.calls.back() == "orientation" &&
          globals.guard == 3u && globals_four.guard == 3u,
          "Vtable 5 uses +0x0c manager and a third independent cache");

    auto owner_six = Owner();
    owner_six.words[0x24u / 4u] = 0xdeadbeefu;
    owner_six.words[0x1cu / 4u] = 0xdeadbeefu;
    owner_six.words[0x0cu / 4u] = 0xdeadbeefu;
    owner_six.words[0x04u / 4u] = 0x5000u;
    pal4::GameObjectOrientationGlobals globals_six{};
    runtime.calls.clear();
    pal4::GameObjectOrientationUpdater::UpdateVtable6(
        owner_six, subject, 0x6000u, globals_six, runtime);
    Check(globals_six.guard == 3u &&
          globals_six.candidate == Vector{11.0f, 12.0f, 13.0f} &&
          runtime.calls.size() == 18u &&
          runtime.calls[0] == "property" &&
          runtime.calls[1] == "property" &&
          runtime.calls.back() == "orientation" &&
          globals.guard == 3u && globals_four.guard == 3u &&
          globals_five.guard == 3u,
          "Vtable 6 uses +0x04 manager and a fourth independent cache");
}
