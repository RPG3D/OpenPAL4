#include "pal4/clump_object_aim.h"

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

struct Runtime final : pal4::IClumpObjectTransformRuntime {
    std::vector<std::uint32_t> reads;
    std::array<float, 3> position{};
    std::uint32_t manager = 0u;
    std::uint32_t mode = 99u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x10acu, 0x3f800001u}, {0x10b0u, 0x40000002u},
        {0x10b4u, 0x404000feu}};

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    std::uint32_t Translate(std::uint32_t value,
                            const std::array<float, 3>& vector,
                            std::uint32_t relative) override {
        manager = value;
        position = vector;
        mode = relative;
        return 0x123456abu;
    }
    std::uint8_t Rotate(std::uint32_t, const std::array<float, 3>&,
                        std::uint32_t) override {
        Check(false, "unexpected rotate callback");
        return 0u;
    }
    std::uint32_t Scale(std::uint32_t, const std::array<float, 3>&,
                        std::uint32_t) override {
        Check(false, "unexpected scale callback");
        return 0u;
    }
};

struct FrameRuntime final : pal4::IClumpObjectAimRuntime {
    std::vector<std::string> calls;
    pal4::ClumpAimMatrix matrix{};
    std::array<std::uint32_t, 3> scale{};
    std::array<std::uint32_t, 3> position{};
    std::array<std::uint32_t, 3> offset{};
    std::array<float, 3> angles{};
    std::map<std::uint32_t, std::uint32_t> words{
        {0x10acu, std::bit_cast<std::uint32_t>(1.0f)},
        {0x10b0u, std::bit_cast<std::uint32_t>(2.0f)},
        {0x10b4u, std::bit_cast<std::uint32_t>(3.0f)},
        {0x10b8u, std::bit_cast<std::uint32_t>(2.0f)},
        {0x10bcu, std::bit_cast<std::uint32_t>(3.0f)},
        {0x10c0u, std::bit_cast<std::uint32_t>(4.0f)}};

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
    void UpdateMatrix(pal4::ClumpAimMatrix& value) override {
        calls.push_back("update_matrix");
        matrix = value;
    }
    std::uint32_t GetFrame(const pal4::ClumpObjectState& state) override {
        Check(state.words[18u] == 0x1000u, "frame owner");
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
                     const pal4::ClumpAimMatrix& value,
                     std::uint32_t mode) override {
        Check(frame == 0x2000u && mode == 2u, "apply frame and mode");
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
        pal4::ClumpObjectState& state,
        const std::array<float, 3>& value,
        std::uint32_t mode) override {
        Check(state.words[18u] == 0x1000u && mode == 0u,
              "rotation facet and mode");
        angles = value;
        calls.push_back("set_rotation");
        return 0xa7u;
    }
};

void TestInactive() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    state.words[1u] = 0x100u;
    state.words[20u] = 0xdeadu;
    state.words[23u] = 0xbeefu;
    std::array<std::uint32_t, 3> output{7u, 8u, 9u};
    using pal4::ClumpObjectAimFacet;
    Check(ClumpObjectAimFacet::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, runtime) == 0u &&
              ClumpObjectAimFacet::SetPositionWithMode(
                  state, {1.0f, 2.0f, 3.0f}, 7u, runtime) == 0u &&
              ClumpObjectAimFacet::GetPosition(state, output, runtime) == 0u &&
              ClumpObjectAimFacet::SetTarget(state, {11u, 12u, 13u}) == 0u &&
              ClumpObjectAimFacet::SetRotationOffset(
                  state, {21u, 22u, 23u}) == 0u &&
              output == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              state.words[20u] == 0xdeadu &&
              state.words[23u] == 0xbeefu && runtime.reads.empty() &&
              runtime.mode == 99u,
          "all four entries respect the low-byte activity gate");
}

void TestActive() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 1u;
    state.words[18u] = 0x1000u;
    Runtime runtime;
    std::array<std::uint32_t, 3> output{};
    using pal4::ClumpObjectAimFacet;
    Check(ClumpObjectAimFacet::SetPosition(
              state, {1.5f, -2.5f, 3.25f}, runtime) == 0xabu &&
              runtime.manager == 0x1000u && runtime.mode == 0u &&
              runtime.position ==
                  std::array<float, 3>{1.5f, -2.5f, 3.25f} &&
              runtime.reads.empty(),
          "position setter forwards direct vector and native low byte");
    Check(ClumpObjectAimFacet::GetPosition(state, output, runtime) == 0xfeu &&
              output == std::array<std::uint32_t, 3>{
                  0x3f800001u, 0x40000002u, 0x404000feu} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x10acu, 0x10b0u, 0x10b4u},
          "getter copies manager position words and returns third low byte");
    Check(ClumpObjectAimFacet::SetPositionWithMode(
              state, {-4.0f, 5.0f, 6.0f}, 7u, runtime) == 0xabu &&
              runtime.manager == 0x1000u && runtime.mode == 7u &&
              runtime.position == std::array<float, 3>{-4.0f, 5.0f, 6.0f},
          "mode-bearing position setter forwards the caller's byte");
    Check(ClumpObjectAimFacet::SetTarget(
              state, {0x3f800000u, 0x400000a5u, 0x40400000u}) == 0xa5u &&
              state.words[20u] == 0x3f800000u &&
              state.words[21u] == 0x400000a5u &&
              state.words[22u] == 0x40400000u,
          "target setter stores three raw words at +80");
    Check(ClumpObjectAimFacet::SetRotationOffset(
              state, {0x3f000000u, 0x40000000u, 0x4040005au}) == 0x5au &&
              state.words[23u] == 0x3f000000u &&
              state.words[24u] == 0x40000000u &&
              state.words[25u] == 0x4040005au,
          "rotation offset setter stores three raw words at +92");
}

void TestFrameUpdate() {
    pal4::ClumpObjectState state{};
    state.words[18u] = 0x1000u;
    state.words[26u] = 0x12345678u;
    FrameRuntime runtime;
    using pal4::ClumpObjectAimFacet;
    Check(ClumpObjectAimFacet::UpdateFrame(state, 1u, runtime) == 0u &&
              state.words[26u] == 0x12345678u && runtime.calls.empty(),
          "inactive frame update leaves mode untouched");

    state.words[1u] = 1u;
    Check(ClumpObjectAimFacet::UpdateFrame(state, 0u, runtime) == 0u &&
              state.words[26u] == 0x12345600u && runtime.calls.empty(),
          "disabled mode only stores low byte");

    state.words[20u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[21u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[22u] = std::bit_cast<std::uint32_t>(13.0f);
    state.words[23u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[24u] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[25u] = std::bit_cast<std::uint32_t>(6.0f);
    Check(ClumpObjectAimFacet::UpdateFrame(state, 1u, runtime) == 0xa7u &&
              state.words[26u] == 0x12345601u,
          "enabled update returns rotation facet result");
    Check(runtime.matrix.right == std::array<float, 3>{1.0f, 0.0f, 0.0f} &&
              runtime.matrix.up == std::array<float, 3>{0.0f, 1.0f, 0.0f} &&
              runtime.matrix.at == std::array<float, 3>{0.0f, 0.0f, 1.0f} &&
              runtime.matrix.position ==
                  std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              runtime.matrix.flags == 0x20003u,
          "target minus manager position forms the normalized frame basis");
    Check(runtime.scale == std::array<std::uint32_t, 3>{
              std::bit_cast<std::uint32_t>(2.0f),
              std::bit_cast<std::uint32_t>(3.0f),
              std::bit_cast<std::uint32_t>(4.0f)} &&
              runtime.position == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(1.0f),
                  std::bit_cast<std::uint32_t>(2.0f),
                  std::bit_cast<std::uint32_t>(3.0f)} &&
              runtime.offset == std::array<std::uint32_t, 3>{
                  state.words[23u], state.words[24u], state.words[25u]} &&
              runtime.angles == std::array<float, 3>{10.0f, 20.0f, 30.0f},
          "scale, position, cached offset and final angles reach callbacks");
    Check(runtime.calls == std::vector<std::string>{
              "read:4268", "read:4272", "read:4276", "normalize",
              "normalize", "normalize", "update_matrix", "get_frame",
              "read:4280", "read:4284", "read:4288", "identity", "scale",
              "apply", "translate", "update_rotation", "set_rotation"},
          "native matrix and frame callback order");
}

}  // namespace

int main() {
    TestInactive();
    TestActive();
    TestFrameUpdate();
}
