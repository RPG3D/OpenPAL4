#include "pal4/pal_console_variable_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>

namespace pal4 {
namespace {

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

bool EqualsIgnoreCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i])))
            return false;
    }
    return true;
}

std::optional<std::uint32_t> Address(std::string_view name) {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (EqualsIgnoreCase(name, kNames[i]))
            return 0x90dec4u + 4u * static_cast<std::uint32_t>(i);
    }
    return std::nullopt;
}

std::int32_t BadArguments(IPalConsoleVariableRuntime& runtime) {
    return runtime.AddOutputLine(
        runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
}

}  // namespace

std::int32_t PalConsoleVariableCommands::Show(
    const char* command, IPalConsoleVariableRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) return BadArguments(runtime);
    const std::string name(*parser.Parse(command, 1));

    runtime.InitializeFormatBuffer();
    try {
        if (const auto address = Address(name)) {
            const auto value = static_cast<std::int32_t>(
                runtime.ReadDword(*address));
            (void)runtime.AddOutputLine(runtime.Console(),
                                        std::to_string(value));
        }
    } catch (...) {
        (void)runtime.CleanupFormatBuffer();
        throw;
    }
    return runtime.CleanupFormatBuffer();
}

std::int32_t PalConsoleVariableCommands::Set(
    const char* command, IPalConsoleVariableRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return BadArguments(runtime);
    const std::string name(*parser.Parse(command, 1));
    const std::string value_text(*parser.Parse(command, 2));
    const auto value = std::atoi(value_text.c_str());

    runtime.InitializeFormatBuffer();
    try {
        if (const auto address = Address(name))
            runtime.WriteDword(*address,
                               static_cast<std::uint32_t>(value));
    } catch (...) {
        (void)runtime.CleanupFormatBuffer();
        throw;
    }
    return runtime.CleanupFormatBuffer();
}

}  // namespace pal4
