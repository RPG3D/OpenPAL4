#include "pal4/collection_collision_scanner.h"

namespace pal4 {

namespace {

template <class Collection>
bool FindNearestInCollection(
    const Collection& state, const float* query, float& limit,
    std::array<std::uint32_t, 3>& point,
    ICollectionCollisionRuntime& runtime) {
    bool found = false;
    for (auto current = state.At(4u); current != state.At(8u);
         current += 4u) {
        const auto hit = runtime.Probe(query, runtime.ReadWord(current));
        if (hit && hit->distance < static_cast<double>(limit)) {
            limit = hit->distance;
            point = hit->point;
            found = true;
        }
    }
    return found;
}

}  // namespace

bool CollectionCollisionScanner::FindNearest(
    const ScriptUniqueCollectionState& state, const float* query,
    float& limit, std::array<std::uint32_t, 3>& point) {
    return FindNearestInCollection(state, query, limit, point, runtime_);
}

bool CollectionCollisionScanner::FindNearest(
    const CameraControllerState& state, const float* query,
    float& limit, std::array<std::uint32_t, 3>& point) {
    return FindNearestInCollection(state, query, limit, point, runtime_);
}

}  // namespace pal4
