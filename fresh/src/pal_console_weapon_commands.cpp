#include "pal4/pal_console_weapon_commands.h"

namespace pal4 {
namespace {

std::int32_t Apply(bool equip, IPalConsoleWeaponRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadDword(manager + 0x2c50u);
    const auto player = manager + 2836u * index;
    if (player == 0u) return static_cast<std::int32_t>(manager);
    const auto state = runtime.ReadDword(player + 0x1f0u);
    if (state == 0u) return 0;
    return equip ? runtime.ApplyEquipmentVisuals(player)
                 : runtime.ClearWeaponEffects(player);
}

}  // namespace

std::int32_t PalConsoleWeaponCommands::TakeOn(
    IPalConsoleWeaponRuntime& runtime) {
    return Apply(true, runtime);
}

std::int32_t PalConsoleWeaponCommands::TakeOff(
    IPalConsoleWeaponRuntime& runtime) {
    return Apply(false, runtime);
}

}  // namespace pal4
