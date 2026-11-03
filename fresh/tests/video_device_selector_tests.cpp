#include "pal4/video_device_selector.h"

#include <algorithm>
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

class Runtime final : public pal4::IVideoDeviceRuntime {
public:
    std::uint32_t GetNumSubSystems() override {
        events.emplace_back("count");
        return subsystem_count;
    }
    void GetSubSystemInfo(const std::uint32_t index,
                          std::array<std::uint32_t, 20>& output) override {
        events.emplace_back("info");
        info_indices.push_back(index);
        Check(output[0] == 0xa5a5a5a5u,
              "subsystem API sees existing 80-byte slot contents");
        output[0] = index + 100u;
        output[19] = index + 200u;
    }
    std::uint32_t GetCurrentSubSystem() override {
        events.emplace_back("current-subsystem");
        return 5u;
    }
    std::uint64_t ReadTimeStampCounter() override {
        events.emplace_back("rdtsc");
        return 0x1122334455667788ull;
    }
    void EnumDisplaySettings(const std::uint32_t mode_number,
                              pal4::DisplayModeRecord& output) override {
        events.emplace_back("enum-display");
        Check(mode_number == 0xffffffffu &&
                  std::all_of(output.Bytes().begin(), output.Bytes().end(),
                              [](const auto byte) { return byte == 0u; }),
              "current display enumeration receives zeroed DEVMODEA");
        output.WriteDword(104u, display_bits);
    }
    std::int32_t ChangeDisplaySettings(
        const pal4::DisplayModeRecord& mode,
        const std::uint32_t flags) override {
        events.emplace_back("change-display");
        ++display_change_count;
        Check(flags == 0x40000000u && mode.ReadDword(36u) == 156u &&
                  mode.ReadDword(40u) == 0x40000u &&
                  mode.ReadDword(104u) == 32u,
              "CDS_RESET requests a 156-byte, DM_BITSPERPEL mode");
        for (std::uint32_t offset = 0u; offset < mode.kBytes; ++offset) {
            if ((offset >= 36u && offset < 38u) ||
                (offset >= 40u && offset < 44u) ||
                (offset >= 104u && offset < 108u)) {
                continue;
            }
            Check(mode.Bytes()[offset] == 0u,
                  "all other requested DEVMODE bytes are zero");
        }
        return display_change_result;
    }
    std::uint32_t GetCurrentVideoMode() override {
        events.emplace_back("current-video-mode");
        return video_mode_calls++ == 0u ? 7u : 9u;
    }
    std::uint32_t SetSubSystem(const std::uint32_t index) override {
        events.emplace_back("set-subsystem");
        Check(index == 5u, "restores observed current subsystem");
        return subsystem_result;
    }
    std::uint32_t SetVideoMode(const std::uint32_t index) override {
        events.emplace_back("set-video-mode");
        Check(index == 7u, "restores first current video mode");
        return video_mode_result;
    }
    void GetVideoModeInfo(const std::uint32_t index,
                           pal4::VideoModeInfo& output) override {
        events.emplace_back("video-info");
        Check(index == 9u, "requeries current video mode after setting");
        output = {1280, 720, mode_depth, mode_flags, 60, 0};
    }
    void SetWindowLong(const std::uint32_t window, const std::int32_t index,
                        const std::uint32_t value) override {
        events.emplace_back("window-long");
        Check(window == 0x1234u && index == -16 &&
                  value == 0x80000000u,
              "fullscreen sets WS_POPUP through GWL_STYLE");
    }
    void SetWindowPos(const std::uint32_t window,
                       const std::uint32_t flags) override {
        events.emplace_back("window-pos");
        Check(window == 0x1234u && flags == 0x27u,
              "fullscreen frame refresh flags");
    }
    void GetClientRect(const std::uint32_t window,
                        pal4::ClientRect& output) override {
        events.emplace_back("client-rect");
        Check(window == 0x1234u, "windowed branch uses original window");
        output = {0, 0, 1024, 768};
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x5678u;
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x5678u &&
                  source_file ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)",
              "logger context and original source path");
        log_format = format;
        log_line = line;
        ++log_count;
    }

    std::uint32_t subsystem_count = 2u;
    std::uint32_t display_bits = 32u;
    std::int32_t display_change_result = 0;
    std::uint32_t subsystem_result = 1u;
    std::uint32_t video_mode_result = 1u;
    std::uint32_t mode_flags = 0u;
    std::int32_t mode_depth = 32;
    std::uint32_t video_mode_calls = 0u;
    std::uint32_t display_change_count = 0u;
    std::uint32_t log_count = 0u;
    std::uint32_t log_line = 0u;
    std::string log_format;
    std::vector<std::uint32_t> info_indices;
    std::vector<std::string> events;
};

struct Fixture {
    pal4::GameConfigWords config{};
    pal4::GameRenderState state{0x9000u};
    Runtime runtime;

    Fixture() {
        config[2] = 32u;
        state.Write(0u, 0x1234u);
        state.Write(8u, 0xa5a5a5a5u);
        for (std::uint32_t offset = 0x30u; offset < 0x530u; offset += 4u) {
            state.Write(offset, 0xa5a5a5a5u);
        }
    }

    std::uint32_t Select() {
        return pal4::VideoDeviceSelector(config, state).Select(runtime);
    }
};

void CheckLog(const Runtime& runtime, const std::string_view info,
              const std::uint32_t line) {
    Check(runtime.log_count == 1u && runtime.log_line == line &&
              runtime.log_format ==
                  "FILE:  [%s]\nLINE:  [%d]\nINFO:  " +
                      std::string(info) + "\n\n\n" &&
              runtime.events[runtime.events.size() - 2u] == "logger-init" &&
              runtime.events.back() == "logger-append",
          "branch log text, source line and call order");
}

void NoSubSystems() {
    Fixture fixture;
    fixture.runtime.subsystem_count = 0u;
    Check(fixture.Select() == 0u && fixture.state.Read(0x530u) == 0u &&
              fixture.runtime.info_indices.empty() &&
              fixture.runtime.events.size() == 3u,
          "no subsystem exits before enumeration");
    CheckLog(fixture.runtime,
             "Error[RWCSelectDevice failed <SubSystems Error>]", 0x1a3u);
}

void SuccessfulWindowedPathAndCap() {
    Fixture fixture;
    fixture.runtime.subsystem_count = 18u;
    fixture.runtime.mode_depth = 33;
    Check(fixture.Select() == 1u &&
              fixture.state.Read(0x530u) == 16u &&
              fixture.runtime.info_indices.size() == 16u &&
              fixture.runtime.info_indices.front() == 0u &&
              fixture.runtime.info_indices.back() == 15u,
          "subsystem count is capped at sixteen");
    for (std::uint32_t index = 0u; index < 16u; ++index) {
        Check(fixture.state.Read(0x30u + index * 0x50u) == index + 100u &&
                  fixture.state.Read(0x30u + index * 0x50u + 76u) ==
                      index + 200u,
              "all subsystem records are stored in 80-byte slots");
    }
    Check(fixture.state.Read(0x534u) == 5u &&
              fixture.state.Read(0x538u) == 7u &&
              fixture.state.Read(0x598u) == 0x11223344u &&
              fixture.state.Read(0x59cu) == 0x55667788u &&
              fixture.state.Read(8u) == 0xa5a5a5a5u &&
              fixture.config[0] == 1024u && fixture.config[1] == 768u &&
              fixture.runtime.display_change_count == 0u,
          "windowed path stores mode, timestamp and client dimensions");
    CheckLog(fixture.runtime, "RWCSelectDevice success", 0x245u);
}

void FullscreenAndDepthProbe() {
    Fixture fixture;
    fixture.runtime.display_bits = 16u;
    fixture.runtime.mode_depth = 32;
    fixture.runtime.mode_flags = 1u;
    Check(fixture.Select() == 1u &&
              fixture.runtime.display_change_count == 1u &&
              fixture.config[0] == 1280u &&
              fixture.config[1] == 720u &&
              fixture.state.Read(8u) == 1u &&
              std::find(fixture.runtime.events.begin(),
                        fixture.runtime.events.end(), "window-long") !=
                  fixture.runtime.events.end() &&
              std::find(fixture.runtime.events.begin(),
                        fixture.runtime.events.end(), "window-pos") !=
                  fixture.runtime.events.end(),
          "mismatched depth probes 32 bits and fullscreen updates window");
    CheckLog(fixture.runtime, "RWCSelectDevice success", 0x245u);
}

void EveryFailureExit() {
    {
        Fixture fixture;
        fixture.runtime.display_bits = 16u;
        fixture.runtime.display_change_result = -1;
        Check(fixture.Select() == 0u && fixture.state.Read(0x538u) == 0u,
              "unsupported depth exits before video mode selection");
        CheckLog(fixture.runtime,
                 "Error[RWCSelectDevice failed <32Bits Not Support>]",
                 0x208u);
    }
    {
        Fixture fixture;
        fixture.runtime.subsystem_result = 0u;
        Check(fixture.Select() == 0u &&
                  fixture.runtime.video_mode_calls == 1u,
              "subsystem failure skips video mode setting");
        CheckLog(fixture.runtime,
                 "Error[RWCSelectDevice failed <RwEngineSetSubSystem>]",
                 0x214u);
    }
    {
        Fixture fixture;
        fixture.runtime.video_mode_result = 0u;
        Check(fixture.Select() == 0u &&
                  fixture.runtime.video_mode_calls == 1u,
              "video mode failure skips mode info and window update");
        CheckLog(fixture.runtime,
                 "Error[RWCSelectDevice failed <RwEngineSetVideoMode>]",
                 0x21du);
    }
}

}  // namespace

int main() {
    NoSubSystems();
    SuccessfulWindowedPathAndCap();
    FullscreenAndDepthProbe();
    EveryFailureExit();
    std::cout << "6 video device selection path tests passed\n";
}
