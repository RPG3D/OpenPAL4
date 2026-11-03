#include "pal4/cylinder_object_aim.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICylinderObjectAimRuntime {
    std::vector<std::uint32_t> reads;
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return 0x99887700u | (address & 0xffu);
    }
};

struct FrameRuntime final : pal4::ICylinderObjectAimFrameRuntime {
    std::vector<std::string> calls;
    pal4::CylinderAimMatrix matrix{};
    std::array<std::uint32_t, 3> scale{};
    std::array<std::uint32_t, 3> position{};
    std::array<std::uint32_t, 3> offset{};
    std::array<float, 3> angles{};
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1024u, std::bit_cast<std::uint32_t>(1.0f)},
        {0x1028u, std::bit_cast<std::uint32_t>(2.0f)},
        {0x102cu, std::bit_cast<std::uint32_t>(3.0f)},
        {0x1018u, std::bit_cast<std::uint32_t>(2.0f)},
        {0x101cu, std::bit_cast<std::uint32_t>(3.0f)},
        {0x1020u, std::bit_cast<std::uint32_t>(4.0f)}};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void Normalize(std::array<float, 3>& vector) override {
        calls.push_back("normalize");
        const float length = std::sqrt(vector[0u] * vector[0u] +
                                       vector[1u] * vector[1u] +
                                       vector[2u] * vector[2u]);
        for (auto& value : vector)
            value /= length;
    }
    void UpdateMatrix(pal4::CylinderAimMatrix& value) override {
        calls.push_back("update_matrix");
        matrix = value;
    }
    std::uint32_t GetFrame(const pal4::CylinderObjectState& state) override {
        Check(state.words[22u] == 0x1000u, "frame owner");
        calls.push_back("get_frame");
        return 0x2000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x2000u, "identity frame");
        calls.push_back("identity");
    }
    void ScaleFrame(std::uint32_t frame,
                    const std::array<std::uint32_t, 3>& value,
                    std::uint32_t mode) override {
        Check(frame == 0x2000u && mode == 0u, "scale frame and mode");
        scale = value;
        calls.push_back("scale");
    }
    void ApplyMatrix(std::uint32_t frame,
                     const pal4::CylinderAimMatrix& value,
                     std::uint32_t mode) override {
        Check(frame == 0x2000u && mode == 2u, "apply matrix and mode");
        matrix = value;
        calls.push_back("apply");
    }
    void TranslateFrame(std::uint32_t frame,
                        const std::array<std::uint32_t, 3>& value,
                        std::uint32_t mode) override {
        Check(frame == 0x2000u && mode == 2u, "translate frame and mode");
        position = value;
        calls.push_back("translate");
    }
    void UpdateFrameRotation(
        std::uint32_t frame, const std::array<std::uint32_t, 3>& value,
        std::array<float, 3>& output) override {
        Check(frame == 0x2000u, "rotation helper frame");
        offset = value;
        output = {10.0f, 20.0f, 30.0f};
        calls.push_back("update_rotation");
    }
    std::uint8_t SetRotation(
        pal4::CylinderObjectState& state,
        const std::array<float, 3>& value,
        std::uint32_t mode) override {
        Check(state.words[22u] == 0x1000u && mode == 0u,
              "rotation facet and mode");
        angles = value;
        calls.push_back("set_rotation");
        return 0xa7u;
    }
};

struct PositionRuntime final : pal4::ICylinderObjectTransformRuntime {
    std::uint32_t model = 0u;
    std::array<float, 3> position{};
    std::uint8_t mode = 0u;
    std::uint32_t calls = 0u;
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    std::uint8_t SetPosition(std::uint32_t value,
                             const std::array<float, 3>& input,
                             std::uint8_t input_mode) override {
        model = value;
        position = input;
        mode = input_mode;
        ++calls;
        return 0xa9u;
    }
    std::uint8_t SetRotation(std::uint32_t,
                             const std::array<float, 3>&,
                             std::uint8_t) override { return 0u; }
    std::uint8_t SetScale(std::uint32_t,
                          const std::array<float, 3>&,
                          std::uint8_t) override { return 0u; }
};

void TestAimFields() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x2000u;
    std::array<std::uint32_t, 3> output{1u, 2u, 3u};
    const std::array<std::uint32_t, 3> offset{
        0x3f800000u, 0xbf800000u, 0x123456a5u};
    Runtime runtime;
    using pal4::CylinderObjectAim;
    Check(CylinderObjectAim::GetPosition(state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{1u, 2u, 3u} &&
              runtime.reads.empty() &&
              CylinderObjectAim::SetRotationOffset(state, offset) == 0u &&
              state.words[18u] == 0u && state.words[19u] == 0u &&
              state.words[20u] == 0u,
          "inactive aim facet preserves position and rotation offset");
    state.words[1u] = 0x01000001u;
    Check(CylinderObjectAim::GetPosition(state, output, runtime) == 0x2cu &&
              output == std::array<std::uint32_t, 3>{
                  0x99887724u, 0x99887728u, 0x9988772cu} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x2024u, 0x2028u, 0x202cu},
          "position getter reads three raw model words");
    Check(CylinderObjectAim::SetRotationOffset(state, offset) == 0xa5u &&
              state.words[18u] == offset[0u] &&
              state.words[19u] == offset[1u] &&
              state.words[20u] == offset[2u] &&
              state.words[21u] == 0u,
          "rotation offset setter stores three raw owner words and returns low byte");
}

void TestFrameUpdate() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x1000u;
    state.words[21u] = 0x12345678u;
    FrameRuntime runtime;
    using pal4::CylinderObjectAim;
    Check(CylinderObjectAim::UpdateFrame(state, 1u, runtime) == 0u &&
              state.words[21u] == 0x12345678u && runtime.calls.empty(),
          "inactive update retains mode and skips native calls");
    state.words[1u] = 1u;
    Check(CylinderObjectAim::UpdateFrame(state, 0u, runtime) == 0u &&
              state.words[21u] == 0x12345600u && runtime.calls.empty(),
          "disabled update only stores mode byte");
    state.words[15u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[16u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[17u] = std::bit_cast<std::uint32_t>(13.0f);
    state.words[18u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[19u] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[20u] = std::bit_cast<std::uint32_t>(6.0f);
    Check(CylinderObjectAim::UpdateFrame(state, 1u, runtime) == 0xa7u &&
              state.words[21u] == 0x12345601u,
          "enabled update returns rotation facet byte");
    Check(runtime.matrix.right == std::array<float, 3>{1.0f, 0.0f, 0.0f} &&
              runtime.matrix.up == std::array<float, 3>{0.0f, 1.0f, 0.0f} &&
              runtime.matrix.at == std::array<float, 3>{0.0f, 0.0f, 1.0f} &&
              runtime.matrix.flags == 0x20003u,
          "target minus model position forms normalized basis");
    Check(runtime.scale == std::array<std::uint32_t, 3>{
              std::bit_cast<std::uint32_t>(2.0f),
              std::bit_cast<std::uint32_t>(3.0f),
              std::bit_cast<std::uint32_t>(4.0f)} &&
              runtime.position == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(1.0f),
                  std::bit_cast<std::uint32_t>(2.0f),
                  std::bit_cast<std::uint32_t>(3.0f)} &&
              runtime.offset == std::array<std::uint32_t, 3>{
                  state.words[18u], state.words[19u], state.words[20u]} &&
              runtime.angles == std::array<float, 3>{10.0f, 20.0f, 30.0f},
          "scale, position, offset and output angles reach callbacks");
    Check(runtime.calls == std::vector<std::string>{
              "read:4132", "read:4136", "read:4140", "normalize",
              "normalize", "normalize", "update_matrix", "get_frame",
              "read:4120", "read:4124", "read:4128", "identity", "scale",
              "apply", "translate", "update_rotation", "set_rotation"},
          "native vector, matrix and frame call order");
}

void TestPositionWithMode() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x1000u;
    PositionRuntime runtime;
    const std::array<float, 3> position{-2.5f, 0.25f, 7.0f};
    using pal4::CylinderObjectAim;
    Check(CylinderObjectAim::SetPositionWithMode(
              state, position, 7u, runtime) == 0u && runtime.calls == 0u,
          "inactive position facet does not forward");
    state.words[1u] = 1u;
    Check(CylinderObjectAim::SetPositionWithMode(
              state, position, 7u, runtime) == 0xa9u &&
              runtime.calls == 1u && runtime.model == 0x1000u &&
              runtime.position == position && runtime.mode == 7u,
          "active position facet forwards model, vector and mode");
}

}  // namespace

int main() {
    TestAimFields();
    TestFrameUpdate();
    TestPositionWithMode();
}
