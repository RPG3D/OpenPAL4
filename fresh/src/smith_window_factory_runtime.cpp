#include "pal4/smith_window_factory_runtime.h"

namespace pal4 {

std::uint32_t SmithWindowFactoryRuntime::Allocate(
    const std::uint32_t bytes) {
    return allocation_.Allocate(bytes);
}

std::uint32_t SmithWindowFactoryRuntime::Construct(
    const std::uint32_t object, const std::uint32_t layout_name) {
    return SmithWindow::Construct(object, layout_name, window_);
}

void SmithWindowFactoryRuntime::Free(const std::uint32_t object) {
    allocation_.Free(object);
}

}  // namespace pal4
