#include "pal4/cegui_rectangle_queue.h"

namespace pal4 {

void CeguiRectangleQueue::Clear(CeguiRendererState& state) noexcept {
    state.WriteDword(192u, state.ReadDword(188u));
}

}  // namespace pal4
