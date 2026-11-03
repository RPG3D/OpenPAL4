#include "pal4/texture_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITextureObjectRuntime {
    std::vector<std::string> events;
    std::vector<std::uint32_t> raster_results{101u, 202u};
    std::size_t raster_index = 0;
    std::uint32_t texture_result = 303u;
    std::map<std::uint32_t, std::uint32_t> words;
    bool fail_callback = false;

    [[nodiscard]] std::uint32_t CreateRaster(
        const std::uint32_t width, const std::uint32_t height,
        const std::uint32_t depth, const std::uint32_t type) override {
        Check(width == 640u && height == 480u && depth == 0u &&
                  (type == 5u || type == 1u),
              "creates both native raster formats at caller dimensions");
        events.emplace_back("create-raster:" + std::to_string(type));
        return raster_results[raster_index++];
    }
    void DestroyRaster(const std::uint32_t raster) override {
        events.emplace_back("destroy-raster:" + std::to_string(raster));
    }
    [[nodiscard]] std::uint32_t CreateTexture(
        const std::uint32_t raster) override {
        Check(raster == 101u, "texture wraps first raster");
        events.emplace_back("create-texture");
        return texture_result;
    }
    void SetTextureRaster(const std::uint32_t texture,
                          const std::uint32_t raster) override {
        Check(raster == 0u, "detaches raster before texture destroy");
        events.emplace_back("detach:" + std::to_string(texture));
    }
    [[nodiscard]] std::uint32_t DestroyTexture(
        const std::uint32_t texture) override {
        events.emplace_back("destroy-texture:" + std::to_string(texture));
        return 55u;
    }
    void SetTextureFilter(const std::uint32_t texture,
                          const std::uint8_t filter) override {
        Check(texture == 303u && filter == 2u,
              "sets texture byte +80 to two");
        events.emplace_back("filter");
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write:" + std::to_string(address));
    }
    void InvokeCallback(const std::uint32_t callback,
                        const double value,
                        const std::uint32_t context) override {
        Check(callback == 0x9000u && context == 0x1000u &&
                  value == static_cast<double>(1.25f) &&
                  words[0x1060u] == 101u &&
                  words[0x1064u] == 202u,
              "callback sees temporary rasters and promoted float");
        events.emplace_back("callback");
        if (fail_callback) throw std::runtime_error("callback failed");
    }
};

}  // namespace

int main() {
    using Texture = pal4::TextureObject;
    Runtime runtime;
    pal4::TextureObjectState state{11u, 22u, 33u, 44u};
    Check(Texture::Destroy(state, runtime) == 55u &&
              state.first_raster == 0u && state.second_raster == 0u &&
              state.context == 0u && state.texture == 0u &&
              runtime.events == std::vector<std::string>({
                  "destroy-raster:11", "destroy-raster:22", "detach:44",
                  "destroy-texture:44"}),
          "destructor releases two rasters before detaching texture");
    runtime.events.clear();
    Check(Texture::Initialize(state, 0x1000u, 640u, 480u, runtime) &&
              state.first_raster == 101u && state.second_raster == 202u &&
              state.context == 0x1000u && state.texture == 303u &&
              runtime.events == std::vector<std::string>({
                  "create-raster:5", "create-raster:1",
                  "create-texture", "filter"}),
          "initializer creates both raster formats and texture in order");

    runtime.events.clear();
    runtime.words[0x1060u] = 777u;
    runtime.words[0x1064u] = 888u;
    Check(Texture::ApplyAndRevertProperties(
              state, 1.25f, 0x9000u, runtime) == 0x1000u &&
              runtime.words[0x1060u] == 777u &&
              runtime.words[0x1064u] == 888u &&
              runtime.events == std::vector<std::string>({
                  "write:4192", "write:4196", "callback",
                  "write:4192", "write:4196"}),
          "callback observes temporary properties and original values return");
    runtime.events.clear();
    runtime.fail_callback = true;
    bool threw = false;
    try {
        (void)Texture::ApplyAndRevertProperties(
            state, 1.25f, 0x9000u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.words[0x1060u] == 777u &&
              runtime.words[0x1064u] == 888u &&
              runtime.events.back() == "write:4196",
          "callback exception restores both context properties");

    Runtime first_failure;
    first_failure.raster_results = {0u};
    pal4::TextureObjectState failed{};
    Check(!Texture::Initialize(failed, 0x1000u, 640u, 480u,
                               first_failure) &&
              failed.context == 0x1000u && failed.first_raster == 0u &&
              first_failure.events == std::vector<std::string>({
                  "create-raster:5"}),
          "first raster failure stops without creating second raster");
    Runtime second_failure;
    second_failure.raster_results = {101u, 0u};
    failed = {};
    Check(!Texture::Initialize(failed, 0x1000u, 640u, 480u,
                               second_failure) &&
              failed.first_raster == 101u && failed.second_raster == 0u &&
              failed.context == 0x1000u &&
              second_failure.events == std::vector<std::string>({
                  "create-raster:5", "create-raster:1",
                  "destroy-raster:101"}),
          "second raster failure destroys first but preserves native field");
    pal4::TextureObjectRuntimeStub stub;
    pal4::TextureObjectState empty{};
    Check(!Texture::Initialize(empty, 0u, 1u, 1u, stub),
          "RenderWare boundary stub links");
    return EXIT_SUCCESS;
}
