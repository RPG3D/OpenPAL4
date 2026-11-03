#include "pal4/base_object_child_transform.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void WriteFloat(pal4::BaseObjectState& state,
                std::uint32_t offset, float value) {
    state.Write(offset, std::bit_cast<std::uint32_t>(value));
}

float ReadFloat(const pal4::BaseObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.Read(offset));
}

struct Runtime final : pal4::IBaseObjectChildTransformRuntime {
    std::vector<std::pair<char, std::uint32_t>> events;
    std::array<float, 3> vector{10.0f, 20.0f, 30.0f};

    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x4004u,
              "transform reads the parent frame at parent +4");
        events.emplace_back('D', address);
        return 0x5000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x5000u, "transform uses the parent frame");
        events.emplace_back('T', frame);
        return 0x6000u;
    }
    std::array<float, 3> ReadVector3(std::uint32_t address) override {
        Check(address == 0x6030u,
              "transform reads translation at matrix +0x30");
        events.emplace_back('V', address);
        return vector;
    }
};

void CheckPropertyAndParent() {
    pal4::BaseObjectState state{};
    WriteFloat(state, 0x2b4u, 1.25f);
    WriteFloat(state, 0xdcu, 3.5f);
    Check(pal4::BaseObjectChildTransform::GetDoublePropertyAtThis(state,0u)==7.0 &&
              pal4::BaseObjectChildTransform::GetDoublePropertyAtThis(state,0x1d8u)==2.5,
          "same getter respects NPC and embedded-child this adjustments");
    state.Write(0u, 0x4000u);
    Check(pal4::BaseObjectChildTransform::GetDoubleProperty(state) == 2.5 &&
              pal4::BaseObjectChildTransform::GetParent(state) == 0x4000u,
          "child getters use base-relative property and parent fields");
    WriteFloat(state, 0x2b4u, -3.5f);
    Check(pal4::BaseObjectChildTransform::GetDoubleProperty(state) == -7.0,
          "double property preserves signed float value");
}

void CheckUpdate(float amplitude) {
    pal4::BaseObjectState state{};
    state.Write(0u, 0x4000u);
    state.Write(0x1dcu, 0xaaaaaaaau);
    state.Write(0x1ecu, 0xbbbbbbbbu);
    WriteFloat(state, 0xd8u, amplitude);
    Runtime runtime;
    Check(pal4::BaseObjectChildTransform::Update(
              state, 0x100000u, runtime) == 0x1001e0u,
          "update returns the original address of child translation");
    const auto expected_y = static_cast<float>(
        20.0 + static_cast<double>(amplitude) * 2.0 *
                   static_cast<double>(0.9f));
    Check(ReadFloat(state, 0x1e0u) == 10.0f &&
              ReadFloat(state, 0x1e4u) == expected_y &&
              ReadFloat(state, 0x1e8u) == 30.0f &&
              state.Read(0x1dcu) == 0xaaaaaaaau &&
              state.Read(0x1ecu) == 0xbbbbbbbbu,
          "update copies translation and adds doubled amplitude to Y only");
    Check(runtime.events ==
              std::vector<std::pair<char, std::uint32_t>>{
                  {'D', 0x4004u}, {'T', 0x5000u}, {'V', 0x6030u}},
          "parent frame and matrix are read in native order");
}

}  // namespace

int main() {
    CheckPropertyAndParent();
    CheckUpdate(0.0f);
    CheckUpdate(2.0f);
    CheckUpdate(-1.5f);
}
