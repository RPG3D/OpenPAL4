#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_data.h"
#include "pal4/camera_system_object.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICameraConfigRuntime {
public:
    virtual ~ICameraConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateNode(std::string_view name) = 0;
    [[nodiscard]] virtual bool HasAttribute(std::uint32_t node,
                                            std::string_view name) = 0;
    virtual void SetInt(std::uint32_t node, std::string_view name,
                        std::uint32_t value) = 0;
    virtual void AddInt(std::uint32_t node, std::string_view name,
                        std::uint32_t value) = 0;
    virtual void SetFloat(std::uint32_t node, std::string_view name,
                          float value) = 0;
    virtual void AddFloat(std::uint32_t node, std::string_view name,
                          float value) = 0;
    [[nodiscard]] virtual CameraSystemObjectState& ObjectAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual CameraDataState& CameraDataAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PrepareWriter(
        std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint8_t Write(std::uint32_t node,
                                             std::uint32_t writer) = 0;
    virtual void DestroyNode(std::uint32_t node) = 0;
};

class CameraConfigRuntimeStub final : public ICameraConfigRuntime {
public:
    [[nodiscard]] std::uint32_t CreateNode(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] bool HasAttribute(std::uint32_t,
                                    std::string_view) override {
        return false;
    }
    void SetInt(std::uint32_t, std::string_view,
                std::uint32_t) override {}
    void AddInt(std::uint32_t, std::string_view,
                std::uint32_t) override {}
    void SetFloat(std::uint32_t, std::string_view,
                  float) override {}
    void AddFloat(std::uint32_t, std::string_view,
                  float) override {}
    [[nodiscard]] CameraSystemObjectState& ObjectAt(
        std::uint32_t) override { return object_; }
    [[nodiscard]] CameraDataState& CameraDataAt(
        std::uint32_t) override { return camera_; }
    [[nodiscard]] std::uint32_t PrepareWriter(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t Write(std::uint32_t,
                                     std::uint32_t) override { return 0u; }
    void DestroyNode(std::uint32_t) override {}

private:
    CameraSystemObjectState object_{};
    CameraDataState camera_{};
};

class CameraConfigPersistence final {
public:
    explicit CameraConfigPersistence(ICameraConfigRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ebd00: serialize current mode and Main camera yaw/pitch.
    [[nodiscard]] std::uint8_t Save(const CameraControllerState& state,
                                    std::uint32_t writer_argument);

private:
    ICameraConfigRuntime& runtime_;
};

}  // namespace pal4
