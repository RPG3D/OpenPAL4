#include "pal4/player_manager_slot_script_data.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotScriptRuntimeStub::GetValue(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotScriptRuntimeStub::GetMaximum(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotScriptRuntimeStub::SetValue(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotScriptRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotScriptRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotScriptRuntimeStub::ProcessInventory(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotScriptRuntimeStub::ProcessSkills(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t PlayerManagerSlotScriptData::Refresh(
    std::uint32_t object, IPlayerManagerSlotScriptRuntime& runtime) {
    const auto value7 = runtime.GetValue(object, 7u);
    const auto value9 = runtime.GetValue(object, 9u);
    const auto value8 = runtime.GetValue(object, 8u);
    for (std::uint32_t index = 0u; index < 0x54u; ++index)
        runtime.WriteDword(object + 0x154u + index * 4u,
                           runtime.ReadDword(object + 4u + index * 4u));
    runtime.ProcessInventory(object + 0x2ccu,
                             object + 0x154u, object + 4u);
    runtime.ProcessSkills(object, 1u,
                          object == 0x654u ? 0u : object);
    static_cast<void>(runtime.SetValue(object, 7u, value7));
    static_cast<void>(runtime.SetValue(object, 9u, value9));
    static_cast<void>(runtime.SetValue(object, 8u, value8));

    const auto current7 = static_cast<std::int32_t>(
        runtime.GetValue(object, 7u));
    const auto maximum0 = static_cast<std::int32_t>(
        runtime.GetMaximum(object, 0u));
    if (current7 > maximum0)
        static_cast<void>(runtime.SetValue(
            object, 7u, runtime.GetMaximum(object, 0u)));

    const auto current9 = static_cast<std::int32_t>(
        runtime.GetValue(object, 9u));
    const auto maximum2 = runtime.GetMaximum(object, 2u);
    if (current9 > static_cast<std::int32_t>(maximum2))
        return runtime.SetValue(object, 9u,
                                runtime.GetMaximum(object, 0u));
    return maximum2;
}

}  // namespace pal4
