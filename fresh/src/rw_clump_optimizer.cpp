#include "pal4/rw_clump_optimizer.h"

namespace pal4 {

std::uint32_t RwClumpOptimizer::OptimizeAtomic(
    const std::uint32_t atomic, const std::uint32_t context,
    IRwClumpOptimizerRuntime& runtime) {
    const auto geometry = runtime.ReadOptimizerWord(atomic + 24);
    if (geometry != 0) {
        (void)runtime.ForAllOptimizerMeshes(
            geometry, &SetMeshTriStripMethod, context);
    }
    runtime.OptimizeAtomicRaw(atomic);
    return atomic;
}

std::uint32_t RwClumpOptimizer::SetMeshTriStripMethod(
    const std::uint32_t mesh, const std::uint32_t mesh_header,
    const std::uint32_t context, IRwClumpOptimizerRuntime& runtime) {
    runtime.SetTriStripMethod(context);
    const auto flags = runtime.ReadOptimizerWord(mesh_header);
    runtime.WriteOptimizerWord(
        mesh_header, (flags & 0xffffff00u) | runtime.TriStripMeshFlag());
    return mesh;
}

std::uint32_t RwClumpOptimizer::OptimizeClump(
    const std::uint32_t clump, IRwClumpOptimizerRuntime& runtime) {
    if (clump == 0) return 0;
    runtime.OptimizeClumpRaw(clump, 0);
    return runtime.ForAllOptimizerAtomics(clump, &OptimizeAtomic, 0);
}

}  // namespace pal4
