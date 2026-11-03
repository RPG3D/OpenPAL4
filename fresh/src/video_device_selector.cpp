#include "pal4/video_device_selector.h"

#include <string_view>

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)";
constexpr std::string_view kNoSubSystems =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCSelectDevice failed <SubSystems Error>]\n\n\n";
constexpr std::string_view kUnsupportedDepth =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCSelectDevice failed <32Bits Not Support>]\n\n\n";
constexpr std::string_view kSetSubSystemFailed =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCSelectDevice failed <RwEngineSetSubSystem>]\n\n\n";
constexpr std::string_view kSetVideoModeFailed =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCSelectDevice failed <RwEngineSetVideoMode>]\n\n\n";
constexpr std::string_view kSuccess =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCSelectDevice success\n\n\n";

void Log(IGameConfigLogger& logger, const std::string_view format,
         const std::uint32_t line) {
    logger.AppendLog(logger.InitializeLogger(), format, kSourceFile, line);
}

}  // namespace

std::uint32_t DisplayModeRecord::ReadDword(const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

void DisplayModeRecord::WriteDword(const std::uint32_t offset,
                                    const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

void DisplayModeRecord::WriteWord(const std::uint32_t offset,
                                   const std::uint16_t value) {
    for (std::uint32_t index = 0u; index < 2u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

std::uint32_t VideoDeviceSelector::Select(IVideoDeviceRuntime& runtime) {
    std::uint32_t count = runtime.GetNumSubSystems();
    state_.Write(0x530u, count);
    if (count == 0u) {
        Log(runtime, kNoSubSystems, 0x1a3u);
        return 0u;
    }
    if (count > 16u) {
        count = 16u;
        state_.Write(0x530u, count);
    }
    for (std::uint32_t index = 0u; index < count; ++index) {
        std::array<std::uint32_t, 20> info{};
        for (std::uint32_t word = 0u; word < info.size(); ++word) {
            info[word] = state_.Read(0x30u + index * 0x50u + word * 4u);
        }
        runtime.GetSubSystemInfo(index, info);
        for (std::uint32_t word = 0u; word < info.size(); ++word) {
            state_.Write(0x30u + index * 0x50u + word * 4u, info[word]);
        }
    }
    state_.Write(0x534u, runtime.GetCurrentSubSystem());
    const std::uint64_t timestamp = runtime.ReadTimeStampCounter();
    state_.Write(0x598u, static_cast<std::uint32_t>(timestamp >> 32u));
    state_.Write(0x59cu, static_cast<std::uint32_t>(timestamp));

    DisplayModeRecord current_mode;
    runtime.EnumDisplaySettings(0xffffffffu, current_mode);
    if (current_mode.ReadDword(104u) != config_[2]) {
        DisplayModeRecord requested_mode;
        requested_mode.WriteWord(36u, 156u);
        requested_mode.WriteDword(40u, 0x40000u);
        requested_mode.WriteDword(104u, config_[2]);
        if (runtime.ChangeDisplaySettings(requested_mode, 0x40000000u) !=
            0) {
            Log(runtime, kUnsupportedDepth, 0x208u);
            return 0u;
        }
    }
    state_.Write(0x538u, runtime.GetCurrentVideoMode());
    if (runtime.SetSubSystem(state_.Read(0x534u)) == 0u) {
        Log(runtime, kSetSubSystemFailed, 0x214u);
        return 0u;
    }
    if (runtime.SetVideoMode(state_.Read(0x538u)) == 0u) {
        Log(runtime, kSetVideoModeFailed, 0x21du);
        return 0u;
    }

    VideoModeInfo info{};
    runtime.GetVideoModeInfo(runtime.GetCurrentVideoMode(), info);
    const std::uint32_t window = state_.Read(0u);
    if ((info.flags & 1u) != 0u) {
        config_[0] = static_cast<std::uint32_t>(info.width);
        config_[1] = static_cast<std::uint32_t>(info.height);
        state_.Write(8u, 1u);
        runtime.SetWindowLong(window, -16, 0x80000000u);
        runtime.SetWindowPos(window, 0x27u);
    } else {
        ClientRect rect{};
        runtime.GetClientRect(window, rect);
        config_[0] = static_cast<std::uint32_t>(rect.right);
        config_[1] = static_cast<std::uint32_t>(rect.bottom);
    }
    Log(runtime, kSuccess, 0x245u);
    return 1u;
}

}  // namespace pal4
