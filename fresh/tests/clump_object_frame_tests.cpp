#include "pal4/clump_object_frame.h"

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

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpObjectFrameRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, float> floats{
        {0x2040u, 5.0f}, {0x2044u, 25.0f}, {0x2048u, 40.0f}};
    std::array<float, 3> position{};
    std::uint32_t rotation_index = 0u;

    std::uint32_t GetFrame(const pal4::ClumpObjectState& state) override {
        Check(state.words[18u] == 0x1000u, "owner virtual+32 supplies the frame, not manager clump +20");
        calls.push_back("get_frame");
        return 0x2000u;
    }
    void TranslateMatrix(std::uint32_t matrix,
                         const std::array<float, 3>& vector,
                         std::uint32_t mode) override {
        Check(matrix == 0x2010u && mode == 2u,
              "matrix translation address and mode");
        const auto first = calls.size() == 1u;
        Check(vector == (first ? std::array<float, 3>{
                    -10.0f, -20.0f, -30.0f} :
                    std::array<float, 3>{10.0f, 20.0f, 30.0f}),
              "translation brackets pivot rotations");
        calls.push_back(first ? "translate_negative" : "translate_positive");
    }
    void RotateMatrix(std::uint32_t matrix,
                      const std::array<float, 3>& axis,
                      float angle, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}}};
        constexpr std::array<float, 3> angles{15.0f, 25.0f, 35.0f};
        Check(matrix == 0x2010u && mode == 2u &&
                  rotation_index < axes.size() &&
                  axis == axes[rotation_index] &&
                  angle == angles[rotation_index],
              "rotation axis, angle and order");
        calls.push_back("rotate:" + std::to_string(rotation_index++));
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x2000u, "frame update address");
        calls.push_back("update_frame");
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return floats.at(address);
    }
    std::uint32_t SetPosition(std::uint32_t manager,
                              const std::array<float, 3>& value,
                              std::uint32_t mode) override {
        Check(manager == 0x1000u && mode == 0u,
              "final position manager and mode");
        calls.push_back("set_position");
        position = value;
        return 0xabcdef01u;
    }
};

pal4::ClumpObjectState State() {
    pal4::ClumpObjectState state{};
    state.words[18u] = 0x1000u;
    return state;
}

void CheckCallOrder(const Runtime& runtime) {
    Check(runtime.calls == std::vector<std::string>{
              "get_frame", "translate_negative", "rotate:0", "rotate:1",
              "rotate:2", "translate_positive", "update_frame",
              "read:8256", "read:8260", "read:8264", "set_position"},
          "frame transformation and approach callback order");
}

void TestFrameTransform() {
    auto state = State();
    Runtime runtime;
    constexpr std::array<float, 3> target{10.0f, 20.0f, 30.0f};
    constexpr std::array<float, 3> rotation{15.0f, 25.0f, 35.0f};
    constexpr std::array<float, 3> step{2.0f, 3.0f, 4.0f};
    Check(pal4::ClumpObject::ApplyFrameTransform(
              state, target, rotation, step, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive frame transform has no external effects");

    state.words[1u] = 1u;
    Check(pal4::ClumpObject::ApplyFrameTransform(
              state, target, rotation, step, runtime) == 0xabcdef01u &&
              runtime.position == std::array<float, 3>{7.0f, 22.0f, 36.0f},
          "frame transform advances each axis toward target");
    CheckCallOrder(runtime);

    runtime.calls.clear();
    runtime.rotation_index = 0u;
    runtime.floats = {{0x2040u, 9.0f}, {0x2044u, 18.0f},
                      {0x2048u, 28.0f}};
    Check(pal4::ClumpObject::ApplyFrameTransform(
              state, target, rotation, step, runtime) == 0xabcdef01u &&
              runtime.position == target,
          "overshooting steps clamp to target");
    CheckCallOrder(runtime);

    runtime.calls.clear();
    runtime.rotation_index = 0u;
    runtime.floats[0x2040u] = 10.0f;
    runtime.floats[0x2044u] = std::bit_cast<float>(0x7fc00001u);
    runtime.floats[0x2048u] = 30.0f;
    Check(pal4::ClumpObject::ApplyFrameTransform(
              state, target, rotation, step, runtime) == 0xabcdef01u &&
              runtime.position[0u] == 10.0f &&
              std::isnan(runtime.position[1u]) &&
              runtime.position[2u] == 30.0f,
          "equal and unordered axes keep their observed frame values");
}

}  // namespace

int main() {
    TestFrameTransform();
}
