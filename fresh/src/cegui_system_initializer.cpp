#include "pal4/cegui_system_initializer.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<CeguiKeyMapping, 100> kKeyMappings{{
    {8u, 14u}, {9u, 15u}, {13u, 28u}, {16u, 42u},
    {17u, 29u}, {18u, 56u}, {19u, 197u}, {20u, 58u},
    {27u, 1u}, {32u, 57u}, {33u, 201u}, {34u, 209u},
    {36u, 199u}, {35u, 207u}, {37u, 203u}, {39u, 205u},
    {38u, 200u}, {40u, 208u}, {44u, 183u}, {45u, 210u},
    {46u, 211u}, {47u, 59u}, {48u, 11u}, {49u, 2u},
    {50u, 3u}, {51u, 4u}, {52u, 5u}, {53u, 6u},
    {54u, 7u}, {55u, 8u}, {56u, 9u}, {57u, 10u},
    {65u, 30u}, {66u, 48u}, {67u, 46u}, {68u, 32u},
    {69u, 18u}, {70u, 33u}, {71u, 34u}, {72u, 35u},
    {73u, 23u}, {74u, 36u}, {75u, 37u}, {76u, 38u},
    {77u, 50u}, {78u, 49u}, {79u, 24u}, {80u, 25u},
    {81u, 16u}, {82u, 19u}, {83u, 31u}, {84u, 20u},
    {85u, 22u}, {86u, 47u}, {87u, 17u}, {88u, 45u},
    {89u, 21u}, {90u, 44u}, {91u, 219u}, {92u, 220u},
    {93u, 221u}, {96u, 82u}, {97u, 79u}, {98u, 80u},
    {99u, 81u}, {100u, 75u}, {101u, 76u}, {102u, 77u},
    {103u, 71u}, {104u, 72u}, {105u, 73u}, {106u, 55u},
    {107u, 78u}, {109u, 74u}, {108u, 74u}, {110u, 83u},
    {111u, 181u}, {112u, 59u}, {113u, 60u}, {114u, 61u},
    {115u, 62u}, {116u, 63u}, {117u, 64u}, {118u, 65u},
    {119u, 66u}, {120u, 67u}, {121u, 68u}, {122u, 87u},
    {123u, 88u}, {124u, 100u}, {125u, 101u}, {126u, 102u},
    {144u, 69u}, {145u, 70u}, {160u, 42u}, {161u, 54u},
    {162u, 29u}, {163u, 157u}, {164u, 56u}, {165u, 184u},
}};

}  // namespace

std::uint32_t CeguiSystemState::ReadDword(
    const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

float CeguiSystemState::ReadFloat(const std::uint32_t offset) const {
    return std::bit_cast<float>(ReadDword(offset));
}

void CeguiSystemState::WriteFloat(
    const std::uint32_t offset, const float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(bits >> (index * 8u));
    }
}

void CeguiSystemState::EnableMouseCapture() noexcept {
    if (bytes_[0] == 0u) bytes_[0] = 1u;
}

void CeguiSystemState::DisableMouseCapture() noexcept {
    if (bytes_[0] != 0u) bytes_[0] = 0u;
}

std::array<float, 2> CeguiSystemState::TransformMouseCoordinates(
    const std::array<float, 2>& point) const {
    const auto x = static_cast<float>(
        (static_cast<double>(point[0]) + ReadFloat(0x1cu)) *
        ReadFloat(0x14u));
    const auto y = static_cast<float>(
        static_cast<double>(point[1]) * ReadFloat(0x18u));
    return {x, y};
}

std::array<float, 2> CeguiSystemState::TransformUiCoordinates(
    const std::array<float, 2>& point) const {
    const auto x = static_cast<float>(
        static_cast<double>(point[0]) / ReadFloat(0x14u) -
        ReadFloat(0x1cu));
    const auto y = static_cast<float>(
        static_cast<double>(point[1]) / ReadFloat(0x18u));
    return {x, y};
}

void CeguiRuntimeStub::ConstructObject(std::uint32_t /*address*/) {}
void CeguiRuntimeStub::InsertKeyMapping(
    std::uint32_t /*object*/, CeguiKeyMapping /*mapping*/) {}
std::uint32_t CeguiRuntimeStub::AllocateSystem(
    std::uint32_t /*bytes*/) { return 0u; }
void CeguiRuntimeStub::ConstructSystemWithProvider(
    std::uint32_t /*address*/, std::uint32_t /*renderer*/,
    std::uint32_t /*provider*/, std::string_view /*log_filename*/) {}
void CeguiRuntimeStub::ConstructSystemWithDefaultProvider(
    std::uint32_t /*address*/, std::uint32_t /*renderer*/,
    std::string_view /*log_filename*/) {}
std::uint32_t CeguiRuntimeStub::GetLogger() { return 0u; }
void CeguiRuntimeStub::WriteDword(
    std::uint32_t /*address*/, std::uint32_t /*value*/) {}

const std::array<CeguiKeyMapping, 100>&
CeguiSystemInitializer::KeyMappings() noexcept {
    return kKeyMappings;
}

std::uint32_t CeguiSystemInitializer::Initialize(
    CeguiSystemState& state, const std::uint32_t self_address,
    const std::uint32_t renderer, const std::uint32_t resource_provider,
    const std::int32_t screen_width, const std::int32_t screen_height,
    ICeguiSystemRuntime& runtime) {
    state.Bytes()[0] = 0u;
    const auto object = self_address + 4u;
    runtime.ConstructObject(object);
    for (const auto mapping : kKeyMappings) {
        runtime.InsertKeyMapping(object, mapping);
    }

    const auto system = runtime.AllocateSystem(kSystemBytes);
    if (system != 0u) {
        if (resource_provider != 0u) {
            runtime.ConstructSystemWithProvider(
                system, renderer, resource_provider, kLogFilename);
        } else {
            runtime.ConstructSystemWithDefaultProvider(
                system, renderer, kLogFilename);
        }
        runtime.WriteDword(system, kSystemVtable);
    }
    runtime.WriteDword(runtime.GetLogger(), 0u);

    if (screen_width == 1280 && screen_height == 800) {
        state.WriteFloat(0x1cu, -106.66669f);
        state.WriteFloat(0x18u, 0.75f);
        state.WriteFloat(0x14u, 0.75f);
    } else {
        state.WriteFloat(0x1cu, 0.0f);
        state.WriteFloat(0x14u,
                         static_cast<float>(800.0 / screen_width));
        state.WriteFloat(0x18u,
                         static_cast<float>(600.0 / screen_height));
    }
    return self_address;
}

}  // namespace pal4
