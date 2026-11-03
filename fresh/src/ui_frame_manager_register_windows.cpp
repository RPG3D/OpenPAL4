#include "pal4/ui_frame_manager_singleton.h"

#include <array>
#include <cstdint>

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::RegisterWindows(
    const std::uint32_t manager,
    IUiFrameManagerRegisterWindowsRuntime& runtime) {
    constexpr std::array<const char*, 11> names{
        "roleStateWindow", "EquipmentWindow", "magicWindow",
        "MissionWindow", "PropertyWindow", "SystemSetting",
        "sysToolBar", "SmithWindow", "TradeWindow",
        "sceneWindow", "portrait"};
    std::uint32_t result = 0u;
    for (const auto* name : names)
        result = runtime.RegisterFrame(manager, name);
    return result;
}

}  // namespace pal4
