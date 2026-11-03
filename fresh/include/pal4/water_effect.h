#pragma once

#include "pal4/screen_effect_script.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

struct WaterEffectState {
    std::uint32_t vtable{};
    std::uint8_t loaded{};
    std::uint8_t enabled{};
    std::uint8_t reserved[2]{};
    float time{};
    std::uint32_t script{};
};
static_assert(sizeof(WaterEffectState) == 16u);
static_assert(offsetof(WaterEffectState, time) == 8u);
static_assert(offsetof(WaterEffectState, script) == 12u);

class IWaterEffectRuntime {
public:
    virtual ~IWaterEffectRuntime() = default;
    virtual void Reset(WaterEffectState& state) = 0;
    [[nodiscard]] virtual bool CheckPixelShader(std::uint32_t version) = 0;
    [[nodiscard]] virtual bool CheckVertexShader(std::uint32_t version) = 0;
    [[nodiscard]] virtual std::int32_t ScreenWidth() = 0;
    [[nodiscard]] virtual std::int32_t ScreenHeight() = 0;
    virtual void SetInteger(std::uint32_t script, std::string_view name,
                            std::int32_t value) = 0;
};

class WaterEffectRuntimeStub final : public IWaterEffectRuntime {
public:
    void Reset(WaterEffectState& state) override;
    [[nodiscard]] bool CheckPixelShader(std::uint32_t version) override;
    [[nodiscard]] bool CheckVertexShader(std::uint32_t version) override;
    [[nodiscard]] std::int32_t ScreenWidth() override;
    [[nodiscard]] std::int32_t ScreenHeight() override;
    void SetInteger(std::uint32_t script, std::string_view name,
                    std::int32_t value) override;
};

class WaterEffect final {
public:
    [[nodiscard]] static std::uint8_t Enable(WaterEffectState& state);
    [[nodiscard]] static std::uint32_t Apply(
        const WaterEffectState& state, std::uint32_t input,
        IScreenEffectRuntime& script_runtime, IWaterEffectRuntime& water_runtime);
    [[nodiscard]] static std::uint8_t Render(
        const WaterEffectState& state, IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetParameter(
        const WaterEffectState& state, std::uint32_t value,
        IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopRendering(
        const WaterEffectState& state, IScreenEffectRuntime& runtime);
    [[nodiscard]] static bool Load(
        WaterEffectState& state, std::uint32_t message_box_type,
        std::uint32_t environment_word, IScreenEffectRuntime& script_runtime,
        IWaterEffectRuntime& water_runtime);
    [[nodiscard]] static std::uint8_t Cleanup(
        WaterEffectState& state, IScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Advance(
        WaterEffectState& state, double delta);
};

struct WaterVertex {
    float x{};
    float y{};
    float z{};
    std::uint32_t color{};
    float u{};
    float v{};
};
static_assert(sizeof(WaterVertex) == 24u);

class IWaterMeshRuntime {
public:
    virtual ~IWaterMeshRuntime() = default;
    virtual void BindRaster(std::uint32_t raster) = 0;
    virtual void SetRenderState(std::uint32_t state, std::uint32_t value) = 0;
    virtual void SetFvf(std::uint32_t fvf) = 0;
    virtual void DrawTriangleStrip(const WaterVertex* vertices,
                                   std::uint32_t primitive_count,
                                   std::uint32_t stride) = 0;
    [[nodiscard]] virtual std::uint32_t RestoreState(
        std::uint32_t state, std::uint32_t value) = 0;
};

class WaterMeshRuntimeStub final : public IWaterMeshRuntime {
public:
    void BindRaster(std::uint32_t raster) override;
    void SetRenderState(std::uint32_t state, std::uint32_t value) override;
    void SetFvf(std::uint32_t fvf) override;
    void DrawTriangleStrip(const WaterVertex* vertices,
                           std::uint32_t primitive_count,
                           std::uint32_t stride) override;
    [[nodiscard]] std::uint32_t RestoreState(
        std::uint32_t state, std::uint32_t value) override;
};

struct WaterFrameImage {
    std::uint32_t opaque{};
    std::uint32_t raster{};
};
static_assert(offsetof(WaterFrameImage, raster) == 4u);

struct WaterFrameContext {
    std::uint32_t opaque[4]{};
    const WaterFrameImage* image{};
};
static_assert(offsetof(WaterFrameContext, image) == 16u);

class WaterMeshRenderer final {
public:
    [[nodiscard]] std::uint32_t Render(
        const WaterEffectState& state, std::uint32_t raster,
        float left, float top, float width, float height,
        std::uint32_t color, IWaterMeshRuntime& runtime);
    [[nodiscard]] std::uint8_t RenderFullScreen(
        const WaterEffectState& state, const WaterFrameContext& frame,
        std::int32_t screen_width, std::int32_t screen_height,
        IWaterMeshRuntime& runtime);
    void Cleanup();
    [[nodiscard]] const std::vector<WaterVertex>& Vertices() const;

private:
    std::vector<WaterVertex> vertices_;
};

}  // namespace pal4
