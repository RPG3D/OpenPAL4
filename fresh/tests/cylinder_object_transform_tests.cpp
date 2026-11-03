#include "pal4/cylinder_object_transform.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICylinderObjectTransformRuntime {
    std::vector<std::uint32_t> reads;
    std::vector<char> setters;
    std::array<float, 3> last_value{};
    std::uint32_t last_model = 0u;
    std::uint8_t last_mode = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return 0x11223300u | (address & 0xffu);
    }
    std::uint8_t SetPosition(std::uint32_t model,
                             const std::array<float, 3>& value,
                             std::uint8_t mode) override {
        Record('P', model, value, mode);
        return 0x91u;
    }
    std::uint8_t SetRotation(std::uint32_t model,
                             const std::array<float, 3>& value,
                             std::uint8_t mode) override {
        Record('R', model, value, mode);
        return 0xa2u;
    }
    std::uint8_t SetScale(std::uint32_t model,
                          const std::array<float, 3>& value,
                          std::uint8_t mode) override {
        Record('S', model, value, mode);
        return 0xb3u;
    }
    void Record(char setter, std::uint32_t model,
                const std::array<float, 3>& value, std::uint8_t mode) {
        setters.push_back(setter);
        last_model = model;
        last_value = value;
        last_mode = mode;
    }
};

void TestGetters() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x2000u;
    std::array<std::uint32_t, 3> output{7u, 8u, 9u};
    Runtime runtime;
    using pal4::CylinderObjectTransform;
    Check(CylinderObjectTransform::GetRotation(state, output, runtime) == 0u &&
              CylinderObjectTransform::GetScale(state, output, runtime) == 0u &&
              CylinderObjectTransform::GetPosition(state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              runtime.reads.empty(),
          "inactive getters preserve outputs and do not read model");

    state.words[1u] = 0x01000001u;
    Check(CylinderObjectTransform::GetRotation(state, output, runtime) == 0x14u &&
              output == std::array<std::uint32_t, 3>{
                  0x1122330cu, 0x11223310u, 0x11223314u} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x200cu, 0x2010u, 0x2014u},
          "rotation reads model +12/+16/+20 as raw words");
    runtime.reads.clear();
    Check(CylinderObjectTransform::GetScale(state, output, runtime) == 0x20u &&
              output == std::array<std::uint32_t, 3>{
                  0x11223318u, 0x1122331cu, 0x11223320u} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x2018u, 0x201cu, 0x2020u},
          "scale reads model +24/+28/+32");
    runtime.reads.clear();
    Check(CylinderObjectTransform::GetPosition(state, output, runtime) == 0x2cu &&
              output == std::array<std::uint32_t, 3>{
                  0x11223324u, 0x11223328u, 0x1122332cu} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x2024u, 0x2028u, 0x202cu},
          "position reads model +36/+40/+44");
}

void TestSetters() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x2000u;
    Runtime runtime;
    const std::array<float, 3> value{-1.25f, 0.5f, 90.0f};
    using pal4::CylinderObjectTransform;
    Check(CylinderObjectTransform::SetScale(state, value, 3u, runtime) == 0u &&
              CylinderObjectTransform::SetRotation(state, value, 4u, runtime) == 0u &&
              CylinderObjectTransform::SetPosition(state, value, 5u, runtime) == 0u &&
              runtime.setters.empty(),
          "inactive setters skip native callbacks");
    state.words[1u] = 0x01000001u;
    Check(CylinderObjectTransform::SetScale(state, value, 3u, runtime) == 0xb3u &&
              runtime.setters == std::vector<char>{'S'} &&
              runtime.last_model == 0x2000u && runtime.last_value == value &&
              runtime.last_mode == 3u,
          "scale setter forwards vector, mode and result");
    Check(CylinderObjectTransform::SetRotation(state, value, 4u, runtime) == 0xa2u &&
              runtime.setters == std::vector<char>{'S', 'R'} &&
              runtime.last_model == 0x2000u && runtime.last_value == value &&
              runtime.last_mode == 4u,
          "rotation setter forwards native arguments");
    Check(CylinderObjectTransform::SetPosition(state, value, 5u, runtime) == 0x91u &&
              runtime.setters == std::vector<char>{'S', 'R', 'P'} &&
              runtime.last_model == 0x2000u && runtime.last_value == value &&
              runtime.last_mode == 5u,
          "position setter forwards native arguments");
}

}  // namespace

int main() {
    TestGetters();
    TestSetters();
}
