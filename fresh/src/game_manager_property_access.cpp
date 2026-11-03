#include "pal4/game_manager_property_access.h"

#include <bit>

namespace pal4 {
namespace {

double GetFloat(const GameManagerPropertyState& state, std::int32_t index,
                std::uint32_t first_word, std::uint32_t count) {
    if (index < 0 || static_cast<std::uint32_t>(index) >= count)
        return static_cast<double>(index);
    return static_cast<double>(std::bit_cast<float>(
        state.words[first_word + static_cast<std::uint32_t>(index)]));
}

double SetFloat(GameManagerPropertyState& state, std::int32_t index,
                float value, std::uint32_t first_word, std::uint32_t count) {
    if (index < 0 || static_cast<std::uint32_t>(index) >= count)
        return static_cast<double>(index);
    state.words[first_word + static_cast<std::uint32_t>(index)] =
        std::bit_cast<std::uint32_t>(value);
    return static_cast<double>(value);
}

std::uint32_t AdjustFloat(GameManagerPropertyState& state,
                          std::int32_t index, float delta,
                          std::uint32_t getter_slot,
                          std::uint32_t setter_slot,
                          IGameManagerPropertyRuntime& runtime) {
    const auto vtable = state.words[0];
    const auto current = runtime.GetFloat(vtable, getter_slot, state, index);
    const auto value = static_cast<float>(current + static_cast<double>(delta));
    return runtime.SetFloat(vtable, setter_slot, state, index, value);
}

std::uint32_t SetBounded(GameManagerPropertyState& state,
                         std::uint32_t word, std::int32_t value,
                         std::int32_t exclusive_limit) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    if (value >= 0 && value < exclusive_limit)
        state.words[word] = bits;
    return bits;
}

double AdjustFloatDirect(GameManagerPropertyState& state,
                         std::int32_t index, float delta,
                         std::uint32_t first_word, std::uint32_t count) {
    if (index < 0 || static_cast<std::uint32_t>(index) >= count)
        return static_cast<double>(index);
    auto& bits = state.words[first_word + static_cast<std::uint32_t>(index)];
    const auto sum = static_cast<double>(std::bit_cast<float>(bits)) +
                     static_cast<double>(delta);
    bits = std::bit_cast<std::uint32_t>(static_cast<float>(sum));
    return sum;
}

}  // namespace

double GameManagerPropertyRuntimeStub::GetFloat(
    std::uint32_t, std::uint32_t, GameManagerPropertyState&,
    std::int32_t) { return 0.0; }
std::uint32_t GameManagerPropertyRuntimeStub::SetFloat(
    std::uint32_t, std::uint32_t, GameManagerPropertyState&,
    std::int32_t, float) { return 0u; }
std::uint32_t GameManagerPropertyRuntimeStub::GetInteger(
    std::uint32_t, std::uint32_t, GameManagerPropertyState&,
    std::int32_t) { return 0u; }
std::uint32_t GameManagerPropertyRuntimeStub::SetInteger(
    std::uint32_t, std::uint32_t, GameManagerPropertyState&,
    std::int32_t, std::uint32_t) { return 0u; }

double GameManagerPropertyAccess::GetFloatArrayElement(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x178u / 4u, 4u);
}
double GameManagerPropertyAccess::SetFloatArrayElement(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x178u / 4u, 4u);
}
std::uint32_t GameManagerPropertyAccess::AdjustFloatArrayElement(
    GameManagerPropertyState& state, std::int32_t index, float delta,
    IGameManagerPropertyRuntime& runtime) {
    return AdjustFloat(state, index, delta, 0x2cu, 0x3cu, runtime);
}

double GameManagerPropertyAccess::GetFloatGroup1(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x1b8u / 4u, 6u);
}
double GameManagerPropertyAccess::SetFloatGroup1(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x1b8u / 4u, 6u);
}
std::uint32_t GameManagerPropertyAccess::AdjustFloatGroup1(
    GameManagerPropertyState& state, std::int32_t index, float delta,
    IGameManagerPropertyRuntime& runtime) {
    return AdjustFloat(state, index, delta, 0x70u, 0x74u, runtime);
}

double GameManagerPropertyAccess::GetFloatGroup2(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x1a0u / 4u, 6u);
}
double GameManagerPropertyAccess::SetFloatGroup2(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x1a0u / 4u, 6u);
}
std::uint32_t GameManagerPropertyAccess::AdjustFloatGroup2(
    GameManagerPropertyState& state, std::int32_t index, float delta,
    IGameManagerPropertyRuntime& runtime) {
    return AdjustFloat(state, index, delta, 0x64u, 0x68u, runtime);
}

std::uint32_t GameManagerPropertyAccess::GetIntegerGroup3(
    const GameManagerPropertyState& state, std::int32_t index) {
    return index >= 0 && index < 6
        ? state.words[0x188u / 4u + static_cast<std::uint32_t>(index)]
        : std::bit_cast<std::uint32_t>(index);
}
std::uint32_t GameManagerPropertyAccess::SetIntegerGroup3(
    GameManagerPropertyState& state, std::int32_t index,
    std::uint32_t value) {
    if (index < 0 || index >= 6)
        return std::bit_cast<std::uint32_t>(index);
    return state.words[0x188u / 4u + static_cast<std::uint32_t>(index)] = value;
}
std::uint32_t GameManagerPropertyAccess::AdjustIntegerGroup3(
    GameManagerPropertyState& state, std::int32_t index,
    std::uint32_t delta, IGameManagerPropertyRuntime& runtime) {
    const auto vtable = state.words[0];
    const auto current = runtime.GetInteger(vtable, 0x58u, state, index);
    return runtime.SetInteger(vtable, 0x5cu, state, index, current + delta);
}

double GameManagerPropertyAccess::GetFloatGroup4(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x248u / 4u, 5u);
}
double GameManagerPropertyAccess::SetFloatGroup4(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x248u / 4u, 5u);
}
std::uint32_t GameManagerPropertyAccess::AdjustFloatGroup4(
    GameManagerPropertyState& state, std::int32_t index, float delta,
    IGameManagerPropertyRuntime& runtime) {
    return AdjustFloat(state, index, delta, 0x28u, 0x38u, runtime);
}

double GameManagerPropertyAccess::GetFloatGroup5(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x25cu / 4u, 7u);
}
double GameManagerPropertyAccess::SetFloatGroup5(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x25cu / 4u, 7u);
}
std::uint32_t GameManagerPropertyAccess::AdjustFloatGroup5(
    GameManagerPropertyState& state, std::int32_t index, float delta,
    IGameManagerPropertyRuntime& runtime) {
    return AdjustFloat(state, index, delta, 0x7cu, 0x80u, runtime);
}

std::uint32_t GameManagerPropertyAccess::Get158(const GameManagerPropertyState& state) {
    return state.words[158];
}
std::uint32_t GameManagerPropertyAccess::Set158(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 158u, value, 6);
}
std::uint32_t GameManagerPropertyAccess::Get159(const GameManagerPropertyState& state) {
    return state.words[159];
}
std::uint32_t GameManagerPropertyAccess::Set159(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 159u, value, 7);
}
std::uint32_t GameManagerPropertyAccess::Get160(const GameManagerPropertyState& state) {
    return state.words[160];
}
std::uint32_t GameManagerPropertyAccess::Set160(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 160u, value, 7);
}
std::uint32_t GameManagerPropertyAccess::Get161(const GameManagerPropertyState& state) {
    return state.words[161];
}
std::uint32_t GameManagerPropertyAccess::Set161(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 161u, value, 2);
}
std::uint32_t GameManagerPropertyAccess::Get162(const GameManagerPropertyState& state) {
    return state.words[162];
}
std::uint32_t GameManagerPropertyAccess::Set162(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 162u, value, 4);
}
std::uint32_t GameManagerPropertyAccess::Get163(const GameManagerPropertyState& state) {
    return state.words[163];
}
std::uint32_t GameManagerPropertyAccess::Set163(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 163u, value, 2);
}
std::uint32_t GameManagerPropertyAccess::Get164(const GameManagerPropertyState& state) {
    return state.words[164];
}
std::uint32_t GameManagerPropertyAccess::Set164(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 164u, value, 5);
}
std::uint32_t GameManagerPropertyAccess::Get165(const GameManagerPropertyState& state) {
    return state.words[165];
}
std::uint32_t GameManagerPropertyAccess::Set165(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 165u, value, 3);
}
std::uint32_t GameManagerPropertyAccess::Get166(const GameManagerPropertyState& state) {
    return state.words[166];
}
std::uint32_t GameManagerPropertyAccess::Set166(GameManagerPropertyState& state, std::int32_t value) {
    return SetBounded(state, 166u, value, 6);
}

double GameManagerPropertyAccess::GetFloatGroup6(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x1e4u / 4u, 6u);
}
double GameManagerPropertyAccess::SetFloatGroup6(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x1e4u / 4u, 6u);
}
double GameManagerPropertyAccess::AdjustFloatGroup6(
    GameManagerPropertyState& state, std::int32_t index, float delta) {
    return AdjustFloatDirect(state, index, delta, 0x1e4u / 4u, 6u);
}

double GameManagerPropertyAccess::GetFloatGroup7(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x1fcu / 4u, 7u);
}
double GameManagerPropertyAccess::SetFloatGroup7(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x1fcu / 4u, 7u);
}
double GameManagerPropertyAccess::AdjustFloatGroup7(
    GameManagerPropertyState& state, std::int32_t index, float delta) {
    return AdjustFloatDirect(state, index, delta, 0x1fcu / 4u, 7u);
}

double GameManagerPropertyAccess::GetFloatGroup8(
    const GameManagerPropertyState& state, std::int32_t index) {
    return GetFloat(state, index, 0x218u / 4u, 2u);
}
double GameManagerPropertyAccess::SetFloatGroup8(
    GameManagerPropertyState& state, std::int32_t index, float value) {
    return SetFloat(state, index, value, 0x218u / 4u, 2u);
}
double GameManagerPropertyAccess::AdjustFloatGroup8(
    GameManagerPropertyState& state, std::int32_t index, float delta) {
    return AdjustFloatDirect(state, index, delta, 0x218u / 4u, 2u);
}

}  // namespace pal4
