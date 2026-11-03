#include "pal4/rw_raster_copy.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IRwRasterCopyRuntime {
public:
    std::array<std::uint8_t, 48> destination{};
    std::vector<std::string> calls;
    bool lock_succeeds = true;

    std::uint32_t TextureRaster(std::uint32_t texture) override {
        Check(texture == 0x55u, "uses texture's raster");
        calls.emplace_back("raster");
        return 0x66u;
    }
    std::int32_t RasterWidth(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster width");
        calls.emplace_back("width");
        return 4;
    }
    std::int32_t RasterHeight(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster height");
        calls.emplace_back("height");
        return 3;
    }
    std::int32_t RasterDepthBits(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster depth");
        calls.emplace_back("depth");
        return 16;
    }
    std::int32_t RasterStride(std::uint32_t raster) override {
        Check(raster == 0x66u, "reads raster stride");
        calls.emplace_back("stride");
        return 12;
    }
    std::uint8_t* LockRaster(std::uint32_t raster, std::int32_t level,
                             std::int32_t mode) override {
        Check(raster == 0x66u && level == 0 && mode == 1,
              "locks mip zero for write");
        calls.emplace_back("lock");
        return lock_succeeds ? destination.data() : nullptr;
    }
    void UnlockRaster(std::uint32_t raster) override {
        Check(raster == 0x66u, "unlocks the raster");
        calls.emplace_back("unlock");
    }
};

void CopyBoundsAndRows() {
    Runtime runtime;
    constexpr std::array<std::uint8_t, 8> source{1, 2, 3, 4, 5, 6, 7, 8};
    Check(pal4::RwRasterCopy::LockAndCopyData(
              0, source.data(), 0, 0, 2, 2, runtime) == 0 &&
              pal4::RwRasterCopy::LockAndCopyData(
                  0x55u, nullptr, 0, 0, 2, 2, runtime) == 0 &&
              runtime.calls.empty(),
          "null texture and pixels return before runtime access");
    Check(pal4::RwRasterCopy::LockAndCopyData(
              0x55u, source.data(), 3, 0, 2, 2, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"raster", "width"},
          "horizontal overflow returns before height or lock");
    runtime.calls.clear();
    Check(pal4::RwRasterCopy::LockAndCopyData(
              0x55u, source.data(), 0, 2, 2, 2, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "raster", "width", "height"},
          "vertical overflow returns before lock");
    runtime.calls.clear();
    runtime.lock_succeeds = false;
    Check(pal4::RwRasterCopy::LockAndCopyData(
              0x55u, source.data(), 1, 1, 2, 2, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "raster", "width", "height", "lock"},
          "lock failure returns without reading pixel metadata");

    runtime.calls.clear();
    runtime.lock_succeeds = true;
    runtime.destination.fill(0xee);
    Check(pal4::RwRasterCopy::LockAndCopyData(
              0x55u, source.data(), 1, 1, 2, 2, runtime) == 0x55u &&
              runtime.calls == std::vector<std::string>{
                  "raster", "width", "height", "lock", "depth", "stride",
                  "unlock"},
          "successful copy locks, copies and returns texture");
    for (std::size_t i = 0; i < source.size(); ++i) {
        const auto destination_offset = (i < 4 ? 14u + i : 26u + i - 4u);
        Check(runtime.destination[destination_offset] == source[i],
              "source rows map to raster stride and x offset");
    }
    Check(runtime.destination[13] == 0xee &&
              runtime.destination[18] == 0xee &&
              runtime.destination[25] == 0xee,
          "copy preserves pixels outside the region");
    runtime.calls.clear();
    Check(pal4::RwRasterCopy::LockAndCopyData(
              0x55u, source.data(), 4, 0, 0, 2, runtime) == 0x55u &&
              runtime.calls.back() == "unlock",
          "zero-width region still locks and unlocks");
}

}  // namespace

int main() {
    CopyBoundsAndRows();
    std::cout << "rw_raster_copy_tests passed\n";
}
