#include "pal4/rw_texture_image_factory.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IRwTextureImageFactoryRuntime {
public:
    std::vector<char> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t image = 0x3000u;
    std::uint32_t new_raster = 0x4000u;
    std::uint32_t texture = 0x5000u;
    std::uint32_t format = 0;
    bool use_alpha = false;
    float scale = 0;
    std::uint32_t levels = 1;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back('R');
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back('B');
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back('W');
        words[address] = value;
    }
    std::uint32_t CreateImage(std::uint32_t width,
                              std::uint32_t height,
                              std::uint32_t depth) override {
        calls.push_back('I');
        Check(width == 64u && height == 32u && depth == 32u,
              "image uses raster dimensions and 32-bit depth");
        return image;
    }
    void AllocatePixels(std::uint32_t value) override {
        calls.push_back('A');
        Check(value == image, "allocates temporary image pixels");
    }
    void SetImageFromRaster(std::uint32_t destination,
                            std::uint32_t source) override {
        calls.push_back('S');
        Check(destination == image && source == 0x2000u,
              "reads pixels from source raster");
    }
    std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t output_format) override {
        calls.push_back('N');
        Check(width == 64u && height == 32u && depth == 32u,
              "raster builder uses temporary image dimensions");
        format = output_format;
        return new_raster;
    }
    std::uint32_t RasterLevelCount(std::uint32_t raster) override {
        calls.push_back('L');
        Check(raster == new_raster, "queries generated raster levels");
        return levels;
    }
    void LockRaster(std::uint32_t raster,
                    std::uint32_t level,
                    std::uint32_t mode) override {
        calls.push_back('K');
        Check(raster == new_raster && level == 0u && mode == 5u,
              "locks first generated raster level");
    }
    std::uint32_t ResampleImage(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override {
        calls.push_back('E');
        return 0;
    }
    std::uint32_t CreateNormalMapImage(
        std::uint32_t value, bool alpha,
        float output_scale) override {
        calls.push_back('G');
        Check(value == image, "first level uses original temporary image");
        use_alpha = alpha;
        scale = output_scale;
        return 0x6000u;
    }
    void SetRasterFromImage(std::uint32_t raster,
                            std::uint32_t source) override {
        calls.push_back('U');
        Check(raster == new_raster && source == 0x6000u,
              "writes generated normal map into raster");
    }
    void UnlockRaster(std::uint32_t raster) override {
        calls.push_back('Q');
        Check(raster == new_raster, "unlocks generated raster");
    }
    std::uint32_t CreateTexture(std::uint32_t raster) override {
        calls.push_back('T');
        Check(raster == new_raster,
              "creates texture from generated raster");
        return texture;
    }
    void DestroyImage(std::uint32_t value) override {
        calls.push_back('D');
        Check(value == image || value == 0x6000u,
              "destroys temporary and normal-map images");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0u, 0.5f, runtime) == 0u && runtime.calls.empty(),
          "null texture stops before memory access");
    runtime.words[0x1000u] = 0;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 0.5f, runtime) == 0u &&
              runtime.calls == std::vector<char>{'R'},
          "missing source raster stops before image creation");

    runtime.calls.clear();
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x200cu] = 64u;
    runtime.words[0x2010u] = 32u;
    runtime.words[0x1050u] = 0xaabb3307u;
    runtime.words[0x5050u] = 0x11224455u;
    runtime.words[0x3004u] = 64u;
    runtime.words[0x3008u] = 32u;
    runtime.words[0x400cu] = 64u;
    runtime.words[0x4010u] = 32u;
    runtime.bytes[0x2020u] = 0xabu;
    runtime.bytes[0x2023u] = 0x80u;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 0.5f, runtime) == 0x5000u &&
              runtime.format == 0x8603u && runtime.use_alpha &&
              runtime.scale == 0.5f &&
              runtime.words[0x5050u] == 0x11223307u &&
              runtime.calls == std::vector<char>{
                  'R', 'R', 'R', 'I', 'A', 'S', 'R', 'B', 'B',
                  'R', 'R', 'N', 'R', 'R', 'L', 'K', 'R', 'R',
                  'G', 'U', 'D', 'Q', 'T', 'R', 'W', 'D'},
          "successful conversion preserves high target flags and source low flags");

    runtime.calls.clear();
    runtime.words[0x1050u] = 0x00003002u;
    runtime.bytes[0x2023u] = 0;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 1.0f, runtime) == 0x5000u &&
              runtime.use_alpha && runtime.format == 0x0603u,
          "second alpha-flag branch and format without high raster bit");
    runtime.words[0x1050u] = 0x00001002u;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 1.0f, runtime) == 0x5000u &&
              !runtime.use_alpha,
          "non-alpha source avoids alpha raster mode");

    runtime.calls.clear();
    runtime.image = 0;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 1.0f, runtime) == 0u &&
              runtime.calls == std::vector<char>{'R', 'R', 'R', 'I'},
          "failed temporary image stops conversion safely");
    runtime.image = 0x3000u;
    runtime.calls.clear();
    runtime.new_raster = 0;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 1.0f, runtime) == 0u &&
              runtime.calls.back() == 'D' &&
              runtime.calls[runtime.calls.size() - 2] == 'N',
          "failed raster still releases temporary image");
    runtime.new_raster = 0x4000u;
    runtime.calls.clear();
    runtime.texture = 0;
    Check(pal4::RwTextureImageFactory::CreateFromTexture(
              0x1000u, 1.0f, runtime) == 0u &&
              runtime.calls.back() == 'D' &&
              runtime.calls[runtime.calls.size() - 2] == 'T',
          "failed texture creation releases temporary image");

    std::cout << "rw_texture_image_factory_tests passed\n";
}
