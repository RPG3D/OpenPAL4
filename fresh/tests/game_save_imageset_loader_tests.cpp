#include "pal4/game_save_imageset_loader.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameSaveImagesetRuntime {
    std::vector<std::string> calls;
    std::uint32_t packed_queries = 0;
    std::uint32_t imageset_queries = 0;
    bool present = true;

    void Log(const char* call) { calls.emplace_back(call); }
    std::uint32_t PackedFileManager() override {
        Log("packed manager");
        return 0x1000 + ++packed_queries * 0x100;
    }
    void ProcessPackedFile3(std::uint32_t manager) override {
        Check(manager == 0x1100, "initial packed-file manager");
        Log("process file 3");
    }
    void ProcessPackedFile4(std::uint32_t manager) override {
        Check(manager == 0x1200, "final packed-file manager");
        Log("process file 4");
    }
    std::string ConvertToUtf8(std::string_view text) override {
        Check(text == "save\\-7.png" || text == "GameSaveImage-7" ||
                  text == "full_image", "UTF-8 conversion source");
        calls.emplace_back("convert " + std::string(text));
        return std::string(text);
    }
    std::uint32_t ImagesetManager() override {
        Log("imageset manager");
        return 0x4000 + ++imageset_queries * 0x100;
    }
    bool IsImagesetPresent(std::uint32_t manager,
                           std::string_view name) override {
        Check(manager == 0x4100 && name == "GameSaveImage-7",
              "first manager and existing image name");
        Log("imageset present");
        return present;
    }
    void DestroyImageset(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x4200 && name == "GameSaveImage-7",
              "destroy prior named imageset");
        Log("destroy imageset");
    }
    void CreateImagesetFromImageFile(
        std::uint32_t manager, std::string_view name,
        std::string_view image_file,
        std::string_view resource_group) override {
        Check(manager == (present ? 0x4300 : 0x4200) &&
                  name == "GameSaveImage-7" &&
                  image_file == "save\\-7.png" &&
                  resource_group.empty(),
              "imageset creation name, path and empty resource group");
        Log("create imageset");
    }
    std::uint32_t GetImageset(std::uint32_t manager,
                              std::string_view name) override {
        Check(manager == (present ? 0x4400 : 0x4300) &&
                  name == "GameSaveImage-7",
              "get imageset with separate singleton lookup");
        Log("get imageset");
        return 0x5000;
    }
    void SetAutoScalingEnabled(std::uint32_t imageset,
                               bool enabled) override {
        Check(imageset == 0x5000 && !enabled,
              "thumbnail imageset disables autoscaling");
        Log("disable autoscaling");
    }
    std::uint32_t GetImage(std::uint32_t imageset,
                           std::string_view name) override {
        Check(imageset == 0x5000 && name == "full_image",
              "full image lookup");
        Log("get full image");
        return 0x6000;
    }
};

void TestExistingImageset() {
    Runtime runtime;
    Check(pal4::GameSaveImagesetLoader::Load(-7, runtime) == 0x6000,
          "loader returns CEGUI full-image object");
    Check(runtime.calls == std::vector<std::string>{
                               "packed manager", "process file 3",
                               "convert save\\-7.png",
                               "convert GameSaveImage-7",
                               "imageset manager", "imageset present",
                               "imageset manager", "destroy imageset",
                               "imageset manager", "create imageset",
                               "imageset manager", "get imageset",
                               "disable autoscaling", "convert full_image",
                               "packed manager", "process file 4",
                               "get full image"},
          "existing imageset is destroyed before loading and lookup");
    Check(runtime.imageset_queries == 4 && runtime.packed_queries == 2,
          "independent manager lookups follow IDA call sites");
}

void TestAbsentImageset() {
    Runtime runtime;
    runtime.present = false;
    Check(pal4::GameSaveImagesetLoader::Load(-7, runtime) == 0x6000 &&
              runtime.imageset_queries == 3,
          "absent imageset skips destroy lookup");
    Check(runtime.calls == std::vector<std::string>{
                               "packed manager", "process file 3",
                               "convert save\\-7.png",
                               "convert GameSaveImage-7",
                               "imageset manager", "imageset present",
                               "imageset manager", "create imageset",
                               "imageset manager", "get imageset",
                               "disable autoscaling", "convert full_image",
                               "packed manager", "process file 4",
                               "get full image"},
          "absent imageset still follows create and full-image path");
}

}  // namespace

int main() {
    try {
        TestExistingImageset();
        TestAbsentImageset();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
