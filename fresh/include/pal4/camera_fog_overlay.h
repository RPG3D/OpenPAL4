#pragma once

#include "pal4/camera_scene_frame.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICameraFogRuntime {
public:
    virtual ~ICameraFogRuntime() = default;
    [[nodiscard]] virtual std::int32_t RoundToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual float DeviceDepth() = 0;
    [[nodiscard]] virtual std::uint32_t BeginUpdate(
        std::uint32_t camera) = 0;
    virtual void EndUpdate(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t SetDeviceState(
        std::uint32_t state, std::uint32_t value) = 0;
    virtual void DrawQuad(
        std::uint32_t primitive,
        const std::array<CameraScreenVertex, 4u>& vertices,
        std::uint32_t count) = 0;
};

class CameraFogRuntimeStub final : public ICameraFogRuntime {
public:
    [[nodiscard]] std::int32_t RoundToInt(float) override { return 0; }
    [[nodiscard]] std::int32_t Ftol(double) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::int32_t ReadSignedWord(
        std::uint32_t) override { return 0; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 1.0f; }
    [[nodiscard]] float DeviceDepth() override { return 0.0f; }
    [[nodiscard]] std::uint32_t BeginUpdate(
        std::uint32_t) override { return 0u; }
    void EndUpdate(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SetDeviceState(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void DrawQuad(std::uint32_t,
                  const std::array<CameraScreenVertex, 4u>&,
                  std::uint32_t) override {}
};

class CameraFogOverlay final {
public:
    // IDA 0x5fde00.
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t camera, std::uint32_t raster,
        float amount, float distance, float scale,
        bool enhanced, ICameraFogRuntime& runtime);
};

}  // namespace pal4
