#include "pal4/game_config_initializer.h"

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)";
constexpr std::string_view kSuccessFormat =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInitializeGlobal success\n\n\n";

std::uint32_t ReadConfiguredValue(IGameConfigRuntime& runtime,
                                  ConfigDocument& document,
                                  const std::string_view key,
                                  std::uint32_t& current) {
    const std::uint32_t token = runtime.FindValue(document, key);
    if (token != 0u) {
        return runtime.ValueToInt(token, current);
    }
    return current;
}

}  // namespace

std::uint32_t GameConfigInitializer::Initialize(
    const std::uint32_t config_source, IGameConfigRuntime& runtime) {
    config_[0] = 800u;
    config_[1] = 600u;
    config_[2] = 32u;
    config_[3] &= 0xffffff00u;
    config_[4] = 0x3faaaaabu;
    config_[5] = 0x3f000000u;
    config_[6] = 0u;
    config_[8] = 0u;
    config_[7] = 0u;
    config_[9] = 1u;
    config_[11] = 0u;
    config_[10] = 0u;

    state_.Write(0x20u, 1u);
    for (const auto offset : {0x24u, 0x28u, 0x2cu, 0x530u, 0x534u,
                              0x538u, 0x53cu, 0x540u, 0x544u, 0x548u,
                              0x594u, 0x598u, 0x59cu, 0x5a0u, 0x5a4u,
                              0x14u, 0x10u, 0x08u, 0x0cu}) {
        state_.Write(offset, 0u);
    }
    state_.Write(0x54cu, 1u);
    state_.Write(0x550u, 1u);
    for (std::uint32_t offset = 0x554u; offset <= 0x560u; offset += 4u) {
        state_.Write(offset, 0u);
    }
    state_.Write(0x564u, 0x40a00000u);
    state_.Write(0x568u, 0x472fc800u);
    for (std::uint32_t offset = 0x56cu; offset <= 0x574u; offset += 4u) {
        state_.Write(offset, 0u);
    }
    state_.Write(0x578u, 0x3dcccccdu);
    state_.Write(0x57cu, 0x3f800000u);
    state_.Write(0x580u, 1u);
    state_.Write(0x584u, 0u);
    state_.Write(0x588u, 0u);
    state_.Write(0x58cu, 1u);
    state_.Write(0x590u, 0u);

    std::uint32_t minimum_period = state_.Read(0x18u);
    std::uint32_t maximum_period = state_.Read(0x1cu);
    runtime.GetTimerCaps(minimum_period, maximum_period, 8u);
    state_.Write(0x18u, minimum_period);
    state_.Write(0x1cu, maximum_period);
    runtime.BeginTimePeriod(state_.Read(0x18u));

    state_.Write(4u, config_source);
    runtime.AppendLog(runtime.InitializeLogger(), kSuccessFormat,
                      kSourceFile, 0x87u);

    ConfigDocument document{};
    runtime.InitializeDocument(document);
    runtime.LoadConfig("config.cfg", document);

    config_[0] = ReadConfiguredValue(runtime, document, "width", config_[0]);
    config_[1] = ReadConfiguredValue(runtime, document, "height", config_[1]);
    std::uint32_t fullscreen = state_.Read(8u);
    state_.Write(8u, ReadConfiguredValue(runtime, document, "fullscreen",
                                         fullscreen));
    std::uint32_t widescreen = 0u;
    state_.Write(0x0cu, ReadConfiguredValue(runtime, document, "widescreen",
                                            widescreen));
    std::uint32_t sync = state_.Read(0x54cu);
    state_.Write(0x54cu, ReadConfiguredValue(runtime, document, "sync", sync));

    runtime.DestroyDocument(document);
    return 1u;
}

std::uint32_t GameConfigInitializer::SetInputDevice(
    const std::uint32_t selector, const std::uint32_t callback,
    IGameConfigLogger& logger) {
    std::string_view format;
    std::uint32_t line;
    std::uint32_t result = 1u;
    if (selector == 0u) {
        config_[8] = callback;
        config_[7] = 1u;
        format =
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInputDeviceAttach <rwcKEYBOARD>\n\n\n";
        line = 0x16du;
    } else if (selector == 1u) {
        config_[11] = callback;
        config_[10] = 1u;
        format =
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInputDeviceAttach <rwcMOUSE>\n\n\n";
        line = 0x177u;
    } else {
        format =
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInputDeviceAttach <UNKNOWN>\n\n\n";
        line = 0x17eu;
        result = 0u;
    }
    logger.AppendLog(logger.InitializeLogger(), format, kSourceFile, line);
    return result;
}

}  // namespace pal4
