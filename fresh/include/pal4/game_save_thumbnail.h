#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

struct GameSavePixelBuffer {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride = 0;
    std::span<std::uint8_t> pixels;
};

class IGameSaveThumbnailRuntime {
public:
    virtual ~IGameSaveThumbnailRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PackedFileManager() = 0;
    virtual void ProcessPackedFile3(std::uint32_t manager) = 0;
    virtual void ProcessPackedFile4(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t CreateImage(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth) = 0;
    virtual void AllocateImagePixels(std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual GameSavePixelBuffer FrameBuffer(
        std::uint32_t framebuffer_slot) = 0;
    [[nodiscard]] virtual GameSavePixelBuffer ImageBuffer(
        std::uint32_t image) = 0;
    virtual void DeleteFile(std::string_view path) = 0;
    virtual void SaveImage(std::uint32_t image,
                           std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t TextureDictionary() = 0;
    virtual void RemoveTextureByCrc32(std::uint32_t dictionary,
                                       std::string_view path) = 0;
    virtual void DestroyImage(std::uint32_t image) = 0;
};

class GameSaveThumbnail final {
public:
    static constexpr std::uint32_t kWidth = 98u;
    static constexpr std::uint32_t kHeight = 75u;
    static constexpr std::uint32_t kDepth = 32u;
    static constexpr std::uint32_t kFrameBufferOffset = 0x148u;

    // IDA 0x5cc5a0.
    [[nodiscard]] static std::uint8_t Capture(
        std::int32_t slot, IGameSaveThumbnailRuntime& runtime);
};

}  // namespace pal4
