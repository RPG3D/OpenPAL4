#include "pal4/game_object_property_access.h"

#include <bit>

namespace pal4 {

std::uint32_t GameObjectPropertyRuntimeStub::CallGetter(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) { return 0u; }
std::uint32_t GameObjectPropertyRuntimeStub::CallSetter(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
void GameObjectPropertyRuntimeStub::NotifyHealthStatus(
    std::uint32_t) {}

namespace {

std::uint32_t AdjustVirtual(
    const GameObjectPropertyState& state, std::uint32_t object,
    std::uint32_t index, std::uint32_t delta,
    std::uint32_t getter_slot, std::uint32_t setter_slot,
    IGameObjectPropertyRuntime& runtime) {
    const auto vtable = state.words[0];
    const auto previous = runtime.CallGetter(vtable, getter_slot,
                                             object, index);
    return runtime.CallSetter(vtable, setter_slot, object,
                              index, previous + delta);
}

}  // namespace

std::uint32_t GameObjectPropertyAccess::GetHeightLimitFlag(
    const GameObjectPropertyState& state) {
    return state.words[104];
}

std::uint32_t GameObjectPropertyAccess::Get85(
    const GameObjectPropertyState& state) {
    return state.words[85];
}
std::uint32_t GameObjectPropertyAccess::Set85(
    GameObjectPropertyState& state, std::uint32_t value) {
    return state.words[85] = value;
}
std::uint32_t GameObjectPropertyAccess::Get168(
    const GameObjectPropertyState& state) {
    return state.words[168];
}
std::uint32_t GameObjectPropertyAccess::Set168(
    GameObjectPropertyState& state, std::uint32_t value) {
    return state.words[168] = value;
}
std::uint32_t GameObjectPropertyAccess::Get167(
    const GameObjectPropertyState& state) {
    return state.words[167];
}
std::uint32_t GameObjectPropertyAccess::Set167And83(
    GameObjectPropertyState& state, std::uint32_t value) {
    state.words[83] = value;
    return state.words[167] = value;
}
std::uint32_t GameObjectPropertyAccess::GetIndexed116(
    const GameObjectPropertyState& state, std::uint32_t index) {
    return index < 5u ? state.words[116u + index] : index;
}
std::uint32_t GameObjectPropertyAccess::SetIndexed116(
    GameObjectPropertyState& state, std::uint32_t index,
    std::uint32_t value) {
    if (index >= 5u)
        return index;
    return state.words[116u + index] = value;
}
std::uint32_t GameObjectPropertyAccess::AdjustIndexed(
    const GameObjectPropertyState& state, std::uint32_t object,
    std::uint32_t index, std::uint32_t delta,
    IGameObjectPropertyRuntime& runtime) {
    return AdjustVirtual(state, object, index, delta,
                         0x1cu, 0x20u, runtime);
}

std::uint32_t GameObjectPropertyAccess::GetIndexed136(
    const GameObjectPropertyState& state, std::uint32_t index) {
    return index < 10u ? state.words[136u + index] : index;
}

std::uint32_t GameObjectPropertyAccess::SetIndexed136(
    GameObjectPropertyState& state, std::uint32_t object,
    std::uint32_t index, std::int32_t value,
    IGameObjectPropertyRuntime& runtime) {
    const auto raw = std::bit_cast<std::uint32_t>(value);
    if (index == 4u)
        state.words[56] = raw;
    if (index >= 10u)
        return index;

    auto& property = state.words[136u + index];
    property = raw;
    if (index == 7u || index == 9u) {
        const auto maximum = std::bit_cast<std::int32_t>(
            state.words[index == 7u ? 86u : 88u]);
        if (value > maximum)
            property = state.words[index == 7u ? 86u : 88u];
        else if (value < 0)
            property = 0u;
    } else if (index == 8u) {
        if (value > 100)
            property = 100u;
        else if (value < 0)
            property = 0u;
    }
    const auto result = property;
    if (index == 7u)
        runtime.NotifyHealthStatus(object);
    return result;
}

std::uint32_t GameObjectPropertyAccess::AdjustProperty(
    const GameObjectPropertyState& state, std::uint32_t object,
    std::uint32_t index, std::uint32_t delta,
    IGameObjectPropertyRuntime& runtime) {
    return AdjustVirtual(state, object, index, delta,
                         0x34u, 0x44u, runtime);
}

std::uint32_t GameObjectPropertyAccess::GetIndexed86(
    const GameObjectPropertyState& state, std::uint32_t index) {
    return index < 8u ? state.words[86u + index] : index;
}

std::uint32_t GameObjectPropertyAccess::SetIndexed86(
    GameObjectPropertyState& state, std::uint32_t index,
    std::uint32_t value) {
    if (index >= 8u)
        return index;
    return state.words[86u + index] = value;
}

std::uint32_t GameObjectPropertyAccess::AdjustRecordValue(
    const GameObjectPropertyState& state, std::uint32_t object,
    std::uint32_t index, std::uint32_t delta,
    IGameObjectPropertyRuntime& runtime) {
    return AdjustVirtual(state, object, index, delta,
                         0x30u, 0x40u, runtime);
}

}  // namespace pal4
