#include "pal4/atomic_vertex_color.h"

namespace pal4 {

std::uint32_t AtomicVertexColor::SetClumpColor(
    const std::uint32_t clump, const std::uint32_t color,
    IAtomicVertexColorRuntime& runtime) {
    return runtime.ForEachAtomic(clump, VertexColorPass::set, &color);
}

std::uint32_t AtomicVertexColor::SetAtomicColor(
    const std::uint32_t atomic, const std::uint32_t& color,
    IAtomicVertexColorRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    const auto flags = runtime.ReadWord(geometry + 8u);
    if ((flags & 8u) == 0u) return atomic;
    const auto locked = runtime.LockGeometry(geometry, 8u);
    const auto colors = runtime.ReadWord(locked + 48u);
    const auto count = static_cast<std::int32_t>(runtime.ReadWord(locked + 20u));
    if (colors == 0u) {
        runtime.WriteWord(locked + 8u, flags & ~8u);
        return atomic;
    }
    for (std::int32_t index = 0; index < count; ++index)
        runtime.WriteWord(colors + static_cast<std::uint32_t>(index) * 4u,
                          color);
    runtime.UnlockGeometry(geometry);
    return atomic;
}

std::uint32_t AtomicVertexColor::ClearClumpColor(
    const std::uint32_t clump, const std::uint32_t mask,
    IAtomicVertexColorRuntime& runtime) {
    return runtime.ForEachAtomic(clump, VertexColorPass::clear, &mask);
}

std::uint32_t AtomicVertexColor::ClearAtomicColor(
    const std::uint32_t atomic, const std::uint32_t& mask,
    IAtomicVertexColorRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    runtime.WriteWord(geometry + 8u,
                      runtime.ReadWord(geometry + 8u) & ~mask);
    return atomic;
}

}  // namespace pal4
