#include "pal4/tga_header_loader.h"

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

struct Runtime final : pal4::ITgaHeaderRuntime {
    static constexpr std::uint32_t stream = 0x1000u;
    static constexpr std::uint32_t metadata = 0x2000u;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t image_result{};
    std::uint32_t color_map_result{};
    std::uint32_t image_data_result{1u};
    std::uint32_t allocation{0x3000u};
    std::uint32_t rows{3u};
    std::uint32_t requested_size{};
    std::uint32_t requested_flags{};
    std::uint32_t notified_code{999u};
    std::uint32_t logged_line{};
    std::string logged_message;
    bool has_callback{true};
    bool rewrite_error{};

    Runtime() {
        WriteWord(metadata + 12u, 47u);
        WriteWord(stream + 8u, 5u);
        WriteByte(stream + 12u, 1u);
        WriteByte(stream + 13u, 1u);
        WriteHalf(stream + 26u, 5u);
        WriteHalf(stream + 28u, 3u);
        WriteByte(stream + 30u, 24u);
    }
    void WriteHalf(const std::uint32_t address, const std::uint16_t value) {
        WriteByte(address, static_cast<std::uint8_t>(value));
        WriteByte(address + 1u, static_cast<std::uint8_t>(value >> 8u));
    }
    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return static_cast<std::uint32_t>(ReadByte(address)) |
            (static_cast<std::uint32_t>(ReadByte(address + 1u)) << 8u) |
            (static_cast<std::uint32_t>(ReadByte(address + 2u)) << 16u) |
            (static_cast<std::uint32_t>(ReadByte(address + 3u)) << 24u);
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (unsigned byte = 0u; byte < 4u; ++byte)
            WriteByte(address + byte,
                      static_cast<std::uint8_t>(value >> (byte * 8u)));
    }
    void WriteByte(const std::uint32_t address, const std::uint8_t value) {
        bytes[address] = value;
    }
    [[nodiscard]] std::uint32_t LoadImage(
        const std::uint32_t opened) override {
        Check(opened == stream, "loads the supplied stream");
        events.emplace_back("image");
        return image_result;
    }
    [[nodiscard]] std::uint32_t LoadColorMap(
        const std::uint32_t opened, const std::uint32_t data) override {
        Check(opened == stream && data == metadata,
              "passes the complete metadata to color-map load");
        events.emplace_back("color-map");
        return color_map_result;
    }
    [[nodiscard]] std::uint32_t LoadImageData(
        const std::uint32_t opened, const std::uint32_t color_map_slot,
        const std::uint32_t flags) override {
        Check(opened == stream && color_map_slot == metadata + 4u &&
                  flags == ReadWord(metadata + 12u),
              "passes color-map slot and current flags to image-data load");
        events.emplace_back("image-data");
        return image_data_result;
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t size, const std::uint32_t flags) override {
        requested_size = size;
        requested_flags = flags;
        events.emplace_back("allocate");
        return allocation;
    }
    [[nodiscard]] std::uint32_t LoadRows(
        const std::uint32_t opened, const std::uint32_t pixels,
        const std::uint32_t first_row, const std::uint32_t row_count,
        const std::uint32_t flags) override {
        Check(opened == stream && pixels == allocation && first_row == 0u &&
                  row_count == 3u && flags == ReadWord(metadata + 12u),
              "passes allocation, row range and updated flags to row loader");
        events.emplace_back("rows");
        return rows;
    }
    [[nodiscard]] bool HasErrorCallback(
        const std::uint32_t opened) override {
        Check(opened == stream, "checks callback on the stream");
        events.emplace_back("callback-check");
        return has_callback;
    }
    void NotifyError(const std::uint32_t opened,
                     const std::uint32_t code) override {
        Check(opened == stream, "notifies the supplied stream");
        notified_code = code;
        events.emplace_back("notify");
        if (rewrite_error) WriteWord(stream + 8u, 7u);
    }
    void LogError(const std::string_view file, const std::uint32_t line,
                  const std::string_view message) override {
        Check(file.find("TgaLoader\\tga.c") != std::string_view::npos,
              "logs the original TGA source file");
        logged_line = line;
        logged_message = message;
        events.emplace_back("log");
    }
};

using Events = std::vector<std::string>;

void TestNullAndImageFailure() {
    Runtime null_stream;
    Check(pal4::TgaHeaderLoader::Load(0u, Runtime::metadata, null_stream) == 0u &&
              null_stream.events.empty(), "null stream has no side effects");

    Runtime failed;
    failed.image_result = 1u;
    failed.rewrite_error = true;
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      failed) == 0u,
          "image failure returns zero");
    Check(failed.events == Events({"image", "callback-check", "notify", "log"}),
          "image failure reports and stops");
    Check(failed.notified_code == 5u && failed.logged_line == 45u &&
              failed.logged_message == "Unknown sub-format" &&
              failed.ReadWord(Runtime::stream + 8u) == 7u,
          "callback gets original error, log gets updated byte");
}

void TestColorMapAndFlagGates() {
    Runtime missing_map;
    missing_map.WriteByte(Runtime::stream + 12u, 0u);
    missing_map.WriteWord(Runtime::metadata + 12u, 0x10403u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      missing_map) == 0u &&
              missing_map.ReadWord(Runtime::metadata + 12u) == 0x10402u &&
              missing_map.ReadWord(Runtime::stream + 8u) == 0u &&
              missing_map.events == Events({"image"}),
          "absent color map clears only bit zero and skips pixels");

    Runtime disabled;
    disabled.WriteWord(Runtime::metadata + 12u, 0x10400u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      disabled) == 0u &&
              disabled.ReadWord(Runtime::metadata + 12u) == 0x10400u &&
              disabled.events == Events({"image"}),
          "disabled color and pixels preserve upper flag bits");

    Runtime bad_map;
    bad_map.color_map_result = 1u;
    bad_map.WriteWord(Runtime::metadata + 12u, 0x105u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      bad_map) == 0u &&
              bad_map.ReadWord(Runtime::metadata + 12u) == 0x10cu &&
              bad_map.logged_line == 52u &&
              bad_map.events == Events({"image", "color-map",
                                      "callback-check", "notify", "log",
                                      "image-data", "allocate", "rows"}),
          "color-map failure clears bit zero and continues loading rows");
}

void TestImageDataBranches() {
    Runtime failed;
    failed.image_data_result = 0u;
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      failed) == 0u &&
              failed.ReadWord(Runtime::metadata + 12u) == 39u &&
              failed.ReadWord(Runtime::stream + 8u) == 5u &&
              failed.logged_line == 62u &&
              failed.events == Events({"image", "color-map", "image-data",
                                      "callback-check", "notify", "log"}),
          "image-data failure clears bit three and returns without allocation");

    Runtime good;
    good.WriteWord(Runtime::metadata + 12u, 0x107u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      good) == 0u &&
              good.ReadWord(Runtime::metadata + 12u) == 0x10fu &&
              good.ReadWord(Runtime::metadata + 8u) == good.allocation &&
              good.requested_size == 45u && good.requested_flags == 0u &&
              good.ReadWord(Runtime::stream + 8u) == 0u,
          "success sets bit three and allocates width times height times depth");

    Runtime no_map_image;
    no_map_image.WriteByte(Runtime::stream + 13u, 2u);
    no_map_image.WriteWord(Runtime::metadata + 12u, 0x107u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      no_map_image) == 0u &&
              no_map_image.ReadWord(Runtime::metadata + 12u) == 0x107u &&
              no_map_image.events == Events({"image", "color-map",
                                            "allocate", "rows"}),
          "nonpalette image skips image-data load and bit-three change");
}

void TestAllocationAndRowsFailure() {
    Runtime no_pixels;
    no_pixels.allocation = 0u;
    no_pixels.WriteWord(Runtime::stream + 8u, 6u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      no_pixels) == 0u &&
              no_pixels.ReadWord(Runtime::metadata + 8u) == 0u &&
              no_pixels.ReadWord(Runtime::metadata + 12u) == 43u &&
              no_pixels.notified_code == 2u &&
              no_pixels.logged_line == 72u &&
              no_pixels.logged_message == "Out of memory" &&
              no_pixels.ReadWord(Runtime::stream + 8u) == 2u &&
              no_pixels.events == Events({"image", "color-map", "image-data",
                                          "allocate", "callback-check",
                                          "notify", "log"}),
          "allocation failure clears image bit and stores error after callback");

    Runtime short_rows;
    short_rows.rows = 2u;
    short_rows.has_callback = false;
    short_rows.WriteWord(Runtime::stream + 8u, 5u);
    Check(pal4::TgaHeaderLoader::Load(Runtime::stream, Runtime::metadata,
                                      short_rows) == 0u &&
              short_rows.ReadWord(Runtime::metadata + 12u) == 43u &&
              short_rows.logged_line == 78u &&
              short_rows.logged_message == "Read failed" &&
              short_rows.notified_code == 999u &&
              short_rows.ReadWord(Runtime::stream + 8u) == 0u &&
              short_rows.events == Events({"image", "color-map", "image-data",
                                          "allocate", "rows",
                                          "callback-check", "log"}),
          "short row read logs without callback and clears stream error");
}

}  // namespace

int main() {
    TestNullAndImageFailure();
    TestColorMapAndFlagGates();
    TestImageDataBranches();
    TestAllocationAndRowsFailure();
    return EXIT_SUCCESS;
}
