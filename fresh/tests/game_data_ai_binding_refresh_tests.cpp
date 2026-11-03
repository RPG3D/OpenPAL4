#include "pal4/game_data_ai_binding_refresh.h"

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

struct Runtime final : pal4::IGameDataAiBindingRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t log_handle = 0x4000;
    std::uint32_t release_result = 0x7777;

    Runtime() {
        words[0x1080] = 0x3000;
        words[0x1084] = 0x3008;
        words[0x1018] = 0x2000;
        words[0x101c] = 0x2078;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t BeginLog(std::string_view prefix,
                           std::uint32_t table_name) override {
        Check(prefix == "beginBindField, table:" && table_name == 0x1034,
              "begin log includes exact text and table name");
        calls.emplace_back("begin log");
        return log_handle;
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "effect assignment addresses");
        calls.emplace_back("assign");
    }
    void InitializeConfigStructures(std::uint32_t object) override {
        Check(object == 0x106c, "config initializer address");
        calls.emplace_back("init config");
    }
    void InitializePropertyStructures(std::uint32_t object) override {
        Check(object == 0x106c, "property initializer address");
        calls.emplace_back("init property");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "bind address");
        calls.emplace_back("bind");
    }
    void EndLog(std::uint32_t log, std::string_view prefix,
                std::uint32_t table_name) override {
        Check(log == log_handle && prefix == "endBindField, table:" &&
                  table_name == 0x1034,
              "end log reuses temporary buffer and exact text");
        calls.emplace_back("end log");
    }
    std::uint32_t ReleaseLog(std::uint32_t log) override {
        Check(log == log_handle, "release receives temporary log");
        calls.emplace_back("release log");
        return release_result;
    }
};

void CheckUnchanged() {
    Runtime runtime;
    Check(pal4::GameDataAiBindingRefresh::RefreshConfig(
              0x1000, runtime) == 0 && runtime.calls.empty(),
          "equal non-null counts return zero without binding");
    runtime.words[0x1080] = runtime.words[0x1084] = 0;
    runtime.words[0x1018] = runtime.words[0x101c] = 0;
    Check(pal4::GameDataAiBindingRefresh::RefreshProperty(
              0x1000, runtime) == 0x1014 && runtime.calls.empty(),
          "equal null counts return object plus twenty");
}

void CheckBothBindingRoutes() {
    Runtime config;
    config.words[0x101c] = 0x203c;
    Check(pal4::GameDataAiBindingRefresh::RefreshConfig(
              0x1000, config) == 0x7777 &&
              config.calls == std::vector<std::string>{
                                  "begin log", "assign", "init config",
                                  "bind", "end log", "release log"},
          "AiConfig binds, logs and releases in original order");

    Runtime property;
    property.words[0x101c] = 0x203c;
    Check(pal4::GameDataAiBindingRefresh::RefreshProperty(
              0x1000, property) == 0x7777 &&
              property.calls == std::vector<std::string>{
                                    "begin log", "assign", "init property",
                                    "bind", "end log", "release log"},
          "AiProperty uses its own structure initializer");
}

void CheckNullLog() {
    Runtime runtime;
    runtime.words[0x101c] = 0x203c;
    runtime.log_handle = 0;
    Check(pal4::GameDataAiBindingRefresh::RefreshConfig(
              0x1000, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "begin log", "assign", "init config",
                                   "bind", "end log"},
          "failed log allocation still binds but has no release result");
}

}  // namespace

int main() {
    try {
        CheckUnchanged();
        CheckBothBindingRoutes();
        CheckNullLog();
        std::cout << "game_data_ai_binding_refresh_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
