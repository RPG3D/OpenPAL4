#include "pal4/property_window_callback_clone.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t PropertyWindowCallbackClone::Clone(
    const std::uint32_t source, const std::uint32_t base_vtable,
    const std::uint32_t final_vtable,
    IPropertyWindowCallbackCloneRuntime& runtime) {
    const auto result = runtime.Allocate(0x18u);
    if (result == 0u) return 0u;
    runtime.WriteWord(result, base_vtable);
    for (std::uint32_t offset : {8u, 12u, 16u}) {
        runtime.WriteWord(result + offset, runtime.ReadWord(source + offset));
    }
    runtime.WriteWord(result, final_vtable);
    return result;
}

std::uint32_t PropertyWindowCallbackClone::CloneUseItem(
    const std::uint32_t source,
    IPropertyWindowCallbackCloneRuntime& runtime) {
    return Clone(source, 0x843500u, 0x843a20u, runtime);
}

std::uint32_t PropertyWindowCallbackClone::CloneDescription(
    const std::uint32_t source,
    IPropertyWindowCallbackCloneRuntime& runtime) {
    return Clone(source, 0x8435d0u, 0x843a6cu, runtime);
}

}  // namespace pal4
