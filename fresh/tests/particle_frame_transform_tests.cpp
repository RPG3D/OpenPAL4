#include "pal4/particle_visual_object.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IParticleFrameTransformRuntime {
    std::vector<std::string> calls;
    void SetIdentity(std::uint32_t frame) override {
        Check(frame == 0x6000u, "frame identity target");
        calls.emplace_back("identity");
    }
    void Scale(std::uint32_t frame, pal4::ParticleVisualPoint scale,
               std::uint32_t combine) override {
        Check(frame == 0x6000u && combine == 0u &&
                  scale.x == 2.f && scale.y == 3.f && scale.z == 4.f,
              "frame scale arguments");
        calls.emplace_back("scale");
    }
    void Rotate(std::uint32_t frame, pal4::ParticleVisualPoint axis,
                float angle, std::uint32_t combine) override {
        Check(frame == 0x6000u && combine == 2u,
              "frame rotation mode");
        if (axis.x == 1.f && axis.y == 0.f && axis.z == 0.f) {
            Check(angle == 10.f, "X rotation angle");
            calls.emplace_back("rotate_x");
        } else if (axis.x == 0.f && axis.y == 1.f && axis.z == 0.f) {
            Check(angle == 20.f, "Y rotation angle");
            calls.emplace_back("rotate_y");
        } else {
            Check(axis.x == 0.f && axis.y == 0.f && axis.z == 1.f &&
                      angle == 30.f,
                  "Z rotation axis and angle");
            calls.emplace_back("rotate_z");
        }
    }
    void Translate(std::uint32_t frame,
                   pal4::ParticleVisualPoint position,
                   std::uint32_t combine) override {
        Check(frame == 0x6000u && combine == 2u &&
                  position.x == 5.f && position.y == 6.f &&
                  position.z == 7.f,
              "frame translation arguments");
        calls.emplace_back("translate");
    }
    std::uint8_t UpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x6000u, "frame update target");
        calls.emplace_back("update");
        return 9u;
    }
};

void TestInactive() {
    Runtime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::UpdateFrameTransform(
              state, runtime) == 0u && runtime.calls.empty(),
          "inactive object skips all frame operations");
}

void TestTransformOrderAndValues() {
    Runtime runtime;
    pal4::ParticleVisualState state;
    state.WriteByte(0u, 1u);
    state.WriteWord(44u, 0x6000u);
    const auto write_float = [&state](std::size_t offset, float value) {
        state.WriteWord(offset, std::bit_cast<std::uint32_t>(value));
    };
    write_float(100u, 2.f);
    write_float(104u, 3.f);
    write_float(108u, 4.f);
    write_float(88u, 10.f);
    write_float(92u, 20.f);
    write_float(96u, 30.f);
    write_float(112u, 5.f);
    write_float(116u, 6.f);
    write_float(120u, 7.f);
    Check(pal4::ParticleVisualObject::UpdateFrameTransform(
              state, runtime) == 9u &&
              runtime.calls == std::vector<std::string>{
                  "identity", "scale", "rotate_x", "rotate_y",
                  "rotate_z", "translate", "update"},
          "native frame transform order and final return");
}

}  // namespace

int main() {
    TestInactive();
    TestTransformOrderAndValues();
}
