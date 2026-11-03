#pragma once

#include "pal4/script_unique_collection.h"
#include "pal4/camera_controller.h"

#include <array>
#include <cstdint>
#include <optional>

namespace pal4 {

struct CollectionCollisionHit {
    float distance{};
    std::array<std::uint32_t, 3> point{};
};

class ICollectionCollisionRuntime {
public:
    virtual ~ICollectionCollisionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::optional<CollectionCollisionHit> Probe(
        const float* query, std::uint32_t object) = 0;
};

class CollectionCollisionScanner final {
public:
    explicit CollectionCollisionScanner(ICollectionCollisionRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ebc80: retain the nearest collision under the caller's limit.
    [[nodiscard]] bool FindNearest(
        const ScriptUniqueCollectionState& state, const float* query,
        float& limit, std::array<std::uint32_t, 3>& point);
    [[nodiscard]] bool FindNearest(
        const CameraControllerState& state, const float* query,
        float& limit, std::array<std::uint32_t, 3>& point);

private:
    ICollectionCollisionRuntime& runtime_;
};

}  // namespace pal4
