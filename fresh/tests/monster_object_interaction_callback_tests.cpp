#include "pal4/monster_object_interaction_callback.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterObjectInteractionRuntime {
    std::vector<std::string> calls;
    pal4::MonsterInteractionContext context{};
    pal4::MonsterInteractionObject candidate{};
    double target_property{1.5};
    double distance{5.0};
    float radius{5.0f};
    std::uint32_t clump{0x9000u};
    bool blocked{};
    float observed_dx{};
    float observed_dz{};
    std::array<float, 6> observed_endpoints{};
    std::uint32_t entered_candidate{};
    std::uint32_t entered_target{};

    Runtime() {
        context.position = {1.0f, 2.0f, 3.0f};
        context.collision_position = {4.0f, 5.0f, 6.0f};
        context.target_address = 0x7000u;
        context.source_address = 0x8000u;
        candidate.position = {4.0f, 4.0f, 7.0f};
        candidate.collision_position = {8.0f, 9.0f, 10.0f};
    }

    pal4::MonsterInteractionContext Context(std::uint32_t address) override {
        Check(address == 0x5000u, "manager context address");
        calls.emplace_back("context");
        return context;
    }
    pal4::MonsterInteractionObject Candidate(
        std::uint32_t address) override {
        Check(address == 0x6000u, "candidate address");
        calls.emplace_back("candidate");
        return candidate;
    }
    double TargetDoubleProperty(std::uint32_t address) override {
        Check(address == 0x7000u, "target property address");
        calls.emplace_back("target-property");
        return target_property;
    }
    double PlanarDistance(float dx, float dz) override {
        calls.emplace_back("distance");
        observed_dx = dx;
        observed_dz = dz;
        return distance;
    }
    float InteractionRadius(std::uint32_t source) override {
        Check(source == 0x8000u, "source config address");
        calls.emplace_back("radius");
        return radius;
    }
    std::uint32_t CollisionClump() override {
        calls.emplace_back("clump");
        return clump;
    }
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t address) override {
        Check(address == clump, "collision clump address");
        calls.emplace_back("collision");
        observed_endpoints = endpoints;
        return blocked;
    }
    void EnterPursuit(std::uint32_t candidate_address,
                      std::uint32_t target_address) override {
        calls.emplace_back("enter");
        entered_candidate = candidate_address;
        entered_target = target_address;
    }
};

void IdentityAndVerticalGates() {
    Runtime same;
    Check(pal4::MonsterObjectInteractionCallback::Process(
              0x5000u, 0x5000u, same) == 0x5000u && same.calls.empty(),
          "identical candidate returns before runtime reads");

    for (const auto property : {1.0, std::numeric_limits<double>::quiet_NaN()}) {
        Runtime runtime;
        runtime.target_property = property;
        Check(pal4::MonsterObjectInteractionCallback::Process(
                  0x6000u, 0x5000u, runtime) == 0x6000u &&
                  runtime.calls == std::vector<std::string>{
                      "context", "candidate", "target-property",
                      "distance", "radius"} &&
                  runtime.observed_dx == 3.0f &&
                  runtime.observed_dz == 4.0f,
              "equal or unordered vertical reach still measures distance");
    }
}

void DistanceAndCollisionGates() {
    for (const auto distance : {5.01,
             std::numeric_limits<double>::quiet_NaN()}) {
        Runtime runtime;
        runtime.distance = distance;
        Check(pal4::MonsterObjectInteractionCallback::Process(
                  0x6000u, 0x5000u, runtime) == 0x6000u &&
                  runtime.calls == std::vector<std::string>{
                      "context", "candidate", "target-property",
                      "distance", "radius"},
              "outside or unordered planar range skips collision");
    }
    Runtime unordered_radius;
    unordered_radius.radius = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::MonsterObjectInteractionCallback::Process(
              0x6000u, 0x5000u, unordered_radius) == 0x6000u &&
              unordered_radius.calls.back() == "radius",
          "unordered radius skips collision");

    Runtime blocked;
    blocked.blocked = true;
    Check(pal4::MonsterObjectInteractionCallback::Process(
              0x6000u, 0x5000u, blocked) == 0x6000u &&
              blocked.calls.back() == "collision" &&
              blocked.observed_endpoints == std::array<float, 6>{
                  4.0f, 5.0f, 6.0f, 8.0f, 9.0f, 10.0f},
          "blocked clump path preserves endpoint order and skips transition");
}

void ClearPathAndStub() {
    Runtime runtime;
    Check(pal4::MonsterObjectInteractionCallback::Process(
              0x6000u, 0x5000u, runtime) == 0x6000u &&
              runtime.entered_candidate == 0x6000u &&
              runtime.entered_target == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "context", "candidate", "target-property", "distance",
                  "radius", "clump", "collision", "enter"},
          "equal planar limit and clear collision start candidate pursuit");
    pal4::MonsterObjectInteractionRuntimeStub stub;
    Check(pal4::MonsterObjectInteractionCallback::Process(
              0x6000u, 0x5000u, stub) == 0x6000u,
          "external object and collision services have a linkable stub");
}

}  // namespace

int main() {
    IdentityAndVerticalGates();
    DistanceAndCollisionGates();
    ClearPathAndStub();
    std::cout << "monster interaction callback cases passed\n";
}
