#include "pal4/camera_frustum_geometry.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void SetVector(pal4::CameraDataState& state, std::size_t offset,
               const std::array<float, 3>& value) {
    for (std::size_t i = 0; i < value.size(); ++i)
        state.At(offset + i * 4u) = std::bit_cast<std::uint32_t>(value[i]);
}

void CheckVertices(const pal4::CameraDataState& state,
                   const std::array<std::array<float, 3>, 8>& expected) {
    for (std::size_t vertex = 0; vertex < expected.size(); ++vertex)
        for (std::size_t axis = 0; axis < 3u; ++axis)
            Check(state.At(0x1c0u + vertex * 12u + axis * 4u) ==
                      std::bit_cast<std::uint32_t>(expected[vertex][axis]),
                  "frustum vertex coordinate or ordering");
}

void Cases() {
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    SetVector(state, 0x74u, {10.0f, 20.0f, 30.0f});
    SetVector(state, 0x80u, {0.0f, 1.0f, 0.0f});
    SetVector(state, 0x5cu, {1.0f, 0.0f, 0.0f});
    SetVector(state, 0x50u, {0.0f, 0.0f, 1.0f});
    state.At(0x1bcu) = std::bit_cast<std::uint32_t>(2.0f);

    constexpr std::array<std::array<float, 3>, 8> expected{{
        {10.0f, 22.0f, 30.0f}, {10.0f, 22.0f, 25.0f},
        {10.0f, 18.0f, 30.0f}, {10.0f, 18.0f, 25.0f},
        {8.0f, 20.0f, 30.0f}, {8.0f, 20.0f, 25.0f},
        {12.0f, 20.0f, 30.0f}, {12.0f, 20.0f, 25.0f}}};
    Check(pal4::CameraFrustumGeometry::UpdateVertices(state, 5.0f) ==
              std::bit_cast<std::uint32_t>(25.0f),
          "last vertex Z bits are returned");
    CheckVertices(state, expected);
    Check(state.At(0x1b8u) == 0xdeadbeefu &&
              state.At(0x220u) == 0xdeadbeefu,
          "frustum update preserves adjacent fields");

    Check(pal4::CameraFrustumGeometry::UpdateVertices(state, 0.0f) ==
              std::bit_cast<std::uint32_t>(30.0f) &&
              state.At(0x1ccu) == state.At(0x1c0u) &&
              state.At(0x1d0u) == state.At(0x1c4u) &&
              state.At(0x1d4u) == state.At(0x1c8u),
          "zero depth collapses front and back vertices");
}

void OriginalM05CameraProbe() {
    // Real MainCamera input from live222; expected coordinates were executed
    // by IDA5EA420 original750-byte pure function with x87CW007F (PC24).
    // Oracle/source evidence lives in objs/camera-frustum-isa-20261027.
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    state.At(0x74u) = 0x44960000u;
    state.At(0x78u) = 0xc3750350u;
    state.At(0x7cu) = 0x432f0000u;
    state.At(0x80u) = 0xbe962f07u;
    state.At(0x84u) = 0x3f66175eu;
    state.At(0x88u) = 0x3ea6cbc1u;
    state.At(0x5cu) = 0x3f3e3ebeu;
    state.At(0x60u) = 0x00000000u;
    state.At(0x64u) = 0x3f2b4c24u;
    state.At(0x50u) = 0xbf19f60bu;
    state.At(0x54u) = 0xbee0722fu;
    state.At(0x58u) = 0x3f2afdbcu;
    state.At(0x1bcu) = 0x416f5c29u;
    struct Case { std::uint32_t depth; std::array<std::uint32_t,24> expected; };
    constexpr std::array<Case,3> cases{{
        {0x429e62a2u, {0x44957394u, 0xc3679126u, 0x4333dfa2u, 0x449b67a7u, 0xc344d9e9u, 0x42fdf4c8u, 0x44968c6cu, 0xc3813abdu, 0x432a205eu, 0x449c807fu, 0xc35fbe3du, 0x42ea7640u, 0x44949c3eu, 0xc3750350u, 0x4324fd64u, 0x449a9051u, 0xc3524c13u, 0x42e0304cu, 0x449763c2u, 0xc3750350u, 0x4339029cu, 0x449d57d5u, 0xc3524c13u, 0x43041d5eu}},
        {0x42a3baf4u, {0x44957394u, 0xc3679126u, 0x4333dfa2u, 0x449b9b16u, 0xc343adffu, 0x42fa62d6u, 0x44968c6cu, 0xc3813abdu, 0x432a205eu, 0x449cb3eeu, 0xc35e9253u, 0x42e6e44eu, 0x44949c3eu, 0xc3750350u, 0x4324fd64u, 0x449ac3c0u, 0xc3512029u, 0x42dc9e5au, 0x449763c2u, 0xc3750350u, 0x4339029cu, 0x449d8b44u, 0xc3512029u, 0x43025465u}},
        {0x42a5a78cu, {0x44957394u, 0xc3679126u, 0x4333dfa2u, 0x449bad9au, 0xc3434206u, 0x42f919d1u, 0x44968c6cu, 0xc3813abdu, 0x432a205eu, 0x449cc672u, 0xc35e265au, 0x42e59b49u, 0x44949c3eu, 0xc3750350u, 0x4324fd64u, 0x449ad644u, 0xc350b430u, 0x42db5555u, 0x449763c2u, 0xc3750350u, 0x4339029cu, 0x449d9dc8u, 0xc350b430u, 0x4301afe2u}},
    }};
    for (const auto& test : cases) {
        const auto result=pal4::CameraFrustumGeometry::UpdateVertices(
            state,std::bit_cast<float>(test.depth));
        for (unsigned i=0;i<24u;++i)
            Check(state.At(0x1c0u+i*4u)==test.expected[i],
                  "M05 probe vertices match original PC24 instruction output exactly");
        Check(result==test.expected.back() && state.At(0x220u)==0xdeadbeefu,
              "M05 probe return and adjacent storage are preserved");
    }
}

}  // namespace

int main() {
    try {
        Cases();
        OriginalM05CameraProbe();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
