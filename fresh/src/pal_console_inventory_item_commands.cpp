#include "pal4/pal_console_inventory_item_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

std::uint32_t Error(IPalConsoleInventoryItemRuntime& runtime,
                    std::string_view message) {
    const auto console = runtime.Console();
    return runtime.AddOutputLine(console, message);
}

constexpr char kInvalidArguments[] =
    "\xb4\xed\xce\xf3\xb2\xce\xca\xfd";
constexpr char kInvalidItemId[] =
    "\xb5\xc0\xbe\xdf\x49\x44\xb4\xed\xce\xf3";

}  // namespace

std::uint32_t PalConsoleInventoryItemCommands::AddProperty(
    const char* command, IPalConsoleInventoryItemRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return Error(runtime, kInvalidArguments);
    const std::string id_text(*parser.Parse(command, 1));
    const std::string quantity_text(*parser.Parse(command, 2));
    const auto item_id = std::atoi(id_text.c_str());
    const auto quantity = std::atoi(quantity_text.c_str());
    const auto database = runtime.GameDatabase();
    if (!runtime.HasProperty(database, item_id))
        return Error(runtime, kInvalidItemId);
    const auto inventory = runtime.Inventory();
    return runtime.ProcessProperty(inventory, item_id, quantity);
}

std::uint32_t PalConsoleInventoryItemCommands::AddEquipment(
    const char* command, IPalConsoleInventoryItemRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1))
        return Error(runtime, kInvalidArguments);
    const std::string id_text(*parser.Parse(command, 1));
    const auto item_id = std::atoi(id_text.c_str());
    const auto database = runtime.GameDatabase();
    if (!runtime.HasEquipment(database + 0x58u, item_id))
        return Error(runtime, kInvalidItemId);
    const auto inventory = runtime.Inventory();
    return runtime.ProcessEquipment(inventory + 0x7cu, item_id);
}

}  // namespace pal4
