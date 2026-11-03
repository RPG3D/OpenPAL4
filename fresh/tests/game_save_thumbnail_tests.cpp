#include "pal4/game_save_thumbnail.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameSaveThumbnailRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint8_t> source_pixels;
    std::vector<std::uint8_t> image_pixels;
    std::uint32_t source_width = 0;
    std::uint32_t source_height = 0;
    std::uint32_t source_stride = 0;
    std::uint32_t image_stride = 98u * 4u + 8u;
    std::uint32_t manager_queries = 0;

    Runtime() {
        SetSource(196, 150, 12);
        image_pixels.resize(75u * image_stride);
    }
    void SetSource(std::uint32_t width, std::uint32_t height,
                   std::uint32_t padding) {
        source_width = width;
        source_height = height;
        source_stride = width * 4u + padding;
        source_pixels.assign(height * source_stride, 0);
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const auto offset = y * source_stride + x * 4u;
                source_pixels[offset] = static_cast<std::uint8_t>(x);
                source_pixels[offset + 1u] = static_cast<std::uint8_t>(y);
                source_pixels[offset + 2u] =
                    static_cast<std::uint8_t>(x ^ y);
                source_pixels[offset + 3u] = 0x11u;
            }
        }
    }
    void Log(const char* call) { calls.emplace_back(call); }
    std::uint32_t PackedFileManager() override {
        Log("packed file manager");
        return 0x1000 + ++manager_queries * 0x100;
    }
    void ProcessPackedFile3(std::uint32_t manager) override {
        Check(manager == 0x1100, "first packed-file manager");
        Log("process file 3");
    }
    void ProcessPackedFile4(std::uint32_t manager) override {
        Check(manager == 0x1200, "second packed-file manager");
        Log("process file 4");
    }
    std::uint32_t CreateImage(std::uint32_t width,
                              std::uint32_t height,
                              std::uint32_t depth) override {
        Check(width == 98 && height == 75 && depth == 32,
              "RenderWare image dimensions and depth");
        Log("create image");
        return 0x2000;
    }
    void AllocateImagePixels(std::uint32_t image) override {
        Check(image == 0x2000, "allocate new image pixels");
        Log("allocate pixels");
    }
    std::uint32_t UiFrameManager() override {
        Log("ui frame manager");
        return 0x3000;
    }
    pal4::GameSavePixelBuffer FrameBuffer(
        std::uint32_t slot) override {
        Check(slot == 0x3148, "UI frame buffer at +0x148");
        Log("frame buffer");
        return {source_width, source_height, source_stride, source_pixels};
    }
    pal4::GameSavePixelBuffer ImageBuffer(
        std::uint32_t image) override {
        Check(image == 0x2000, "destination image buffer");
        Log("image buffer");
        return {98, 75, image_stride, image_pixels};
    }
    void DeleteFile(std::string_view path) override {
        Check(path == "save\\-7.png", "signed thumbnail path");
        Log("delete file");
    }
    void SaveImage(std::uint32_t image,
                   std::string_view path) override {
        Check(image == 0x2000 && path == "save\\-7.png",
              "image saved to thumbnail path");
        Log("save image");
    }
    std::uint32_t TextureDictionary() override {
        Log("texture dictionary");
        return 0x4000;
    }
    void RemoveTextureByCrc32(std::uint32_t dictionary,
                               std::string_view path) override {
        Check(dictionary == 0x4000 && path == "save\\-7.png",
              "thumbnail texture invalidation");
        Log("remove texture");
    }
    void DestroyImage(std::uint32_t image) override {
        Check(image == 0x2000, "destroy thumbnail image");
        Log("destroy image");
    }
};

void CheckPixel(const Runtime& runtime, std::uint32_t x,
                std::uint32_t y, std::uint32_t source_x,
                std::uint32_t source_y) {
    const auto destination = y * runtime.image_stride + x * 4u;
    const auto source = source_y * runtime.source_stride + source_x * 4u;
    Check(runtime.image_pixels[destination] ==
                  runtime.source_pixels[source] &&
              runtime.image_pixels[destination + 1u] ==
                  runtime.source_pixels[source + 1u] &&
              runtime.image_pixels[destination + 2u] ==
                  runtime.source_pixels[source + 2u] &&
              runtime.image_pixels[destination + 3u] == 0xffu,
          "nearest-neighbor sample with forced opaque alpha");
}

void TestCapture() {
    Runtime runtime;
    Check(pal4::GameSaveThumbnail::Capture(-7, runtime) == 1,
          "capture returns one");
    Check(runtime.calls == std::vector<std::string>{
                               "packed file manager", "process file 3",
                               "create image", "allocate pixels",
                               "ui frame manager", "frame buffer",
                               "image buffer", "delete file", "save image",
                               "texture dictionary", "remove texture",
                               "destroy image", "packed file manager",
                               "process file 4"},
          "image, disk, texture and packed-file operations retain order");
    Check(runtime.manager_queries == 2,
          "packed-file manager reacquired for final processing");
    CheckPixel(runtime, 0, 0, 0, 0);
    CheckPixel(runtime, 1, 1, 2, 2);
    CheckPixel(runtime, 49, 37, 98, 74);
    CheckPixel(runtime, 97, 74, 194, 148);
    Check(runtime.image_pixels[98u * 4u] == 0 &&
              runtime.image_pixels[runtime.image_stride - 1u] == 0,
          "destination row padding is untouched");
}

void TestSmallFrameRepeatsFirstPixel() {
    Runtime runtime;
    runtime.SetSource(50, 40, 8);
    Check(pal4::GameSaveThumbnail::Capture(-7, runtime) == 1,
          "small source still produces thumbnail");
    CheckPixel(runtime, 0, 0, 0, 0);
    CheckPixel(runtime, 97, 74, 0, 0);
}

}  // namespace

int main() {
    try {
        TestCapture();
        TestSmallFrameRepeatsFirstPixel();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
