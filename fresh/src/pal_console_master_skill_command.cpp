#include "pal4/pal_console_master_skill_command.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

std::int32_t Output(IPalConsoleMasterSkillRuntime& runtime,
                    std::string_view format) {
    const auto console = runtime.Console();
    return runtime.AddOutputLine(console, format);
}

}  // namespace

std::int32_t PalConsoleMasterSkillCommand::Execute(
    const char* command, IPalConsoleMasterSkillRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return Output(runtime, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");

    const std::string name(*parser.Parse(command, 1));
    const std::string id_text(*parser.Parse(command, 2));
    const auto manager = runtime.PlayerManager();
    const auto player_id = runtime.PlayerId(manager, name);
    if (player_id == 4)
        return Output(runtime, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");

    const auto second_manager = runtime.PlayerManager();
    const auto player = runtime.PlayerByIndex(second_manager, player_id);
    const auto skill_id = std::atoi(id_text.c_str());
    if (runtime.HasMagic(runtime.GameDatabase() + 0x0cu, skill_id)) {
        const auto magic = runtime.FindMagic(
            runtime.GameDatabase() + 0x0cu, skill_id);
        const auto collection = player + 0x938u;
        const auto current = runtime.CurrentValue(
            collection, magic.category, skill_id);
        runtime.AddDelta(collection, magic.category, skill_id,
                         magic.maximum - current);
        runtime.InsertMagic(collection, magic.category, skill_id);
        (void)Output(runtime, magic.name);
        return Output(runtime, magic.description);
    }
    if (runtime.HasStunt(runtime.GameDatabase() + 0x0cu, skill_id)) {
        const auto stunt = runtime.FindStunt(
            runtime.GameDatabase() + 0x0cu, skill_id);
        runtime.InsertStunt(player + 0x938u, skill_id);
        (void)Output(runtime, stunt.name);
        return Output(runtime, stunt.description);
    }
    return Output(runtime,
                  "\xd5\xd2\xb2\xbb\xb5\xbd\xb6\xd4\xd3\xa6\xb5\xc4\xcf\xc9\xca\xf5\xbb\xf2\xcc\xd8\xbc\xbc");
}

}  // namespace pal4
