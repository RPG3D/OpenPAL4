#include "pal4/object_clump_collision.h"

namespace pal4 {

std::int32_t ObjectClumpCollisionRuntimeStub::ProcessClumpCollision(
    std::uint32_t, std::uint32_t, std::array<std::uint8_t, 24>&) {
    return 0;
}

std::int32_t ObjectClumpCollision::ProcessConditional(
    std::uint32_t vector_address, std::uint32_t clump_address,
    IObjectClumpCollisionRuntime& runtime) {
    if (clump_address == 0u) return 0;
    std::array<std::uint8_t, 24> scratch{};
    return runtime.ProcessClumpCollision(
        vector_address, clump_address, scratch);
}

}  // namespace pal4
