#include "pal4/game_config_initializer.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameConfigRuntime {
public:
    void GetTimerCaps(std::uint32_t& minimum, std::uint32_t& maximum,
                      const std::uint32_t bytes) override {
        events.emplace_back("timer-caps");
        Check(bytes == 8u, "TIMECAPS byte count");
        minimum = 4u;
        maximum = 20u;
    }
    void BeginTimePeriod(const std::uint32_t period) override {
        events.emplace_back("begin-period");
        Check(period == 4u, "timer uses returned minimum period");
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x1234u;
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t source_line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x1234u && source_line == 0x87u &&
                  source_file ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)" &&
                  format == "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInitializeGlobal success\n\n\n",
              "logger source and message");
    }
    void InitializeDocument(pal4::ConfigDocument& document) override {
        events.emplace_back("document-init");
        document[0] = 0xcafeu;
    }
    void LoadConfig(const std::string_view filename,
                    pal4::ConfigDocument& document) override {
        events.emplace_back("load-config");
        Check(filename == "config.cfg" && document[0] == 0xcafeu,
              "config loader receives initialized document");
    }
    std::uint32_t FindValue(pal4::ConfigDocument& document,
                            const std::string_view key) override {
        events.emplace_back("find:" + std::string(key));
        Check(document[0] == 0xcafeu, "lookups use same document");
        auto found = values.find(std::string(key));
        if (found == values.end()) {
            return 0u;
        }
        return found->second.first;
    }
    std::uint32_t ValueToInt(const std::uint32_t token,
                             std::uint32_t& output) override {
        events.emplace_back("parse:" + std::to_string(token));
        for (const auto& [key, value] : values) {
            if (value.first == token) {
                parse_inputs.emplace_back(key, output);
                output = 0xdeadu;
                return value.second;
            }
        }
        Check(false, "parser token is known");
        return 0u;
    }
    void DestroyDocument(pal4::ConfigDocument& document) override {
        events.emplace_back("document-destroy");
        Check(document[0] == 0xcafeu, "destroy receives same document");
    }

    std::vector<std::string> events;
    std::map<std::string, std::pair<std::uint32_t, std::uint32_t>> values;
    std::vector<std::pair<std::string, std::uint32_t>> parse_inputs;
};

class Logger final : public pal4::IGameConfigLogger {
public:
    std::uint32_t InitializeLogger() override {
        events.emplace_back("init");
        return 0x5678u;
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t line) override {
        events.emplace_back("append");
        Check(logger == 0x5678u &&
                  source_file ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)",
              "input attach logger parameters");
        formats.emplace_back(format);
        lines.push_back(line);
    }
    std::vector<std::string> events;
    std::vector<std::string> formats;
    std::vector<std::uint32_t> lines;
};

void CheckStateDefaults(const pal4::GameRenderState& state) {
    std::array<std::uint32_t, pal4::GameRenderState::kBytes / 4u> expected;
    expected.fill(0xa5a5a5a5u);
    for (const auto offset : {0x24u, 0x28u, 0x2cu, 0x530u, 0x534u,
                              0x538u, 0x53cu, 0x540u, 0x544u, 0x548u,
                              0x594u, 0x598u, 0x59cu, 0x5a0u, 0x5a4u,
                              0x14u, 0x10u, 0x08u, 0x0cu, 0x554u,
                              0x558u, 0x55cu, 0x560u, 0x56cu, 0x570u,
                              0x574u, 0x584u, 0x588u, 0x590u}) {
        expected[offset / 4u] = 0u;
    }
    for (const auto offset : {0x20u, 0x54cu, 0x550u, 0x580u, 0x58cu}) {
        expected[offset / 4u] = 1u;
    }
    expected[4u / 4u] = 0x77u;
    expected[0x18u / 4u] = 4u;
    expected[0x1cu / 4u] = 20u;
    expected[0x564u / 4u] = 0x40a00000u;
    expected[0x568u / 4u] = 0x472fc800u;
    expected[0x578u / 4u] = 0x3dcccccdu;
    expected[0x57cu / 4u] = 0x3f800000u;
    for (std::uint32_t offset = 0u; offset < state.kBytes; offset += 4u) {
        Check(state.Read(offset) == expected[offset / 4u],
              "game-state write set and preserved gaps");
    }
}

void DefaultsAndMissingKeys() {
    pal4::GameConfigWords config;
    config.fill(0xa5a5a5a5u);
    pal4::GameRenderState state(0x9000u);
    for (std::uint32_t offset = 0u; offset < state.kBytes; offset += 4u) {
        state.Write(offset, 0xa5a5a5a5u);
    }
    Runtime runtime;
    pal4::GameConfigInitializer initializer(config, state);
    Check(initializer.Initialize(0x77u, runtime) == 1u,
          "initializer always returns one");
    Check(config == pal4::GameConfigWords{800u, 600u, 32u, 0xa5a5a500u,
                                          0x3faaaaabu, 0x3f000000u,
                                          0u, 0u, 0u, 1u, 0u, 0u},
          "config defaults preserve the upper three bytes at +0x0c");
    CheckStateDefaults(state);
    Check(runtime.events == std::vector<std::string>{
              "timer-caps", "begin-period", "logger-init", "logger-append",
              "document-init", "load-config", "find:width", "find:height",
              "find:fullscreen", "find:widescreen", "find:sync",
              "document-destroy"},
          "default path call order");
    Check(runtime.parse_inputs.empty(), "missing keys never invoke parser");
}

void AllPresentAndReturnedConversions() {
    pal4::GameConfigWords config{};
    pal4::GameRenderState state(0x9000u);
    Runtime runtime;
    runtime.values = {{"width", {1u, 1280u}},
                      {"height", {2u, 720u}},
                      {"fullscreen", {3u, 1u}},
                      {"widescreen", {4u, 1u}},
                      {"sync", {5u, 0u}}};
    pal4::GameConfigInitializer initializer(config, state);
    Check(initializer.Initialize(9u, runtime) == 1u,
          "configured path returns one");
    Check(config[0] == 1280u && config[1] == 720u && config[3] == 0u &&
              state.Read(4u) == 9u && state.Read(8u) == 1u &&
              state.Read(0x0cu) == 1u && state.Read(0x54cu) == 0u,
          "five converted return values reach their observed destinations");
    Check(runtime.parse_inputs ==
              std::vector<std::pair<std::string, std::uint32_t>>{
                  {"width", 800u}, {"height", 600u}, {"fullscreen", 0u},
                  {"widescreen", 0u}, {"sync", 1u}},
          "parser receives current defaults including local widescreen zero");
    Check(runtime.events == std::vector<std::string>{
              "timer-caps", "begin-period", "logger-init", "logger-append",
              "document-init", "load-config", "find:width", "parse:1",
              "find:height", "parse:2", "find:fullscreen", "parse:3",
              "find:widescreen", "parse:4", "find:sync", "parse:5",
              "document-destroy"},
          "configured path call order");
}

void InputDeviceAttachment() {
    pal4::GameConfigWords config;
    config.fill(0xa5a5a5a5u);
    pal4::GameRenderState state(0x9000u);
    pal4::GameConfigInitializer initializer(config, state);
    Logger logger;
    Check(initializer.SetInputDevice(0u, 0x1234u, logger) == 1u &&
              config[7] == 1u && config[8] == 0x1234u &&
              config[10] == 0xa5a5a5a5u,
          "selector zero enables keyboard and stores its callback");
    Check(initializer.SetInputDevice(1u, 0x5678u, logger) == 1u &&
              config[10] == 1u && config[11] == 0x5678u &&
              config[8] == 0x1234u,
          "selector one enables mouse independently");
    const auto preserved = config;
    Check(initializer.SetInputDevice(2u, 0x9999u, logger) == 0u &&
              initializer.SetInputDevice(0xffffffffu, 0x8888u, logger) ==
                  0u &&
              config == preserved,
          "other selectors log UNKNOWN without touching config");
    Check(logger.events == std::vector<std::string>{
              "init", "append", "init", "append",
              "init", "append", "init", "append"},
          "each attach request initializes and appends a log");
    Check(logger.lines == std::vector<std::uint32_t>{
              0x16du, 0x177u, 0x17eu, 0x17eu},
          "attach branch source line numbers");
    const std::string prefix = "FILE:  [%s]\nLINE:  [%d]\nINFO:  ";
    Check(logger.formats ==
              std::vector<std::string>{
                  prefix + "RWCInputDeviceAttach <rwcKEYBOARD>\n\n\n",
                  prefix + "RWCInputDeviceAttach <rwcMOUSE>\n\n\n",
                  prefix + "RWCInputDeviceAttach <UNKNOWN>\n\n\n",
                  prefix + "RWCInputDeviceAttach <UNKNOWN>\n\n\n"},
          "exact input attach log messages");
}

}  // namespace

int main() {
    DefaultsAndMissingKeys();
    AllPresentAndReturnedConversions();
    InputDeviceAttachment();
    std::cout << "3 game config initializer tests passed\n";
}
