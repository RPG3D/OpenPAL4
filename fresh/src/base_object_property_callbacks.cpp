#include "pal4/base_object_property_callbacks.h"

namespace pal4 {

std::uint32_t BaseObjectPropertyCallbacks::GetProperty(
    BaseObjectState& state,
    IBaseObjectPropertyCallbacksRuntime& runtime) {
    return runtime.GetProperty(state, 0x564u);
}

std::uint32_t BaseObjectPropertyCallbacks::Initialize(
    BaseObjectState& state, std::uint32_t property_id,
    IBaseObjectPropertyCallbacksRuntime& runtime) {
    (void)GetProperty(state, runtime);
    runtime.RegisterCountCallback(state, 0x564u);
    runtime.RegisterByIndexCallback(state, 0x564u);
    runtime.RegisterByIdCallback(state, 0x564u);
    runtime.RegisterActiveCallback(state, 0x564u);
    const auto property_bits = state.Read(0xdcu);
    const auto converted = runtime.ConvertZeroToInt64();
    return runtime.SetInitialProperty(state, 0x564u, property_id,
                                      0x41873333u, converted, 0u,
                                      property_bits);
}

}  // namespace pal4
