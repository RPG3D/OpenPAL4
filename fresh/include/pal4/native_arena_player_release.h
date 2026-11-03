#pragma once
#include <cstddef>
#include <cstdint>
namespace pal4 {
struct PlayerManagerObjectState;
class NativeSceneMovementAsset;
class IBaseObjectActivityRuntime;
class IBaseObjectMaterialRuntime;
// IDA51B4BF..51B4D2. Call after actor borrowers retire, before equipment/effects.
// Loaded models and animation owners survive; movement/world must still be live.
class NativeArenaPlayerRelease final {
public:
    static std::size_t LeaveAll(PlayerManagerObjectState& players, std::uint32_t arena,
        NativeSceneMovementAsset& movement, IBaseObjectActivityRuntime& activity,
        IBaseObjectMaterialRuntime& materials, bool update_materials = true);
};
}  // namespace pal4
