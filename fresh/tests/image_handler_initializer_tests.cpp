#include "pal4/image_handler_initializer.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
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

class Runtime final : public pal4::IImageHandlerRuntime {
public:
    std::uint32_t AddPath(const std::string_view extension,
                           const std::uint32_t reader,
                           const std::uint32_t writer) override {
        events.emplace_back("add:" + std::string(extension));
        Check(next < 3u && extension == names[next] &&
                  reader == readers[next] && writer == writers[next],
              "extension order and exact binary callback addresses");
        return next++ == fail_at ? 0u : 1u;
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x1234u;
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x1234u &&
                  source_file ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)",
              "logger and original source path");
        logged_format = format;
        logged_line = line;
    }
    void SetTextureReadCallback() override {
        events.emplace_back("texture-read-callback");
    }

    static constexpr std::array<std::string_view, 3> names{
        "bmp", "png", "tga"};
    static constexpr std::array<std::uint32_t, 3> readers{
        0x7be030u, 0x7bf1c0u, 0x5f8a00u};
    static constexpr std::array<std::uint32_t, 3> writers{
        0x7bea60u, 0x7bee10u, 0u};
    std::uint32_t fail_at = 3u;
    std::uint32_t next = 0u;
    std::uint32_t logged_line = 0u;
    std::string logged_format;
    std::vector<std::string> events;
};

void EveryFailurePath() {
    pal4::ImageHandlerInitializer initializer;
    constexpr std::array<std::uint32_t, 3> lines{
        0x2bfu, 0x2c6u, 0x2cdu};
    for (std::uint32_t failure = 0u; failure < 3u; ++failure) {
        Runtime runtime;
        runtime.fail_at = failure;
        Check(initializer.Initialize(runtime) == 0u &&
                  runtime.next == failure + 1u &&
                  runtime.logged_line == lines[failure] &&
                  runtime.events[runtime.events.size() - 2u] ==
                      "logger-init" &&
                  runtime.events.back() == "logger-append",
              "failure stops at corresponding extension and logs once");
        const std::string expected =
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCRegisterImageLoader failed <RegisterImage - " +
            std::string(Runtime::names[failure]) + ">]\n\n\n";
        Check(runtime.logged_format == expected,
              "failure message names exact extension");
    }
}

void AllSuccess() {
    Runtime runtime;
    pal4::ImageHandlerInitializer initializer;
    Check(initializer.Initialize(runtime) == 1u &&
              runtime.next == 3u && runtime.logged_line == 0x2d2u &&
              runtime.logged_format ==
                  "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCRegisterImageLoader success\n\n\n" &&
              runtime.events ==
                  std::vector<std::string>{
                      "add:bmp", "add:png", "add:tga", "logger-init",
                      "logger-append", "texture-read-callback"},
          "success logs before installing texture read callback");
}

}  // namespace

int main() {
    EveryFailurePath();
    AllSuccess();
    std::cout << "4 image handler initialization paths passed\n";
}
