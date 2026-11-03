#include "pal4/clump_object_transform.h"

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

struct Runtime final : pal4::IClumpObjectTransformRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::array<float, 3> last{};
    std::uint32_t last_mode = 0xffffffffu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t Translate(std::uint32_t manager,
                            const std::array<float, 3>& value,
                            std::uint32_t mode) override {
        Check(manager == 0x1000u, "position manager");
        calls.push_back("translate");
        last = value;
        last_mode = mode;
        return 0x12345678u;
    }
    std::uint8_t Rotate(std::uint32_t manager,
                        const std::array<float, 3>& value,
                        std::uint32_t mode) override {
        Check(manager == 0x1000u, "rotation manager");
        calls.push_back("rotate");
        last = value;
        last_mode = mode;
        return 0x9au;
    }
    std::uint32_t Scale(std::uint32_t manager,
                        const std::array<float, 3>& value,
                        std::uint32_t mode) override {
        Check(manager == 0x1000u, "scale manager");
        calls.push_back("scale");
        last = value;
        last_mode = mode;
        return 0xabcdef01u;
    }
};

void Store(Runtime& runtime, const std::uint32_t offset,
           const std::array<float, 3>& value) {
    for (std::uint32_t i = 0u; i < value.size(); ++i)
        runtime.words[0x1000u + offset + 4u * i] =
            std::bit_cast<std::uint32_t>(value[i]);
}

void TestPositionAndRotation() {
    pal4::ClumpObjectState state{};
    state.words[18u] = 0x1000u;
    Runtime runtime;
    const std::array<float, 3> input{1.0f, -2.0f, 3.0f};
    Check(pal4::ClumpObject::SetPosition(state, input, true, runtime) == 0u &&
              pal4::ClumpObject::SetRotation(state, input, true, runtime) == 0u &&
              pal4::ClumpObject::SetScale(state, input, true, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive transform setters avoid manager operations");

    state.words[1u] = 1u;
    Store(runtime, 172u, {10.0f, 20.0f, 30.0f});
    Check(pal4::ClumpObject::SetPosition(state, input, true, runtime) ==
              0x12345678u &&
              runtime.last == std::array<float, 3>{11.0f, 18.0f, 33.0f} &&
              runtime.last_mode == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4268", "read:4272", "read:4276", "translate"},
          "relative position reads manager +172 and translates with mode zero");
    runtime.calls.clear();
    Check(pal4::ClumpObject::SetPosition(state, input, false, runtime) ==
              0x12345678u && runtime.last == input &&
              runtime.calls == std::vector<std::string>{
                  "read:4268", "read:4272", "read:4276", "translate"},
          "absolute position still reads old vector before translation");

    runtime.calls.clear();
    Check(pal4::ClumpObject::SetRotation(state, input, true, runtime) ==
              0x9au && runtime.last == input && runtime.last_mode == 1u &&
              runtime.calls == std::vector<std::string>{"rotate"},
          "relative rotation delegates input and normalized mode");
    runtime.calls.clear();
    Check(pal4::ClumpObject::SetRotation(state, input, false, runtime) ==
              0x9au && runtime.last_mode == 0u &&
              runtime.calls == std::vector<std::string>{"rotate"},
          "absolute rotation uses mode zero without manager reads");
}

void TestScale() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 1u;
    state.words[18u] = 0x1000u;
    Runtime runtime;
    Store(runtime, 184u, {1.0f, 2.0f, 3.0f});
    Check(pal4::ClumpObject::SetScale(
              state, {-2.0f, 1.0f, -3.0f}, true, runtime) ==
              0xabcdef01u &&
              runtime.last == std::array<float, 3>{0.0f, 3.0f, 0.0f} &&
              runtime.last_mode == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4280", "read:4284", "read:4288", "scale"},
          "relative scale reads +184 and clamps negative components");

    runtime.calls.clear();
    const auto nan = std::bit_cast<float>(0x7fc00001u);
    Check(pal4::ClumpObject::SetScale(
              state, {nan, -0.0f, -1.0f}, false, runtime) ==
              0xabcdef01u && std::isnan(runtime.last[0u]) &&
              std::signbit(runtime.last[1u]) && runtime.last[2u] == 0.0f &&
              runtime.calls == std::vector<std::string>{
                  "read:4280", "read:4284", "read:4288", "scale"},
          "absolute scale preserves unordered and negative-zero values");
}

}  // namespace

int main() {
    TestPositionAndRotation();
    TestScale();
}
