#include "pal4/pal_actor_control_transform.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlTransformRuntime {
    std::vector<std::string> calls;
    pal4::ActorControlMatrix CameraMatrix(std::uint32_t camera) override {
        Check(camera == 0x1000u, "camera matrix address");
        calls.emplace_back("camera matrix");
        pal4::ActorControlMatrix matrix{};
        matrix.words[0] = 0x100u;
        return matrix;
    }
    pal4::ActorControlMatrix ActorTransform(std::uint32_t actor) override {
        Check(actor == 0x2000u, "actor transform address");
        calls.emplace_back("actor matrix");
        pal4::ActorControlMatrix matrix{};
        matrix.words[8] = std::bit_cast<std::uint32_t>(0.5f);
        matrix.words[10] = std::bit_cast<std::uint32_t>(-0.25f);
        return matrix;
    }
    void SetupTransformMatrix(
        pal4::ActorControlMatrix& destination,
        const pal4::ActorControlMatrix& source) override {
        calls.emplace_back("setup");
        destination = source;
        destination.words[0] = 0x200u;
    }
    void NormalizeBasis(pal4::ActorControlMatrix& matrix,
                        std::uint32_t offset) override {
        Check(matrix.words[0] == 0x200u, "prepared matrix input");
        calls.emplace_back("normalize " + std::to_string(offset));
    }
    pal4::ActorControlMatrix Multiply(
        const pal4::ActorControlMatrix& camera,
        const pal4::ActorControlMatrix& actor) override {
        Check(camera.words[0] == 0x100u && actor.words[0] == 0x200u,
              "matrix multiply inputs");
        calls.emplace_back("multiply");
        pal4::ActorControlMatrix matrix{};
        matrix.words[0] = 0x300u;
        return matrix;
    }
    pal4::ActorControlRotation QueryRotation(
        const pal4::ActorControlMatrix& matrix) override {
        Check(matrix.words[0] == 0x300u, "rotation query matrix");
        calls.emplace_back("rotate");
        return {{1.0f, -2.5f, 3.0f}, 33.0f};
    }
    pal4::ActorControlVector3 CameraVector(
        std::uint32_t camera, std::uint32_t offset) override {
        Check(camera == 0x1000u, "camera vector address");
        calls.emplace_back("vector " + std::to_string(offset));
        if (offset == 80u) return {0.25f, 99.0f, -0.5f};
        Check(offset == 116u, "camera position offset");
        return {10.0f, 20.0f, 30.0f};
    }
};

const std::vector<std::string> kMatrixCalls{
    "camera matrix", "actor matrix", "setup", "normalize 0",
    "normalize 16", "normalize 32", "multiply", "rotate"};

float At(const pal4::PalActorControlState& state, std::uint32_t index) {
    return std::bit_cast<float>(state.words[index]);
}

void TestGuardsAndBasicTransform() {
    pal4::PalActorControlState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[6] = 0u;
    Runtime runtime;
    float axis_y = 123.0f;
    Check(pal4::PalActorControlTransform::Compute(
              state, 0u, 0x2000u, &axis_y, runtime) == 0.0f &&
              pal4::PalActorControlTransform::Compute(
                  state, 0x1000u, 0u, &axis_y, runtime) == 0.0f &&
              axis_y == 123.0f && runtime.calls.empty(),
          "missing camera or actor returns zero without writing output");
    Check(pal4::PalActorControlTransform::Compute(
              state, 0x1000u, 0x2000u, &axis_y, runtime) == 33.0f &&
              axis_y == -2.5f && runtime.calls == kMatrixCalls &&
              state.words[225] == 0xa5a5a5a5u,
          "matrix operations and rotation output without debug writes");
    runtime.calls.clear();
    Check(pal4::PalActorControlTransform::Compute(
              state, 0x1000u, 0x2000u, nullptr, runtime) == 33.0f &&
              runtime.calls == kMatrixCalls,
          "null optional axis output still calculates angle");
}

void TestDebugLines() {
    pal4::PalActorControlState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[6] = 1u;
    Runtime runtime;
    float axis_y = 0.0f;
    Check(pal4::PalActorControlTransform::Compute(
              state, 0x1000u, 0x2000u, &axis_y, runtime) == 33.0f &&
              axis_y == -2.5f,
          "debug transform returns queried rotation");
    auto expected_calls = kMatrixCalls;
    expected_calls.emplace_back("vector 80");
    expected_calls.emplace_back("vector 116");
    Check(runtime.calls == expected_calls,
          "debug vectors follow matrix query");
    for (const auto index : {225u, 243u, 261u})
        Check(At(state, index) == 10.0f &&
                  At(state, index + 1u) == 20.0f &&
                  At(state, index + 2u) == 30.0f,
              "all three line origins copy camera position");
    Check(At(state, 234u) == -35.0f &&
              At(state, 235u) == 20.0f &&
              At(state, 236u) == 52.5f,
          "actor-forward line uses scale 90");
    Check(At(state, 252u) == -25.0f &&
              At(state, 253u) == 20.0f &&
              At(state, 254u) == 100.0f,
          "camera-direction line uses scale 140");
    Check(At(state, 270u) == 10.0f &&
              At(state, 271u) == 60.0f &&
              At(state, 272u) == 30.0f,
          "global up line uses scale 40");
    for (const auto index : {231u, 240u, 267u, 276u})
        Check(state.words[index] == 0xff0000ffu,
              "blue line endpoint colors");
    for (const auto index : {249u, 258u})
        Check(state.words[index] == 0xffffff00u,
              "yellow line endpoint colors");
    Check(state.words[224] == 0xa5a5a5a5u &&
              state.words[280] == 0xa5a5a5a5u,
          "adjacent state is preserved");
}

void TestLinkableStub() {
    pal4::PalActorControlState state{};
    pal4::ActorControlTransformRuntimeStub runtime;
    float axis_y = 1.0f;
    Check(pal4::PalActorControlTransform::Compute(
              state, 1u, 2u, &axis_y, runtime) == 0.0f &&
              axis_y == 0.0f,
          "RenderWare-free stub links and returns empty rotation");
}

}  // namespace

int main() {
    TestGuardsAndBasicTransform();
    TestDebugLines();
    TestLinkableStub();
}
