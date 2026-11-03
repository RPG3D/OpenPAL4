#include "pal4/smith_window_factory.h"

namespace pal4 {

std::uint32_t SmithWindowFactory::Create(
    const std::uint32_t layout_name,
    ISmithWindowFactoryRuntime& runtime) {
    const auto object = runtime.Allocate(0xfcu);
    if (object == 0u) return 0u;
    try {
        return runtime.Construct(object, layout_name);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

}  // namespace pal4
