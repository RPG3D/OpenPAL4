#pragma once

#undef ReadTimeStampCounter
#undef EnumDisplaySettings
#undef ChangeDisplaySettings
#undef SetWindowLong

#include "pal4/video_device_selector.h"

namespace pal4 {

class NativeVideoDeviceRuntime : public IVideoDeviceRuntime {
public:
    explicit NativeVideoDeviceRuntime(IGameConfigLogger& logger)
        : logger_(logger) {}

    [[nodiscard]] std::uint32_t GetNumSubSystems() override;
    void GetSubSystemInfo(std::uint32_t index,
                          std::array<std::uint32_t, 20>& output) override;
    [[nodiscard]] std::uint32_t GetCurrentSubSystem() override;
    [[nodiscard]] std::uint64_t ReadTimeStampCounter() override;
    void EnumDisplaySettings(std::uint32_t mode_number,
                             DisplayModeRecord& output) override;
    [[nodiscard]] std::int32_t ChangeDisplaySettings(
        const DisplayModeRecord& mode, std::uint32_t flags) override;
    [[nodiscard]] std::uint32_t GetCurrentVideoMode() override;
    [[nodiscard]] std::uint32_t SetSubSystem(std::uint32_t index) override;
    [[nodiscard]] std::uint32_t SetVideoMode(std::uint32_t index) override;
    void GetVideoModeInfo(std::uint32_t index,
                          VideoModeInfo& output) override;
    void SetWindowLong(std::uint32_t window, std::int32_t index,
                       std::uint32_t value) override;
    void SetWindowPos(std::uint32_t window, std::uint32_t flags) override;
    void GetClientRect(std::uint32_t window, ClientRect& output) override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void AppendLog(std::uint32_t logger, std::string_view format,
                   std::string_view source_file,
                   std::uint32_t source_line) override;

private:
    IGameConfigLogger& logger_;
};

}  // namespace pal4
