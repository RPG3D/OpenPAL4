#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_data.h"
#include "pal4/camera_system_object.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class CameraModeConfiguration;
class CameraParameterSetters;

class ICameraConfigLoadRuntime {
public:
    virtual ~ICameraConfigLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateNode(std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PrepareReader(
        std::uint32_t argument) = 0;
    [[nodiscard]] virtual bool IsCompatible(std::uint32_t reader,
                                             std::uint32_t node) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> FindAttribute(
        std::uint32_t node, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ParseInt(
        std::uint32_t attribute, std::uint32_t fallback) = 0;
    [[nodiscard]] virtual float ParseFloat(
        std::uint32_t attribute, float fallback) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual CameraSystemObjectState& ObjectAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual CameraDataState& CameraDataAt(
        std::uint32_t address) = 0;
    virtual void DestroyNode(std::uint32_t node) = 0;
};

class CameraConfigLoadRuntimeStub final : public ICameraConfigLoadRuntime {
public:
    [[nodiscard]] std::uint32_t CreateNode(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t PrepareReader(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] bool IsCompatible(std::uint32_t,
                                     std::uint32_t) override { return false; }
    [[nodiscard]] std::optional<std::uint32_t> FindAttribute(
        std::uint32_t, std::string_view) override { return std::nullopt; }
    [[nodiscard]] std::uint32_t ParseInt(std::uint32_t,
                                         std::uint32_t fallback) override {
        return fallback;
    }
    [[nodiscard]] float ParseFloat(std::uint32_t,
                                    float fallback) override {
        return fallback;
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] CameraSystemObjectState& ObjectAt(
        std::uint32_t) override { return object_; }
    [[nodiscard]] CameraDataState& CameraDataAt(
        std::uint32_t) override { return camera_; }
    void DestroyNode(std::uint32_t) override {}

private:
    CameraSystemObjectState object_{};
    CameraDataState camera_{};
};

class CameraConfigLoader final {
public:
    CameraConfigLoader(ICameraConfigLoadRuntime& runtime,
                       CameraModeConfiguration& modes,
                       CameraParameterSetters& parameters)
        : runtime_(runtime), modes_(modes), parameters_(parameters) {}

    // 0x5ebfb0: load mode, pitch and yaw, then update Main camera angles.
    [[nodiscard]] bool Load(CameraControllerState& state,
                            std::uint32_t controller_address,
                            std::uint32_t reader_argument);

private:
    [[nodiscard]] float TableFloat(const CameraControllerState& state,
                                   std::uint32_t controller_address,
                                   std::uint32_t address);

    ICameraConfigLoadRuntime& runtime_;
    CameraModeConfiguration& modes_;
    CameraParameterSetters& parameters_;
};

}  // namespace pal4
