#pragma once

#include <cstdint>
#include <optional>

namespace pal4 {

class ICameraAllocationRuntime {
public:
    virtual ~ICameraAllocationRuntime() = default;
    virtual void DestroyDevice(std::uint32_t camera_data) = 0;
    virtual void DestroyData(std::uint32_t camera_data) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t camera_data) = 0;
};

class CameraAllocation final {
public:
    explicit CameraAllocation(ICameraAllocationRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eab20: destroy nested camera, free allocation and clear owner slot.
    [[nodiscard]] std::optional<std::uint32_t> Release(
        std::uint32_t& camera_data);

private:
    ICameraAllocationRuntime& runtime_;
};

}  // namespace pal4
