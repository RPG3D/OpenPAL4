#include "pal4/script_object_touch.h"

#include "pal4/player_manager_object.h"

namespace pal4 {
namespace {

std::uint32_t CurrentPlayer(IScriptObjectTouchRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    return manager + index * PlayerManagerObject::kElementBytes;
}

}  // namespace

std::uint32_t ScriptObjectTouch::Run(
    const char* object_name, IScriptObjectTouchRuntime& runtime) {
    auto player = CurrentPlayer(runtime);
    if (player == 0u) return player;

    const auto name = std::string_view(object_name == nullptr ? "" :
                                        object_name);
    const auto object = runtime.FindByName(runtime.ObjectManager(), name);
    if (object == 0u) return object;

    player = CurrentPlayer(runtime);
    if (player == 0u) return player;
    return runtime.Touch(object, player + 0xacu);
}

}  // namespace pal4
