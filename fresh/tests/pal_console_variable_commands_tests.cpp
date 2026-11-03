#include "pal4/pal_console_variable_commands.h"

#include <array>
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

constexpr std::array<std::string_view, 47> kNames{
    "maze_var1", "maze_var2", "maze_var3", "maze_var4", "maze_var5",
    "maze_var6", "maze_var7", "maze_var8", "maze_var9", "maze_var10",
    "var_shu", "var_xia", "var_wu", "var_feng", "var_zi", "var_mo",
    "var_ming", "var_yan", "var_cha", "var_star", "npc_a", "npc_b",
    "npc_c", "npc_d", "npc_e", "npc_f", "npc_g", "var_zhu",
    "var_shop031", "var_shop010", "var_shop032", "var_shop021",
    "var_shop012", "var_shop034", "var_shop037", "var_shop004",
    "var_shop024", "var_shop025", "var_shop005", "var_shop035",
    "var_shop026", "var_shop006", "var_shop027", "var_shop007",
    "var_shop029", "var_shop009", "var_shop030"};

struct Runtime final : pal4::IPalConsoleVariableRuntime {
    std::vector<std::string> calls;
    std::uint32_t address = 0u;
    std::uint32_t value = 0u;
    std::string output;
    bool fail_read = false;
    bool fail_write = false;

    std::uint32_t ReadDword(std::uint32_t target) override {
        calls.emplace_back("read");
        address = target;
        if (fail_read) throw std::runtime_error("read failed");
        return static_cast<std::uint32_t>(-17);
    }
    void WriteDword(std::uint32_t target,
                    std::uint32_t data) override {
        calls.emplace_back("write");
        address = target;
        value = data;
        if (fail_write) throw std::runtime_error("write failed");
    }
    void InitializeFormatBuffer() override {
        calls.emplace_back("init buffer");
    }
    std::int32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup buffer");
        return 55;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x1000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x1000u, "variable console address");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

void AllFortySevenMappings() {
    for (std::size_t index = 0; index < kNames.size(); ++index) {
        const std::string name(kNames[index]);
        const auto expected_address =
            0x90dec4u + 4u * static_cast<std::uint32_t>(index);
        Runtime shown;
        Check(pal4::PalConsoleVariableCommands::Show(
                  name.c_str(), shown) == 55 &&
                  shown.address == expected_address &&
                  shown.output == "-17" &&
                  shown.calls == std::vector<std::string>{
                      "init buffer", "read", "console", "output",
                      "cleanup buffer"},
              "show variable maps name to signed global value");
        Runtime changed;
        const auto command = name + " -23";
        Check(pal4::PalConsoleVariableCommands::Set(
                  command.c_str(), changed) == 55 &&
                  changed.address == expected_address &&
                  changed.value == static_cast<std::uint32_t>(-23) &&
                  changed.calls == std::vector<std::string>{
                      "init buffer", "write", "cleanup buffer"},
              "set variable maps name to signed global write");
    }
}

void CaseAndUnknownNames() {
    Runtime upper;
    Check(pal4::PalConsoleVariableCommands::Show("NPC_G", upper) == 55 &&
              upper.address == 0x90df2cu,
          "name comparison ignores ASCII case");
    Runtime unknown_show;
    Check(pal4::PalConsoleVariableCommands::Show(
              "no_such_name", unknown_show) == 55 &&
              unknown_show.calls == std::vector<std::string>{
                  "init buffer", "cleanup buffer"},
          "unknown show name emits no output");
    Runtime unknown_set;
    Check(pal4::PalConsoleVariableCommands::Set(
              "no_such_name 4", unknown_set) == 55 &&
              unknown_set.calls == std::vector<std::string>{
                  "init buffer", "cleanup buffer"},
          "unknown set name performs no write");
    Runtime zero;
    Check(pal4::PalConsoleVariableCommands::Set(
              "maze_var1 text", zero) == 55 && zero.value == 0u,
          "set variable uses atoi zero");
    Runtime empty_name;
    Check(pal4::PalConsoleVariableCommands::Show("", empty_name) == 55 &&
              empty_name.calls == std::vector<std::string>{
                  "init buffer", "cleanup buffer"},
          "empty parsed field is an unknown name");
}

void ErrorsAndCleanup() {
    Runtime missing_show;
    Check(pal4::PalConsoleVariableCommands::Show(
              nullptr, missing_show) == -7 &&
              missing_show.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              missing_show.calls ==
                  std::vector<std::string>{"console", "output"},
          "missing show argument skips format buffer");
    for (const char* input :
         std::array<const char*, 2>{nullptr, "maze_var1"}) {
        Runtime missing;
        Check(pal4::PalConsoleVariableCommands::Set(
                  input, missing) == -7 &&
                  missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  missing.calls ==
                      std::vector<std::string>{"console", "output"},
              "missing set field skips format buffer");
    }
    Runtime failed_read;
    failed_read.fail_read = true;
    try {
        (void)pal4::PalConsoleVariableCommands::Show(
            "maze_var1", failed_read);
        throw std::runtime_error("expected read failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "read failed",
              "read failure propagates");
    }
    Check(failed_read.calls == std::vector<std::string>{
              "init buffer", "read", "cleanup buffer"},
          "show exception cleans format buffer");
    Runtime failed_write;
    failed_write.fail_write = true;
    try {
        (void)pal4::PalConsoleVariableCommands::Set(
            "maze_var1 1", failed_write);
        throw std::runtime_error("expected write failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "write failed",
              "write failure propagates");
    }
    Check(failed_write.calls == std::vector<std::string>{
              "init buffer", "write", "cleanup buffer"},
          "set exception cleans format buffer");
}

}  // namespace

int main() {
    try {
        AllFortySevenMappings();
        CaseAndUnknownNames();
        ErrorsAndCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
