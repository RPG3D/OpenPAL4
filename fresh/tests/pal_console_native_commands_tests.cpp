#include "pal4/pal_console_native_commands.h"

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

struct Runtime final : pal4::IPalConsoleNativeCommandRuntime {
    std::vector<std::string> calls;
    std::int32_t player_id = 2;
    std::array<float, 3> position{};
    std::string player_name;
    std::string output;
    std::uint32_t global_value = 0u;
    std::uint32_t inventory_enabled = 2u;
    std::uint32_t inventory_value = 0u;

    std::uint32_t GameConfigAddress() override {
        calls.emplace_back("config");
        return 0x2000u;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x200cu && value == 1u, "exit byte");
        calls.emplace_back("write");
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        if (address == 0x90dec0u) {
            calls.emplace_back("global");
            global_value = value;
        } else {
            Check(address == 0x6134u, "inventory value address");
            calls.emplace_back("write-inventory");
            inventory_value = value;
        }
    }
    std::uint32_t InventorySubsystem() override {
        calls.emplace_back("inventory-subsystem");
        return 0x6000u;
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x3000u, "ID lookup manager");
        player_name = name;
        calls.emplace_back("id");
        return player_id;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t id) override {
        Check(manager == 0x3000u && id == player_id,
              "player lookup operands");
        calls.emplace_back("player");
        return 0x4000u;
    }
    std::uint8_t UpdateInventoryState(std::uint32_t player,
                                      std::uint32_t enabled) override {
        Check(player == 0x4000u && enabled <= 1u,
              "inventory-state operands");
        calls.emplace_back("inventory");
        inventory_enabled = enabled;
        return 0x83u;
    }
    std::int32_t SetPlayerPosition(
        std::uint32_t player, const std::array<float, 3>& value,
        std::uint32_t flags) override {
        Check(player == 0x4000u && flags == 0u,
              "position virtual call operands");
        calls.emplace_back("position");
        position = value;
        return 79;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x5000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x5000u, "output console");
        calls.emplace_back("output");
        output = format;
        return -19;
    }
};

void ParserMatchesIda() {
    pal4::PalConsoleArgumentParser parser;
    Check(!parser.Parse(nullptr, 1) && !parser.Parse("a b", 0) &&
              !parser.Parse("a b", -1),
          "null and nonpositive index");
    Check(parser.Parse("name  12", 1) == "name" &&
              parser.Parse("name  12", 2) == "" &&
              parser.Parse("name  12", 3) == "12" &&
              !parser.Parse("name  12", 4),
          "consecutive spaces retain empty fields");
    Check(parser.Parse("one", 1) == "one" &&
              !parser.Parse("one", 2) &&
              parser.Parse("", 1) == "" &&
              parser.Parse("name ", 2) == "",
          "sentinel supplies the last delimiter");
    Check(parser.Parse(" a", 1) == "" &&
              parser.Parse(" a", 2) == "a",
          "leading space yields empty field");
}

void ExitSetsConfigFlag() {
    Runtime runtime;
    Check(pal4::PalConsoleNativeCommands::Exit(runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>{"config", "write"},
          "exit writes offset 12 and returns config pointer");
}

void PositionCallsPlayerVirtual() {
    Runtime runtime;
    Check(pal4::PalConsoleNativeCommands::SetPlayerPosition(
              "hero -13 0 25 trailing", runtime) == 79 &&
              runtime.player_name == "hero" &&
              runtime.position == std::array<float, 3>{-13.0f, 0.0f, 25.0f} &&
              runtime.calls == std::vector<std::string>{
                  "manager", "id", "manager", "player", "position"},
          "player ID and virtual move order");

    Runtime nonnumeric;
    Check(pal4::PalConsoleNativeCommands::SetPlayerPosition(
              "hero x 2 3", nonnumeric) == 79 &&
              nonnumeric.position[0] == 0.0f,
          "atoi converts a nonnumeric coordinate to zero");
}

void ErrorsUseNativeMessage() {
    for (const char* command :
         std::array<const char*, 4>{nullptr, "hero", "hero 1",
                                    "hero 1 2"}) {
        Runtime runtime;
        Check(pal4::PalConsoleNativeCommands::SetPlayerPosition(
                  command, runtime) == -19 &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"} &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
              "missing argument prints original GBK error");
    }
    Runtime unknown;
    unknown.player_id = 4;
    Check(pal4::PalConsoleNativeCommands::SetPlayerPosition(
              "unknown 1 2 3", unknown) == -19 &&
              unknown.calls == std::vector<std::string>{
                  "manager", "id", "console", "output"},
          "unknown player skips second manager access and move");
}

void GlobalVariableUsesFirstField() {
    Runtime runtime;
    Check(pal4::PalConsoleNativeCommands::SetGlobalVariable(
              "-27 ignored", runtime) == -27 &&
              runtime.global_value == static_cast<std::uint32_t>(-27) &&
              runtime.calls == std::vector<std::string>{"global"},
          "signed atoi result written and returned");

    Runtime invalid;
    Check(pal4::PalConsoleNativeCommands::SetGlobalVariable(
              "invalid", invalid) == 0 && invalid.global_value == 0u &&
              invalid.calls == std::vector<std::string>{"global"},
          "nonnumeric field writes zero");

    Runtime missing;
    Check(pal4::PalConsoleNativeCommands::SetGlobalVariable(
              nullptr, missing) == -19 &&
              missing.calls == std::vector<std::string>{
                  "console", "output"} &&
              missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
          "null command prints error without writing global");
}

void PlayerJoinAndLeave() {
    for (const bool join : {true, false}) {
        Runtime runtime;
        const auto result = join
            ? pal4::PalConsoleNativeCommands::PlayerJoin("hero extra", runtime)
            : pal4::PalConsoleNativeCommands::PlayerLeave("hero extra", runtime);
        Check(result == 0x83u &&
                  runtime.inventory_enabled == (join ? 1u : 0u) &&
                  runtime.player_name == "hero" &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "id", "manager", "player", "inventory"},
              "join and leave share lookup but forward opposite flags");

        Runtime missing;
        const auto missing_result = join
            ? pal4::PalConsoleNativeCommands::PlayerJoin(nullptr, missing)
            : pal4::PalConsoleNativeCommands::PlayerLeave(nullptr, missing);
        Check(missing_result == static_cast<std::uint8_t>(-19) &&
                  missing.calls == std::vector<std::string>{
                      "console", "output"},
              "missing name returns low byte of output result");

        Runtime unknown;
        unknown.player_id = 4;
        const auto unknown_result = join
            ? pal4::PalConsoleNativeCommands::PlayerJoin("hero", unknown)
            : pal4::PalConsoleNativeCommands::PlayerLeave("hero", unknown);
        Check(unknown_result == static_cast<std::uint8_t>(-19) &&
                  unknown.calls == std::vector<std::string>{
                      "manager", "id", "console", "output"},
              "unknown name skips inventory state update");
    }
}

void InventoryValueRequiresNonnegativeInt() {
    Runtime runtime;
    Check(pal4::PalConsoleNativeCommands::SetInventoryValue(
              "250 extra", runtime) == 0x6000u &&
              runtime.inventory_value == 250u &&
              runtime.calls == std::vector<std::string>{
                  "inventory-subsystem", "write-inventory"},
          "money is written at inventory +0x134");

    Runtime zero;
    Check(pal4::PalConsoleNativeCommands::SetInventoryValue(
              "nonnumeric", zero) == 0x6000u &&
              zero.inventory_value == 0u,
          "atoi zero accepted");

    for (const char* input :
         std::vector<const char*>{nullptr, "-1"}) {
        Runtime rejected;
        Check(pal4::PalConsoleNativeCommands::SetInventoryValue(
                  input, rejected) == static_cast<std::uint32_t>(-19) &&
                  rejected.calls == std::vector<std::string>{
                      "console", "output"} &&
                  rejected.output ==
                      "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
              "missing or negative money skips inventory access");
    }
}

}  // namespace

int main() {
    try {
        ParserMatchesIda();
        ExitSetsConfigFlag();
        PositionCallsPlayerVirtual();
        ErrorsUseNativeMessage();
        GlobalVariableUsesFirstField();
        PlayerJoinAndLeave();
        InventoryValueRequiresNonnegativeInt();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
