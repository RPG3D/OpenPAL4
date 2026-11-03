#include "pal4/ui_frame_manager_cegui.h"

#include <cstdint>

namespace pal4 {

void UiFrameManagerCegui::RendererDestructorThunk(
    const std::uint32_t renderer,
    ICeguiRendererDestructorRuntime& runtime) {
    runtime.DestructRenderer(renderer);
}

}  // namespace pal4
