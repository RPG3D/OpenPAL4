#include "pal4/screenshot_capture.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScreenshotCaptureRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x7060u, 0x8000u}, {0x800cu, 640u}, {0x8010u, 480u}};
    bool directory_exists = true;
    bool throw_format = false;
    bool throw_write = false;
    std::string path;

    std::uint32_t InitializeCameraSubsystem() override {
        calls.emplace_back("init camera");
        return 0x6000u;
    }
    std::uint32_t ActiveCameraObject(std::uint32_t subsystem) override {
        Check(subsystem == 0x6000u, "camera subsystem argument");
        calls.emplace_back("active camera");
        return 0x7000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t CurrentTime() override {
        calls.emplace_back("time");
        return 123u;
    }
    pal4::ScreenshotTime LocalTime(std::uint32_t time) override {
        Check(time == 123u, "localtime argument");
        calls.emplace_back("localtime");
        return {124, 8, 29, 13, 4, 7};
    }
    std::uint32_t InitializeFilename() override {
        calls.emplace_back("init filename");
        return 0x9000u;
    }
    std::string FormatFilename(std::uint32_t filename, const char* format,
                               std::int32_t year, std::int32_t month,
                               std::int32_t day, std::int32_t hour,
                               std::int32_t minute,
                               std::int32_t second) override {
        Check(filename == 0x9000u, "filename receiver");
        Check(std::string(format) ==
                  "snapshoot\\%d_%d_%d__%d_%d_%d.bmp",
              "screenshot format");
        Check(year == 2024 && month == 9 && day == 29 &&
                  hour == 13 && minute == 4 && second == 7,
              "localtime field conversion");
        calls.emplace_back("format");
        if (throw_format) throw std::runtime_error("format failed");
        path = "snapshoot\\2024_9_29__13_4_7.bmp";
        return path;
    }
    std::string CurrentDirectory(std::uint32_t buffer_size) override {
        Check(buffer_size == 256u, "directory buffer size");
        calls.emplace_back("get cwd");
        return "C:\\game";
    }
    bool SetCurrentDirectory(const std::string& directory) override {
        calls.emplace_back("set cwd " + directory);
        if (directory == "snapshoot") return directory_exists;
        Check(directory == "C:\\game", "restore original directory");
        return true;
    }
    void CreateDirectory(const std::string& directory) override {
        Check(directory == "snapshoot", "create screenshot directory");
        calls.emplace_back("create directory");
    }
    void DeleteFile(const std::string& filename) override {
        Check(filename == path, "delete formatted path");
        calls.emplace_back("delete file");
    }
    std::uint32_t CleanupFilename(std::uint32_t filename) override {
        Check(filename == 0x9000u, "cleanup filename receiver");
        calls.emplace_back("cleanup filename");
        return 0xc000u;
    }
};

struct RenderWare final : pal4::IRenderWareImageApi {
    explicit RenderWare(Runtime& runtime) : runtime(runtime) {}
    Runtime& runtime;
    std::uint32_t created = 0xa000u;
    std::uint32_t allocated = 0xb000u;
    std::uint32_t write_result = 0xb000u;

    std::uint32_t Write(std::uint32_t image, const char* filename) override {
        Check(image == 0xb000u && runtime.path == filename,
              "write allocated image to formatted path");
        runtime.calls.emplace_back("write image");
        if (runtime.throw_write) throw std::runtime_error("write failed");
        return write_result;
    }

    std::uint32_t Create(std::uint32_t width, std::uint32_t height,
                         std::uint32_t depth) override {
        Check(width == 640u && height == 480u && depth == 32u,
              "image dimensions and depth");
        runtime.calls.emplace_back("create image");
        return created;
    }
    std::uint32_t AllocatePixels(std::uint32_t image) override {
        Check(image == 0xa000u, "allocate created image");
        runtime.calls.emplace_back("allocate pixels");
        return allocated;
    }
    std::uint32_t SetFromRaster(std::uint32_t image,
                                std::uint32_t raster) override {
        Check(image == 0xb000u && raster == 0x8000u,
              "copy active raster to allocated image");
        runtime.calls.emplace_back("copy raster");
        return 0xd000u;
    }
    void Destroy(std::uint32_t image) override {
        Check(image == 0xb000u, "destroy allocated image");
        runtime.calls.emplace_back("destroy image");
    }
};

const std::vector<std::string> kPrefix{
    "init camera", "active camera", "read 28768", "read 32784",
    "read 32780", "create image"};

void CheckEarlyExits() {
    Runtime creation_failed;
    RenderWare first(creation_failed);
    first.created = 0u;
    Check(pal4::ScreenshotCapture::Capture(creation_failed, first) == 0u,
          "creation failure result");
    Check(creation_failed.calls == kPrefix, "creation failure stops immediately");

    Runtime allocation_failed;
    RenderWare second(allocation_failed);
    second.allocated = 0u;
    Check(pal4::ScreenshotCapture::Capture(allocation_failed, second) == 0u,
          "allocation failure result");
    auto expected = kPrefix;
    expected.emplace_back("allocate pixels");
    Check(allocation_failed.calls == expected,
          "allocation failure stops without image or filename cleanup");
}

void CheckSuccess(bool directory_exists, bool writer_success = true) {
    Runtime runtime;
    runtime.directory_exists = directory_exists;
    RenderWare renderware(runtime);
    if (!writer_success) renderware.write_result = 0;
    Check(pal4::ScreenshotCapture::Capture(runtime, renderware) == 0xc000u,
          "returns filename cleanup result");
    auto expected = kPrefix;
    expected.insert(expected.end(), {"allocate pixels", "copy raster", "time",
                                     "localtime", "init filename", "format",
                                     "get cwd", "set cwd snapshoot"});
    if (!directory_exists) expected.emplace_back("create directory");
    expected.insert(expected.end(), {"set cwd C:\\game", "delete file",
                                     "write image", "destroy image",
                                     "cleanup filename"});
    Check(runtime.calls == expected, "screenshot call and cleanup order");
}

void CheckExceptions() {
    for (const bool during_write : {false, true}) {
        Runtime runtime;
        runtime.throw_format = !during_write;
        runtime.throw_write = during_write;
        RenderWare renderware(runtime);
        bool caught = false;
        try {
            (void)pal4::ScreenshotCapture::Capture(runtime, renderware);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught, "exception is propagated");
        Check(runtime.calls.back() == "cleanup filename",
              "filename is cleaned on exception");
        Check(runtime.calls.end() ==
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "destroy image"),
              "exception path does not destroy image in original code");
        if (during_write) {
            Check(runtime.calls[runtime.calls.size() - 2] == "write image",
                  "write exception happens before filename cleanup");
        } else {
            Check(runtime.calls[runtime.calls.size() - 2] == "format",
                  "format exception happens before filename cleanup");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckEarlyExits();
        CheckSuccess(true);
        CheckSuccess(false);
        CheckSuccess(true, false);  // Original caller ignores RwImageWrite's result.
        CheckExceptions();
        std::cout << "screenshot_capture_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
