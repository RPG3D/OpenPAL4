#include "pal4/rp_hanim_hierarchy.h"

namespace pal4 {

std::uint32_t RpHAnimHierarchy::FindInFrame(
    const std::uint32_t frame, std::uint32_t& found,
    IRpHAnimHierarchyRuntime& runtime) {
    const auto hierarchy = runtime.FrameHierarchy(frame);
    if (hierarchy != 0) {
        found = hierarchy;
        return 0;
    }
    runtime.ForAllChildren(frame, &FindInFrame, found);
    return frame;
}

std::uint32_t RpHAnimHierarchy::FindInClump(
    const std::uint32_t clump, IRpHAnimHierarchyRuntime& runtime) {
    if (clump == 0) return 0;
    std::uint32_t found = 0;
    runtime.ForAllChildren(runtime.ClumpFrame(clump),
                           &FindInFrame, found);
    return found;
}

bool RpHAnimHierarchy::HasHierarchy(
    const std::uint32_t clump, IRpHAnimHierarchyRuntime& runtime) {
    std::uint32_t found = 0;
    runtime.ForAllAtomicsGetId(clump, found);
    return found != 0;
}

std::uint32_t* RpHAnimHierarchy::SetFlags(
    std::uint32_t& flags, const std::uint32_t mask) {
    flags |= mask;
    return &flags;
}

}  // namespace pal4
