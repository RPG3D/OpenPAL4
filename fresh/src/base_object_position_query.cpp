#include "pal4/base_object_position_query.h"

namespace pal4 {

std::uint32_t BaseObjectPositionQuery::GetPositionAndData(
    const BaseObjectState& state, const BaseObjectVector3& position,
    std::uint32_t* output, IBaseObjectPositionQueryRuntime& runtime) {
    const auto clump = state.Read(0x14u);
    const auto count = runtime.MaterialCount(clump);
    if (count == 0u) return count;
    const auto result = runtime.ProcessClumpCollision(position, clump);
    if (output != nullptr) *output = result.second;
    return result.first;
}

}  // namespace pal4
