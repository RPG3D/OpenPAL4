#pragma once
#include "pal4/clump_bounding_box_builder.h"
#include "pal4/atomic_bounding_box_builder.h"

namespace pal4 {
// Both references are borrowed. The mode word must also be the secondary mode
// observed by atomic_runtime; it persists after traversal, as in the original.
class NativeClumpBoundingBoxRuntime final : public IClumpBoundingBoxRuntime {
public:
    NativeClumpBoundingBoxRuntime(std::uint32_t& secondary_mode,
                                 IAtomicBoundingBoxRuntime& atomic_runtime)
        : secondary_mode_(secondary_mode), atomic_runtime_(atomic_runtime) {}
    void SetDirectTransformMode(std::uint32_t value) override;
    std::uint32_t ForEachAtomic(std::uint32_t clump, std::uint32_t box) override;
private:
    std::uint32_t& secondary_mode_;
    IAtomicBoundingBoxRuntime& atomic_runtime_;
};
}
