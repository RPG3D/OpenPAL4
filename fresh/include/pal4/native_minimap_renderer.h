#pragma once
#include <cstdint>

namespace pal4 {
class IMinimapIconRuntime;
class IMinimapRasterUpload;
// 0x40DF00, called by WorldState_Render before the UI (0x58EF7E).
// All addresses are borrowed; context points to the active RwCamera* slot.
void DrawNativeMinimap(std::uint32_t manager, std::uint32_t context,
                       IMinimapIconRuntime& icons, IMinimapRasterUpload& upload);
}
