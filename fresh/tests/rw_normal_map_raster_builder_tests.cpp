#include "pal4/rw_normal_map_raster_builder.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::RwNormalMapRasterRuntimeStub {
public:
    std::vector<char> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> resample_sizes;
    std::vector<float> normal_scales;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> normal_inputs;
    std::vector<std::uint32_t> locked_levels;
    std::vector<std::uint32_t> resample_results{0x3000u, 0u};
    std::vector<std::uint32_t> normal_results{0x4000u, 0u};
    std::uint32_t raster = 0x2000u;
    std::uint32_t levels = 3u;
    std::uint32_t set_count = 0;
    std::uint32_t unlock_count = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back('R');
        return words[address];
    }
    std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t format) override {
        calls.push_back('C');
        Check(width == words[0x1004u] &&
                  height == words[0x1008u] &&
                  depth == 32u && format == 0x8603u,
              "raster inherits source dimensions, depth and format");
        return raster;
    }
    std::uint32_t RasterLevelCount(std::uint32_t object) override {
        calls.push_back('L');
        Check(object == raster, "queries created raster level count");
        return levels;
    }
    void LockRaster(std::uint32_t object,
                    std::uint32_t level,
                    std::uint32_t mode) override {
        calls.push_back('K');
        Check(object == raster && mode == 5u,
              "locks level with mode five");
        locked_levels.push_back(level);
    }
    std::uint32_t ResampleImage(
        std::uint32_t source, std::uint32_t width,
        std::uint32_t height) override {
        calls.push_back('E');
        Check(source == 0x1000u, "resamples original source image");
        resample_sizes.emplace_back(width, height);
        const auto index = resample_sizes.size() - 1u;
        return index < resample_results.size()
                   ? resample_results[index] : 0u;
    }
    std::uint32_t CreateNormalMapImage(
        std::uint32_t source, bool alpha,
        float scale) override {
        calls.push_back('N');
        Check(alpha, "alpha mode is forwarded at each mip level");
        normal_inputs.push_back(source);
        normal_scales.push_back(scale);
        const auto index = normal_scales.size() - 1u;
        return index < normal_results.size()
                   ? normal_results[index] : 0u;
    }
    void SetRasterFromImage(
        std::uint32_t object,
        std::uint32_t image) override {
        calls.push_back('S');
        Check(object == raster && image == 0x4000u,
              "writes successful normal map into created raster");
        ++set_count;
    }
    void DestroyImage(std::uint32_t image) override {
        calls.push_back('D');
        destroyed.push_back(image);
    }
    void UnlockRaster(std::uint32_t object) override {
        calls.push_back('U');
        Check(object == raster, "unlocks current raster");
        ++unlock_count;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::RwNormalMapRasterBuilder::Create(
              0u, 0x8603u, true, 1.0f, runtime) == 0u &&
              runtime.calls.empty(),
          "null image is rejected before RenderWare access");

    runtime.words[0x1004u] = 8u;
    runtime.words[0x1008u] = 4u;
    runtime.words[0x200cu] = 8u;
    runtime.words[0x2010u] = 4u;
    runtime.raster = 0;
    Check(pal4::RwNormalMapRasterBuilder::Create(
              0x1000u, 0x8603u, true, 1.0f, runtime) == 0u &&
              runtime.calls == std::vector<char>{'R', 'R', 'C'},
          "failed raster allocation returns without locking");

    runtime.calls.clear();
    runtime.raster = 0x2000u;
    runtime.levels = 0;
    Check(pal4::RwNormalMapRasterBuilder::Create(
              0x1000u, 0x8603u, true, 1.0f, runtime) == 0x2000u &&
              runtime.calls == std::vector<char>{
                  'R', 'R', 'C', 'R', 'R', 'L'},
          "zero-level raster returns without image generation");

    runtime.calls.clear();
    runtime.levels = 3;
    Check(pal4::RwNormalMapRasterBuilder::Create(
              0x1000u, 0x8603u, true, 1.0f, runtime) == 0x2000u &&
              runtime.locked_levels == std::vector<std::uint32_t>{0, 1, 2} &&
              runtime.unlock_count == 3u &&
              runtime.resample_sizes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {4u, 2u}, {2u, 1u}} &&
              runtime.normal_inputs ==
                  std::vector<std::uint32_t>{0x1000u, 0x3000u} &&
              runtime.normal_scales == std::vector<float>{1.0f, 0.5f} &&
              runtime.set_count == 1u &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x4000u, 0x3000u},
          "mip loop handles direct, resampled and failed resample paths");
    Check(runtime.calls == std::vector<char>{
              'R', 'R', 'C', 'R', 'R', 'L',
              'K', 'R', 'R', 'N', 'S', 'D', 'U',
              'K', 'R', 'E', 'N', 'D', 'U',
              'K', 'R', 'E', 'U'},
          "raster operations follow per-level native order");

    Runtime narrow;
    narrow.words[0x1004u] = 2u;
    narrow.words[0x1008u] = 1u;
    narrow.words[0x200cu] = 2u;
    narrow.words[0x2010u] = 1u;
    narrow.levels = 4u;
    narrow.resample_results = {0u, 0u, 0u};
    narrow.normal_results = {0u};
    Check(pal4::RwNormalMapRasterBuilder::Create(
              0x1000u, 0x8603u, true, 8.0f, narrow) == 0x2000u &&
              narrow.resample_sizes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {1u, 1u}, {1u, 1u}, {1u, 1u}} &&
              narrow.normal_scales == std::vector<float>{8.0f} &&
              narrow.unlock_count == 4u && narrow.set_count == 0u,
          "one-pixel dimensions remain clamped and every lock is released");

    std::cout << "rw_normal_map_raster_builder_tests passed\n";
}
