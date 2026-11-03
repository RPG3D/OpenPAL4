#include "pal4/tga_image_loader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITgaImageLoadRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::uint32_t> released;
    std::uint32_t stream{0x1000u};
    bool header_failure{};
    bool image_failure{};
    std::uint16_t width{2u};
    std::uint16_t height{2u};

    Runtime() {
        WriteWord(stream + 8u, 0u);
        WriteHalf(stream + 26u, width);
        WriteHalf(stream + 28u, height);
        bytes[stream + 14u] = 2u;
        bytes[stream + 33u] = 1u;
        events.clear();
    }
    void WriteHalf(const std::uint32_t address, const std::uint16_t value) {
        bytes[address] = static_cast<std::uint8_t>(value);
        bytes[address + 1u] = static_cast<std::uint8_t>(value >> 8u);
    }
    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        for (unsigned shift = 0u; shift < 32u; shift += 8u)
            bytes[address + shift / 8u] =
                static_cast<std::uint8_t>(value >> shift);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    [[nodiscard]] std::uint32_t Free(const std::uint32_t address) override {
        released.push_back(address);
        events.emplace_back("free");
        return address + 1u;
    }
    [[nodiscard]] std::uint32_t OpenFile(
        const std::uint32_t source, const std::string_view mode) override {
        Check(source == 0x8000u && mode == "r", "TGA opens source in read mode");
        events.emplace_back("open");
        return stream;
    }
    [[nodiscard]] std::uint32_t AllocateMetadata(
        const std::uint32_t size, const std::uint32_t flags) override {
        Check(size == 16u && flags == 0u,
              "TGA metadata uses 16-byte zero-flags allocation");
        events.emplace_back("allocate-metadata");
        return 0x3000u;
    }
    [[nodiscard]] bool LoadHeader(const std::uint32_t opened,
                                  const std::uint32_t metadata) override {
        Check(opened == stream && metadata == 0x3000u &&
                  words[metadata + 12u] == 47u &&
                  words[metadata] == 0u && words[metadata + 4u] == 0u,
              "header sees initialized metadata and flags 47");
        events.emplace_back("header");
        WriteWord(metadata + 8u, 0x5000u);
        for (std::uint32_t index = 0u; index < 16u; ++index)
            bytes[0x5000u + index] = static_cast<std::uint8_t>(index + 1u);
        return header_failure;
    }
    void CloseFile(const std::uint32_t opened) override {
        Check(opened == stream, "closes the opened TGA stream");
        events.emplace_back("close");
    }
    void NotifyError(const std::uint32_t opened,
                     const std::uint32_t code) override {
        Check(opened == stream && code == 5u,
              "header failure notifies stream callback with error five");
        events.emplace_back("notify");
    }
    [[nodiscard]] std::string_view ErrorMessage(
        const std::uint32_t code) override {
        Check(code == 5u, "looks up error five");
        events.emplace_back("message");
        return "bad header";
    }
    void LogError(const std::string_view file, const std::uint32_t line,
                  const std::string_view message) override {
        Check(file.find("RwTgaLoader.c") != std::string_view::npos &&
                  line == 66u && message == "bad header",
              "header failure logs source location and error message");
        events.emplace_back("log");
    }
    [[nodiscard]] std::uint32_t CreateImage(
        const std::uint16_t image_width,
        const std::uint16_t image_height,
        const std::uint32_t depth) override {
        Check(image_width == width && image_height == height && depth == 32u,
              "RenderWare image uses TGA dimensions and 32-bit depth");
        events.emplace_back("image-create");
        if (image_failure) return 0u;
        WriteWord(0x4000u + 16u, 12u);
        WriteWord(0x4000u + 20u, 0x6000u);
        return 0x4000u;
    }
    void AllocatePixels(const std::uint32_t image) override {
        Check(image == 0x4000u, "RenderWare allocates the created image");
        events.emplace_back("pixels-allocate");
    }
};

void TestOpenAndHeaderFailures() {
    Runtime missing;
    missing.stream = 0u;
    Check(pal4::TgaImageLoader::Load(0x8000u, missing) == 0u &&
              missing.events == (std::vector<std::string>{"open"}),
          "failed open stops without allocation or close");

    Runtime failed_state;
    failed_state.WriteWord(failed_state.stream + 8u, 9u);
    Check(pal4::TgaImageLoader::Load(0x8000u, failed_state) == 0u &&
              failed_state.events == (std::vector<std::string>{"open"}),
          "stream with existing error is returned unchanged");

    Runtime header;
    header.header_failure = true;
    Check(pal4::TgaImageLoader::Load(0x8000u, header) == 0u &&
              header.words[header.stream + 8u] == 5u &&
              header.released ==
                  (std::vector<std::uint32_t>{0x5000u, 0u, 0u, 0x3000u}),
          "failed header closes, frees metadata and stores error five");
    Check(header.events == (std::vector<std::string>{
              "open", "allocate-metadata", "header", "close",
              "free", "free", "free", "free", "notify", "message", "log"}),
          "header failure callback and log follow resource cleanup");
}

void TestPixelOrientationAndUnsupportedFormat() {
    for (const bool top_down : {true, false}) {
        Runtime runtime;
        runtime.bytes[runtime.stream + 33u] = top_down ? 1u : 0u;
        Check(pal4::TgaImageLoader::Load(0x8000u, runtime) == 0x4000u &&
                  runtime.released ==
                      (std::vector<std::uint32_t>{
                          0x5000u, 0u, 0u, 0x3000u}),
              "supported TGA creates image and releases metadata");
        for (std::uint32_t row = 0u; row < 2u; ++row) {
            const auto destination_row = top_down ? row : 1u - row;
            for (std::uint32_t byte = 0u; byte < 8u; ++byte)
                Check(runtime.bytes[0x6000u + destination_row * 12u + byte] ==
                          row * 8u + byte + 1u,
                      "RGBA bytes follow TGA vertical origin and image stride");
        }
        Check(runtime.events[runtime.events.size() - 5u] == "close",
              "pixel copy completes before close and four frees");
    }

    Runtime unsupported;
    unsupported.bytes[unsupported.stream + 14u] = 1u;
    Check(pal4::TgaImageLoader::Load(0x8000u, unsupported) == 0u &&
              unsupported.events.size() == 8u,
          "unsupported image type skips RenderWare and still cleans resources");

    Runtime no_image;
    no_image.image_failure = true;
    Check(pal4::TgaImageLoader::Load(0x8000u, no_image) == 0u &&
              no_image.events.size() == 9u,
          "RenderWare creation stub can fail without leaking TGA metadata");

    pal4::TgaImageLoadRuntimeStub stub;
    Check(pal4::TgaImageLoader::Load(0x8000u, stub) == 0u,
          "linkable RenderWare/file runtime stub fails closed");
}

}  // namespace

int main() {
    TestOpenAndHeaderFailures();
    TestPixelOrientationAndUnsupportedFormat();
}
