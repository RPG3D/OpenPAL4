#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleCameraRuntime {
public:
    virtual ~IPalConsoleCameraRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetModeByIndex(std::uint32_t manager, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t manager) = 0;
    virtual void SetLookAtFromObjectData(std::uint32_t camera,
                                         std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void FlushMainCameraTailYaw(std::uint32_t control) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraInternalId(
        std::uint32_t manager) = 0;
    virtual void SetPositionAndUpdateMatrix(std::uint32_t camera,
                                            std::uint32_t x,
                                            std::uint32_t y,
                                            std::uint32_t z,
                                            std::uint32_t update) = 0;
    [[nodiscard]] virtual std::int32_t SetDistanceOptimization(
        std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleCameraCommands final {
public:
    // IDA 0x5bc5c0 / 0x5bc650 / 0x5bc870.
    [[nodiscard]] static std::int32_t SetAutoseek(
        const char* command, IPalConsoleCameraRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCameraMode(
        const char* command, IPalConsoleCameraRuntime& runtime);
    [[nodiscard]] static std::int32_t SetDistanceOptionEnabled(
        const char* command, IPalConsoleCameraRuntime& runtime);
};

}  // namespace pal4
