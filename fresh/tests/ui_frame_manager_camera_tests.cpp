#include "pal4/ui_frame_manager_singleton.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerCameraRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t active_camera = 0x7000u;

    std::uint32_t InitializeCameraSubsystem() override {
        events.emplace_back("initialize camera");
        return 0x6000u;
    }
    std::uint32_t ActiveCameraObject(std::uint32_t subsystem) override {
        Check(subsystem == 0x6000u, "active-camera receiver");
        events.emplace_back("active camera");
        return active_camera;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x5148u, "cached image field");
        events.emplace_back("write image");
        words[address] = value;
    }
};

struct RenderWare final : pal4::IRenderWareImageApi {
    std::uint32_t Write(std::uint32_t, const char*) override {
        throw std::runtime_error("camera copy must not write an image file");
    }
    explicit RenderWare(Runtime& runtime) : runtime(runtime) {}
    Runtime& runtime;
    std::uint32_t next_image = 0xa000u;

    std::uint32_t Create(std::uint32_t width,
                         std::uint32_t height,
                         std::uint32_t depth) override {
        Check(width == 640u && height == 480u && depth == 32u,
              "RenderWare image dimensions and depth");
        runtime.events.emplace_back("create");
        runtime.words[next_image + 4u] = width;
        runtime.words[next_image + 8u] = height;
        return next_image;
    }
    void Destroy(std::uint32_t image) override {
        Check(image == 0xb000u, "destroy cached image");
        runtime.events.emplace_back("destroy");
    }
    std::uint32_t AllocatePixels(std::uint32_t image) override {
        Check(image == 0xa000u, "pixels allocated for new image");
        runtime.events.emplace_back("allocate pixels");
        return image;
    }
    std::uint32_t SetFromRaster(std::uint32_t image,
                                std::uint32_t raster) override {
        Check((image == 0xa000u || image == 0xb000u) &&
                  raster == 0x8000u,
              "image/raster copy arguments");
        runtime.events.emplace_back("set raster");
        return 0xc000u;
    }
};

void Configure(Runtime& runtime) {
    runtime.words[0x7060u] = 0x8000u;
    runtime.words[0x800cu] = 640u;
    runtime.words[0x8010u] = 480u;
}

void CheckMissingInputs() {
    Runtime no_camera;
    no_camera.active_camera = 0u;
    RenderWare rw(no_camera);
    Check(pal4::UiFrameManagerSingleton::CopyActiveCameraRaster(
              0x5000u, no_camera, rw) == 0u &&
              no_camera.events == std::vector<std::string>{
                  "initialize camera", "active camera"},
          "no active camera stops before raster access");

    Runtime no_raster;
    RenderWare rw2(no_raster);
    Check(pal4::UiFrameManagerSingleton::CopyActiveCameraRaster(
              0x5000u, no_raster, rw2) == no_raster.active_camera &&
              no_raster.events == std::vector<std::string>{
                  "initialize camera", "active camera", "read 28768"},
          "null raster returns the active camera without image access");
}

void CheckCreateReuseAndResize() {
    Runtime created;
    Configure(created);
    RenderWare rw(created);
    Check(pal4::UiFrameManagerSingleton::CopyActiveCameraRaster(
              0x5000u, created, rw) == 0xc000u &&
              created.words[0x5148u] == 0xa000u,
          "missing image created and copied");
    Check(created.events == std::vector<std::string>{
              "initialize camera", "active camera", "read 28768",
              "read 20808", "read 32784", "read 32780", "create",
              "write image", "allocate pixels", "read 20808",
              "read 32780", "read 40964", "read 40968",
              "read 32784", "read 20808", "set raster"},
          "initial creation follows assembly read and call order");

    Runtime reused;
    Configure(reused);
    reused.words[0x5148u] = 0xb000u;
    reused.words[0xb004u] = 640u;
    reused.words[0xb008u] = 480u;
    RenderWare rw2(reused);
    Check(pal4::UiFrameManagerSingleton::CopyActiveCameraRaster(
              0x5000u, reused, rw2) == 0xc000u &&
              reused.words[0x5148u] == 0xb000u,
          "matching image reused");
    Check(reused.events.end() ==
              std::find(reused.events.begin(), reused.events.end(), "create") &&
              reused.events.end() ==
                  std::find(reused.events.begin(), reused.events.end(), "destroy"),
          "matching dimensions do not recreate image");

    for (const auto wrong_field : {0xb004u, 0xb008u}) {
        Runtime resized;
        Configure(resized);
        resized.words[0x5148u] = 0xb000u;
        resized.words[0xb004u] = 640u;
        resized.words[0xb008u] = 480u;
        resized.words[wrong_field] = 1u;
        RenderWare rw3(resized);
        Check(pal4::UiFrameManagerSingleton::CopyActiveCameraRaster(
                  0x5000u, resized, rw3) == 0xc000u &&
                  resized.words[0x5148u] == 0xa000u,
              "dimension mismatch recreates cached image");
        auto destroyed = std::find(resized.events.begin(),
                                   resized.events.end(), "destroy");
        auto recreated = std::find(resized.events.begin(),
                                   resized.events.end(), "create");
        auto pixels = std::find(resized.events.begin(),
                                resized.events.end(), "allocate pixels");
        auto copied = std::find(resized.events.begin(),
                                resized.events.end(), "set raster");
        Check(destroyed < recreated && recreated < pixels &&
                  pixels < copied, "resize destruction and copy order");
    }
}

}  // namespace

int main() {
    try {
        CheckMissingInputs();
        CheckCreateReuseAndResize();
        std::cout << "ui_frame_manager_camera_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
