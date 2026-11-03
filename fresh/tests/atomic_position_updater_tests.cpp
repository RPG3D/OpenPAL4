#include "pal4/atomic_position_updater.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IAtomicPositionRuntime {
    static constexpr std::uint32_t atomic = 0x1000u;
    static constexpr std::uint32_t destination = 0x2000u;
    static constexpr std::uint32_t object_transform = 0x3000u;
    static constexpr std::uint32_t parent_transform = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    pal4::AtomicVector3 observed_delta{};
    bool direct{true};
    bool initial{};

    Runtime() {
        words[atomic + 4u] = 0x5000u;
        words[atomic + 60u] = 0x6000u;
        words[0x6000u + 4u] = 0x7000u;
        for (std::uint32_t index = 0u; index < 16u; ++index)
            words[object_transform + index * 4u] = index + 1u;
        WriteSphere(atomic + 28u, {4.0f, 0.0f, 0.0f, 2.0f});
        WriteSphere(destination, {0.0f, 0.0f, 0.0f, 1.0f});
        events.clear();
    }
    void WriteSphere(const std::uint32_t address,
                     const pal4::AtomicSphere& sphere) {
        for (std::uint32_t index = 0u; index < 4u; ++index)
            words[address + index * 4u] =
                std::bit_cast<std::uint32_t>(sphere[index]);
    }
    [[nodiscard]] pal4::AtomicSphere SphereAt(
        const std::uint32_t address) {
        pal4::AtomicSphere sphere{};
        for (std::uint32_t index = 0u; index < 4u; ++index)
            sphere[index] = std::bit_cast<float>(ReadWord(address + index * 4u));
        return sphere;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check((address >= destination && address < destination + 16u) ||
                  address == destination + 76u,
              "writes destination sphere or copied camera field");
        words[address] = value;
        events.emplace_back(address == destination + 76u ?
                            "write-camera" : "write-sphere");
    }
    void ResyncInterpolatedSphere(const std::uint32_t opened) override {
        Check(opened == atomic, "resyncs the given atomic");
        words[atomic + 28u] = std::bit_cast<std::uint32_t>(6.0f);
        events.emplace_back("resync");
    }
    [[nodiscard]] bool UseDirectTransform() override {
        events.emplace_back("mode");
        return direct;
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t object) override {
        if (object == 0x5000u) {
            events.emplace_back("object-transform");
            return object_transform;
        }
        Check(object == 0x7000u, "parent frame comes from atomic +60/+4");
        events.emplace_back("parent-transform");
        return parent_transform;
    }
    void SetupTransformMatrix(
        pal4::AtomicMatrix& destination_matrix,
        const std::uint32_t source) override {
        Check(source == parent_transform, "sets up parent matrix");
        destination_matrix.fill(0x33u);
        events.emplace_back("setup");
    }
    void MultiplyMatrices(
        pal4::AtomicMatrix& destination_matrix,
        const std::uint32_t source,
        const pal4::AtomicMatrix& parent) override {
        Check(source == object_transform && parent.front() == 0x33u,
              "combines object and parent matrices");
        destination_matrix.fill(0x44u);
        events.emplace_back("multiply");
    }
    void TransformSphere(
        pal4::AtomicSphere& sphere,
        const pal4::AtomicMatrix& matrix) override {
        Check((direct && matrix.front() == 1u && matrix.back() == 16u) ||
                  (!direct && matrix.front() == 0x44u),
              "transforms with selected 64-byte matrix");
        if (!direct) sphere[0u] += 2.0f;
        events.emplace_back("transform-sphere");
    }
    void CalculatePosition(const std::uint32_t output,
                           const std::uint32_t input,
                           const std::uint32_t transform) override {
        Check(output == destination + 64u &&
                  input == destination + 48u &&
                  transform == object_transform,
              "calculates camera position from source at +48 into +64");
        events.emplace_back("calculate-position");
    }
    [[nodiscard]] bool IsInitialSphere() override {
        events.emplace_back("initial-check");
        return initial;
    }
    void ClearInitialSphere() override {
        initial = false;
        events.emplace_back("clear-initial");
    }
    [[nodiscard]] float Length(
        const pal4::AtomicVector3& vector) override {
        observed_delta = vector;
        events.emplace_back("length");
        return std::sqrt(vector[0u] * vector[0u] +
                         vector[1u] * vector[1u] +
                         vector[2u] * vector[2u]);
    }
};

using Events = std::vector<std::string>;

void TestInitialSphereAndResync() {
    Runtime runtime;
    runtime.initial = true;
    runtime.words[Runtime::atomic + 76u] = 2u;
    Check(pal4::AtomicPositionUpdater::Update(Runtime::atomic,
                                               Runtime::destination,
                                               runtime) == Runtime::atomic &&
              runtime.SphereAt(Runtime::destination) ==
                  pal4::AtomicSphere({6.0f, 0.0f, 0.0f, 2.0f}) &&
              !runtime.initial &&
              runtime.events == Events({"resync", "mode", "object-transform",
                                        "transform-sphere", "initial-check",
                                        "write-sphere", "write-sphere",
                                        "write-sphere", "write-sphere",
                                        "clear-initial"}),
          "resync runs before source copy and first sphere replaces output");
}

void TestContainedSphere() {
    Runtime runtime;
    runtime.WriteSphere(Runtime::atomic + 28u,
                        {1.0f, 0.0f, 0.0f, 1.0f});
    runtime.WriteSphere(Runtime::destination,
                        {0.0f, 0.0f, 0.0f, 5.0f});
    Check(pal4::AtomicPositionUpdater::Update(Runtime::atomic,
                                               Runtime::destination,
                                               runtime) == Runtime::atomic &&
              runtime.SphereAt(Runtime::destination) ==
                  pal4::AtomicSphere({0.0f, 0.0f, 0.0f, 5.0f}) &&
              runtime.observed_delta ==
                  pal4::AtomicVector3({1.0f, 0.0f, 0.0f}) &&
              runtime.events == Events({"mode", "object-transform",
                                        "transform-sphere", "initial-check",
                                        "length"}),
          "contained sphere returns before writes");
}

void TestExpansionAndParentMatrix() {
    Runtime direct;
    Check(pal4::AtomicPositionUpdater::Update(Runtime::atomic,
                                               Runtime::destination,
                                               direct) == Runtime::atomic &&
              direct.SphereAt(Runtime::destination) ==
                  pal4::AtomicSphere({2.5f, 0.0f, 0.0f, 3.5f}) &&
              direct.observed_delta ==
                  pal4::AtomicVector3({4.0f, 0.0f, 0.0f}) &&
              direct.events == Events({"mode", "object-transform",
                                       "transform-sphere", "initial-check",
                                       "length", "write-sphere", "write-sphere",
                                       "write-sphere", "write-sphere"}),
          "sphere expands center and radius by distance and radius delta");

    Runtime parent;
    parent.direct = false;
    Check(pal4::AtomicPositionUpdater::Update(Runtime::atomic,
                                               Runtime::destination,
                                               parent) == Runtime::atomic &&
              parent.SphereAt(Runtime::destination) ==
                  pal4::AtomicSphere({3.5f, 0.0f, 0.0f, 4.5f}) &&
              parent.events == Events({"mode", "object-transform",
                                       "parent-transform", "setup", "multiply",
                                       "transform-sphere", "initial-check",
                                       "length", "write-sphere", "write-sphere",
                                       "write-sphere", "write-sphere"}),
          "parent path composes matrix before transformed sphere merge");
}

void TestCameraPositionCopy() {
    Runtime missing;
    Check(pal4::AtomicPositionUpdater::UpdateCameraPosition(
              Runtime::destination, 0u, missing) == 0u &&
              missing.events.empty(), "null atomic leaves camera record alone");

    Runtime runtime;
    runtime.words[Runtime::destination + 60u] = 0x12345678u;
    Check(pal4::AtomicPositionUpdater::UpdateCameraPosition(
              Runtime::destination, Runtime::atomic, runtime) == 0x12345678u &&
              runtime.ReadWord(Runtime::destination + 76u) == 0x12345678u &&
              runtime.events == Events({"object-transform",
                                        "calculate-position", "write-camera"}),
          "camera calculation precedes field copy and return");
}

void TestCallbackContextSphere() {
    Runtime first;
    std::uint32_t initial = 1;
    first.words[Runtime::atomic + 76u] = 2u;
    Check(pal4::AtomicPositionUpdater::ProcessBoundingSphere(
              Runtime::atomic, 1, initial, Runtime::destination,
              first) == Runtime::atomic &&
          initial == 0 &&
          first.SphereAt(Runtime::destination) ==
              pal4::AtomicSphere({6.0f, 0.0f, 0.0f, 2.0f}) &&
          first.events == Events({"resync", "object-transform",
              "transform-sphere", "write-sphere", "write-sphere",
              "write-sphere", "write-sphere"}),
          "callback initial sphere uses context and clears flag");

    Runtime parent;
    parent.direct = false;
    initial = 0;
    Check(pal4::AtomicPositionUpdater::ProcessBoundingSphere(
              Runtime::atomic, 0, initial, Runtime::destination,
              parent) == Runtime::atomic &&
          parent.SphereAt(Runtime::destination) ==
              pal4::AtomicSphere({3.5f, 0.0f, 0.0f, 4.5f}) &&
          parent.events == Events({"object-transform", "parent-transform",
              "setup", "multiply", "transform-sphere", "length",
              "write-sphere", "write-sphere", "write-sphere", "write-sphere"}),
          "callback existing sphere combines parent transform");
}

void TestCallbackRadiusAdditionOrder() {
    Runtime callback;
    callback.WriteSphere(Runtime::atomic + 28u,
        {4.0f, 0.0f, 0.0f, std::bit_cast<float>(0x3e9f6fd2u)});
    callback.WriteSphere(Runtime::destination,
        {0.0f, 0.0f, 0.0f, std::bit_cast<float>(0x4002fb7fu)});
    Runtime ordinary = callback;
    std::uint32_t initial = 0;
    (void)pal4::AtomicPositionUpdater::ProcessBoundingSphere(
        Runtime::atomic, 1u, initial, Runtime::destination, callback);
    (void)pal4::AtomicPositionUpdater::Update(
        Runtime::atomic, Runtime::destination, ordinary);
    Check(callback.words[Runtime::destination + 12u] == 0x404b74bdu &&
              ordinary.words[Runtime::destination + 12u] == 0x404b74bcu,
          "667C60 and5F9A40 preserve their distinct PC24 radius addition order");
}

}  // namespace

int main() {
    TestInitialSphereAndResync();
    TestContainedSphere();
    TestExpansionAndParentMatrix();
    TestCameraPositionCopy();
    TestCallbackContextSphere();
    TestCallbackRadiusAdditionOrder();
    pal4::AtomicPositionRuntimeStub stub;
    Check(pal4::AtomicPositionUpdater::Update(0x1000u, 0x2000u, stub) ==
              0x1000u,
          "RenderWare stub links and runs the initial-sphere path");
    return EXIT_SUCCESS;
}
