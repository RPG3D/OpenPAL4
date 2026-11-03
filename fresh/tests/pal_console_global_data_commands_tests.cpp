#include "pal4/pal_console_global_data_commands.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleGlobalDataRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> output;
    std::uint32_t movie_flags = 0x80000000u;
    std::uint32_t inventory_money = 0u;
    std::uint32_t round_times = 0u;
    bool throw_on_output = false;

    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x2000u;
    }
    void InitializeInventoryGameData(std::uint32_t inventory) override {
        Check(inventory == 0x2000u, "god-mode inventory argument");
        calls.emplace_back("initialize-inventory");
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x4000u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x4018u, "movie bitset address");
        calls.emplace_back("read-movie");
        return movie_flags;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        if (address == 0x2134u) {
            calls.emplace_back("write-money");
            inventory_money = value;
        } else if (address == 0x4018u) {
            calls.emplace_back("write-movie");
            movie_flags = value;
        } else {
            Check(address == 0x4028u, "round-times address");
            calls.emplace_back("write-round");
            round_times = value;
        }
    }
    void ResetDialogManager(std::uint32_t global_data) override {
        Check(global_data == 0x4000u, "reset global-data argument");
        calls.emplace_back("reset");
    }
    void SaveDialogSettings(std::uint32_t global_data) override {
        Check(global_data == 0x4000u, "save global-data argument");
        calls.emplace_back("save");
    }
    void InitializeFormatBuffer() override {
        calls.emplace_back("init-format");
    }
    std::int32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup-format");
        return 63;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x3000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x3000u, "console output argument");
        calls.emplace_back("output");
        output.emplace_back(format);
        if (throw_on_output) throw std::runtime_error("output failed");
        return -11;
    }
};

void GodMode() {
    Runtime runtime;
    Check(pal4::PalConsoleGlobalDataCommands::SetGodMode(runtime) == -11 &&
              runtime.inventory_money == 99999u &&
              runtime.calls == std::vector<std::string>{
                  "inventory", "initialize-inventory", "inventory",
                  "write-money", "console", "output"} &&
              runtime.output == std::vector<std::string>{
                  "\xb5\xc0\xbe\xdf\xa3\xac\xd7\xb0\xb1\xb8\xa3\xac\xbd\xf0\xc7\xae\xcc\xed\xbc\xd3\xb3\xc9\xb9\xa6"},
          "god mode initializes inventory and sets 0x1869f money");
}

void MovieFlags() {
    Runtime runtime;
    Check(pal4::PalConsoleGlobalDataCommands::OpenMovieFlag(runtime) ==
              -11 &&
              runtime.movie_flags == 0xffffffffu &&
              runtime.calls.size() == 67u &&
              runtime.calls[0] == "global" &&
              runtime.calls[65] == "console" &&
              runtime.calls[66] == "output" &&
              runtime.output == std::vector<std::string>{
                  "\xbf\xaa\xb7\xc5\xcb\xf9\xd3\xd0\xb5\xe7\xd3\xb0\xb2\xa5\xb7\xc5\xc8\xa8\xcf\xde"},
          "all 32 bits set and original movie message printed");
    for (std::size_t bit = 0; bit < 32u; ++bit)
        Check(runtime.calls[1u + bit * 2u] == "read-movie" &&
                  runtime.calls[2u + bit * 2u] == "write-movie",
              "movie flags use 32 read-modify-write steps");
}

void ResetOutputsInNativeOrder() {
    Runtime runtime;
    Check(pal4::PalConsoleGlobalDataCommands::ResetGlobalData(runtime) ==
              -11 &&
              runtime.calls == std::vector<std::string>{
                  "global", "reset", "global", "save",
                  "console", "output", "console", "output",
                  "console", "output", "console", "output"} &&
              runtime.output == std::vector<std::string>{
                  "\xd6\xd8\xd6\xc3\x20\xb5\xe7\xd3\xb0\xb2\xa5\xb7\xc5\xc8\xa8\xcf\xde\x2e\x2e\x2e\x20\x4f\x4b",
                  "\xd6\xd8\xd6\xc3\x20\xb0\xef\xd6\xfa\xce\xc4\xb1\xbe\xb2\xe9\xbf\xb4\xbc\xc7\xc2\xbc\x2e\x2e\x2e\x20\x4f\x4b",
                  "\xd6\xd8\xd6\xc3\x20\xd3\xce\xcf\xb7\xcd\xa8\xb9\xd8\xbc\xc7\xc2\xbc\x2e\x2e\x2e\x20\x4f\x4b",
                  "\xd6\xd8\xd6\xc3\x20\xcf\xc9\xbd\xa3\xce\xca\xb4\xf0\xb3\xc9\xbc\xa8\x2e\x2e\x2e\x20\x4f\x4b"},
          "reset and save precede four ordered GBK messages");
}

void RoundTimesAndCleanup() {
    Runtime runtime;
    Check(pal4::PalConsoleGlobalDataCommands::SetRoundTimes(
              "-2 extra", runtime) == 63 &&
              runtime.round_times == static_cast<std::uint32_t>(-2) &&
              runtime.calls == std::vector<std::string>{
                  "global", "write-round", "global", "save",
                  "init-format", "console", "output", "cleanup-format"} &&
              runtime.output == std::vector<std::string>{
                  "\xc9\xe8\xb6\xa8\xd3\xce\xcf\xb7\xcd\xa8\xb9\xd8\x20-2\x20\xd6\xdc\xc4\xbf\x2e\x2e\x2e\x20\x4f\x4b"},
          "round value, save, formatted GBK output and cleanup");

    Runtime missing;
    Check(pal4::PalConsoleGlobalDataCommands::SetRoundTimes(
              nullptr, missing) == -11 &&
              missing.calls == std::vector<std::string>{
                  "console", "output"} &&
              missing.output[0] ==
                  "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
          "missing round argument skips global data");

    Runtime throwing;
    throwing.throw_on_output = true;
    try {
        (void)pal4::PalConsoleGlobalDataCommands::SetRoundTimes(
            "3", throwing);
        throw std::runtime_error("expected output failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "output failed",
              "output exception preserved");
    }
    Check(throwing.calls.back() == "cleanup-format",
          "round formatted buffer released during unwind");
}

}  // namespace

int main() {
    try {
        GodMode();
        MovieFlags();
        ResetOutputsInNativeOrder();
        RoundTimesAndCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
