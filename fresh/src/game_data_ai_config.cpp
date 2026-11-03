#include "pal4/game_data_ai_config.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace pal4 {

std::uint32_t GameDataAiConfig::FindCommandType(
    std::uint32_t object, std::uint32_t value,
    IGameDataAiConfigRuntime& runtime) {
    const std::uint32_t owner = runtime.ReadWord(object);
    const std::uint32_t property =
        runtime.GetPropertyString(owner, "AiCommandType");
    if (property == 0) return 0;
    return runtime.FindEntry(property, value);
}

std::uint32_t GameDataAiConfig::MemberAt8(
    std::uint32_t object, IGameDataAiConfigRuntime& runtime) {
    return runtime.ReadWord(object + 8);
}

std::uint8_t GameDataAiConfig::RegisterFactories(
    std::uint32_t object, IGameDataAiConfigRuntime& runtime) {
    const std::uint32_t owner = runtime.ReadWord(object);
    (void)runtime.RegisterFactory(owner, "AiConfig", 0x4cc040);
    return static_cast<std::uint8_t>(
        runtime.RegisterFactory(owner, "AiProperty", 0x4ccb60));
}

std::uint8_t GameDataAiConfig::InitializeFromDatabase(
    std::uint32_t object, IGameDataAiConfigRuntime& runtime) {
    const std::uint32_t owner = runtime.ReadWord(object);
    for (const auto [name, offset] : {
             std::pair<std::string_view, std::uint32_t>{"AiConfig", 4},
             std::pair<std::string_view, std::uint32_t>{"AiProperty", 8}}) {
        const std::uint32_t table = runtime.LookupTable(owner, name);
        runtime.WriteWord(object + offset, table);
        if (table == 0) {
            std::string message =
                "ERROR [ palDBAiManager::verifyDataTable < the table ";
            message += name;
            message += "  doesn't exist>]";
            runtime.LogError(message);
            throw std::runtime_error(message);
        }
    }
    return 1;
}

}  // namespace pal4
