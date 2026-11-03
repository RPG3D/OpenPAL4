#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct TextureObjectState final {
    std::uint32_t first_raster = 0u;
    std::uint32_t second_raster = 0u;
    std::uint32_t context = 0u;
    std::uint32_t texture = 0u;
};

static_assert(sizeof(TextureObjectState) == 16);
static_assert(offsetof(TextureObjectState, context) == 8);

class ITextureObjectRuntime {
public:
    virtual ~ITextureObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t type) = 0;
    virtual void DestroyRaster(std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t raster) = 0;
    virtual void SetTextureRaster(std::uint32_t texture,
                                  std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyTexture(
        std::uint32_t texture) = 0;
    virtual void SetTextureFilter(std::uint32_t texture,
                                  std::uint8_t filter) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void InvokeCallback(std::uint32_t callback,
                                double value,
                                std::uint32_t context) = 0;
};

class TextureObjectRuntimeStub final : public ITextureObjectRuntime {
public:
    [[nodiscard]] std::uint32_t CreateRaster(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    void DestroyRaster(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreateTexture(
        std::uint32_t) override { return 0u; }
    void SetTextureRaster(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyTexture(
        std::uint32_t) override { return 0u; }
    void SetTextureFilter(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void InvokeCallback(std::uint32_t, double,
                        std::uint32_t) override {}
};

class TextureObject final {
public:
    // IDA 0x5fc0b0.
    [[nodiscard]] static std::uint32_t Destroy(
        TextureObjectState& state, ITextureObjectRuntime& runtime);
    // IDA 0x5fc100.
    [[nodiscard]] static bool Initialize(
        TextureObjectState& state, std::uint32_t context,
        std::uint32_t width, std::uint32_t height,
        ITextureObjectRuntime& runtime);
    // IDA 0x5fc180.
    [[nodiscard]] static std::uint32_t ApplyAndRevertProperties(
        TextureObjectState& state, float value,
        std::uint32_t callback, ITextureObjectRuntime& runtime);
};

}  // namespace pal4
