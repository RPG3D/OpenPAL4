#include "pal4/game_data_parameter_binding_refresh.h"

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

struct Runtime final : pal4::IGameDataParameterBindingRefreshRuntime {
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
              "begin log text and table name");
        calls.emplace_back("begin log");
        return log_handle;
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "effect assignment offsets");
        calls.emplace_back("assign");
    }
    void InitializeUpgradeStructures(std::uint32_t object) override {
        Check(object == 0x106c, "UpgradeParam initializer address");
        calls.emplace_back("init upgrade");
    }
    void InitializeDropStructures(std::uint32_t object) override {
        Check(object == 0x106c, "DropProperty initializer address");
        calls.emplace_back("init drop");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "bind object address");
        calls.emplace_back("bind");
    }
    void EndLog(std::uint32_t log, std::string_view prefix,
                std::uint32_t table_name) override {
        Check(log == log_handle && prefix == "endBindField, table:" &&
                  table_name == 0x1034,
              "end log reuses buffer and table name");
        calls.emplace_back("end log");
    }
    std::uint32_t ReleaseLog(std::uint32_t log) override {
        Check(log == log_handle, "release receives temporary log");
        calls.emplace_back("release log");
        return release_result;
    }
};

void CheckEqualCounts() {
    Runtime upgrade;
    Check(pal4::GameDataParameterBindingRefresh::RefreshUpgrade(
              0x1000, upgrade) == 0 && upgrade.calls.empty(),
          "equal non-null UpgradeParam counts need no bind");
    Runtime drop;
    drop.words[0x1080] = drop.words[0x1084] = 0;
    drop.words[0x1018] = drop.words[0x101c] = 0;
    Check(pal4::GameDataParameterBindingRefresh::RefreshDrop(
              0x1000, drop) == 0x1014 && drop.calls.empty(),
          "equal null DropProperty counts return field-object address");
}

void CheckBindingRoutes() {
    Runtime upgrade;
    upgrade.words[0x101c] = 0x203c;
    Check(pal4::GameDataParameterBindingRefresh::RefreshUpgrade(
              0x1000, upgrade) == 0x7777 &&
              upgrade.calls == std::vector<std::string>{
                                   "begin log", "assign", "init upgrade",
                                   "bind", "end log", "release log"},
          "UpgradeParam rebinds through its structure initializer");

    Runtime drop;
    drop.words[0x101c] = 0x203c;
    Check(pal4::GameDataParameterBindingRefresh::RefreshDrop(
              0x1000, drop) == 0x7777 &&
              drop.calls == std::vector<std::string>{
                                "begin log", "assign", "init drop",
                                "bind", "end log", "release log"},
          "DropProperty rebinds through its own initializer");
}

void CheckNullLog() {
    Runtime runtime;
    runtime.words[0x101c] = 0x203c;
    runtime.log_handle = 0;
    Check(pal4::GameDataParameterBindingRefresh::RefreshUpgrade(
              0x1000, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "begin log", "assign", "init upgrade",
                                   "bind", "end log"},
          "null log allocation still binds without release");
}

}  // namespace

int main() {
    try {
        CheckEqualCounts();
        CheckBindingRoutes();
        CheckNullLog();
        std::cout << "game_data_parameter_binding_refresh_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
