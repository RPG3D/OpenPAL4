#pragma once

#include "pal4/camera_projection_matrix.h"

#include <cstdint>

namespace pal4 {

class ICameraRenderGeometryCallbackRuntime {
public:
    virtual ~ICameraRenderGeometryCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void SetPixelShader(std::uint32_t shader) = 0;
    virtual void EnableClipping(std::uint32_t clipping,
                                std::uint8_t flags) = 0;
    virtual void SetIndices(std::uint32_t indices) = 0;
    virtual void SetStreams(std::uint32_t streams,
                            std::uint32_t value) = 0;
    virtual void SetVertexDeclaration(std::uint32_t declaration) = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t GetDeviceState(
        std::uint32_t state) = 0;
    virtual void SetDeviceState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void SetTexture(std::uint32_t texture,
                                 std::uint32_t stage) = 0;
    virtual void SetTextureStageState(
        std::uint32_t stage, std::uint32_t state,
        std::uint32_t value) = 0;
    virtual void SetSurfaceProperties(std::uint32_t surface,
                                      std::uint32_t color,
                                      std::uint32_t flags) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void SetVertexShader(std::uint32_t shader) = 0;
    virtual void DrawIndexed(std::uint32_t primitive,
                             std::uint32_t first, std::uint32_t base,
                             std::uint32_t count,
                             std::uint32_t start,
                             std::uint32_t index_count) = 0;
    virtual void DrawPrimitive(std::uint32_t primitive,
                               std::uint32_t first,
                               std::uint32_t count) = 0;
};

class CameraRenderGeometryCallback final {
public:
    // IDA 0x5fd300: void cdecl(resource_entry, object, type, flags).
    // Projection state and runtime are explicit host dependencies.
    static void Render(
        std::uint32_t render_data, std::uint32_t clipping,
        std::uint8_t clipping_flags, std::uint32_t render_flags,
        const CameraProjectionGlobals& projection,
        ICameraRenderGeometryCallbackRuntime& runtime);
};

}  // namespace pal4
