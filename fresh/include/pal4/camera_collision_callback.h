#pragma once

#include "pal4/camera_subsystem_lifecycle.h"
#include "pal4/collection_collision_scanner.h"

#include <array>
#include <cstdint>

namespace pal4 {

class CameraCollisionCallback final {
public:
    CameraCollisionCallback(CameraSubsystemLifecycle& lifecycle,
                            CameraSubsystemRegistrationState& registration,
                            ICameraSubsystemRuntime& runtime,
                            CollectionCollisionScanner& scanner)
        : lifecycle_(lifecycle), registration_(registration),
          runtime_(runtime), scanner_(scanner) {}

    // 0x5ec270: resolve the camera manager before scanning its clumps.
    [[nodiscard]] bool Probe(const float* query, float& limit,
                             std::array<std::uint32_t, 3>& point);

private:
    CameraSubsystemLifecycle& lifecycle_;
    CameraSubsystemRegistrationState& registration_;
    ICameraSubsystemRuntime& runtime_;
    CollectionCollisionScanner& scanner_;
};

}  // namespace pal4
