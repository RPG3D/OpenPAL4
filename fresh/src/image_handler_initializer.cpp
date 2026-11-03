#include "pal4/image_handler_initializer.h"

#include <array>
#include <string>

namespace pal4 {
namespace {

struct Handler {
    std::string_view extension;
    std::uint32_t reader;
    std::uint32_t writer;
    std::uint32_t failure_line;
};

constexpr std::array<Handler, 3> kHandlers{{
    {"bmp", 0x7be030u, 0x7bea60u, 0x2bfu},
    {"png", 0x7bf1c0u, 0x7bee10u, 0x2c6u},
    {"tga", 0x5f8a00u, 0u, 0x2cdu},
}};
constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)";
constexpr std::string_view kSuccessFormat =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCRegisterImageLoader success\n\n\n";

}  // namespace

std::uint32_t ImageHandlerInitializer::Initialize(
    IImageHandlerRuntime& runtime) const {
    for (const auto& handler : kHandlers) {
        if (runtime.AddPath(handler.extension, handler.reader,
                            handler.writer) != 0u) {
            continue;
        }
        std::string format =
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCRegisterImageLoader failed <RegisterImage - ";
        format += handler.extension;
        format += ">]\n\n\n";
        runtime.AppendLog(runtime.InitializeLogger(), format, kSourceFile,
                          handler.failure_line);
        return 0u;
    }
    runtime.AppendLog(runtime.InitializeLogger(), kSuccessFormat, kSourceFile,
                      0x2d2u);
    runtime.SetTextureReadCallback();
    return 1u;
}

}  // namespace pal4
