#pragma once
#include <cstdint>

namespace pal4 {
// Read-only facet shared by the original camera builtins and live adapters.
class IScriptCameraQueryRuntime {
public:
    virtual ~IScriptCameraQueryRuntime() = default;
    virtual std::uint32_t InitializeCameraSubsystem() = 0;
    virtual std::uint32_t ActiveCameraInternalId(std::uint32_t manager) = 0;
    virtual float CameraFloat(std::uint32_t address) = 0;
};
} // namespace pal4
