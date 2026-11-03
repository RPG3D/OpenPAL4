#include "pal4/clump_bounding_box_builder.h"

namespace pal4 {

std::uint32_t ClumpBoundingBoxBuilder::Build(
    const std::uint32_t clump, const std::uint32_t direct_mode,
    const std::uint32_t box, IClumpBoundingBoxRuntime& runtime) {
    runtime.SetDirectTransformMode(direct_mode);
    return runtime.ForEachAtomic(clump, box);
}

}  // namespace pal4
