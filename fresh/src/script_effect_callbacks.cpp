#include "pal4/script_effect_callbacks.h"

#include "pal4/player_manager_object.h"

#include <bit>

namespace pal4 {
namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptEffectRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

}  // namespace

std::uint32_t ScriptEffectCallbacks::PlayCallback(
    std::int32_t key, IScriptEffectRuntime& runtime) {
    auto adjusted = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(key) + 1u);
    if (adjusted > 16) adjusted = 15;
    return runtime.HandleScriptCallback(runtime.EffectManager(), adjusted);
}

std::uint32_t ScriptEffectCallbacks::StopCallback(
    IScriptEffectRuntime& runtime) {
    return runtime.CleanupScriptCallback(runtime.EffectManager());
}

std::uint32_t ScriptEffectCallbacks::Create(
    std::uint32_t name_payload, std::uint32_t argument,
    float x, float y, float z, IScriptEffectRuntime& runtime) {
    const auto name = ResolveName(name_payload, runtime);
    auto handle = runtime.CreateAtCoordinates(
        runtime.EffectManager(), name, {x, y, z}, argument, 0u);
    return runtime.ResetHandle(handle);
}

std::uint32_t ScriptEffectCallbacks::CreateOnObject(
    std::uint32_t object, std::uint32_t name_payload,
    std::uint32_t argument, IScriptEffectRuntime& runtime) {
    if (object == 0u) return 0u;
    const auto name = ResolveName(name_payload, runtime);
    auto handle = runtime.CreateAtAddress(
        runtime.EffectManager(), name, object + 0xacu,
        argument, 0u);
    return runtime.ResetHandle(handle);
}

std::uint32_t ScriptEffectCallbacks::CreateOnPlayer(
    std::uint32_t name_payload, std::uint32_t argument,
    std::int32_t player_index, IScriptEffectRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return CreateOnObject(player, name_payload, argument, runtime);
}

std::uint32_t ScriptEffectCallbacks::CreateOnCurrentPlayer(
    std::uint32_t name_payload, std::uint32_t argument,
    IScriptEffectRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * PlayerManagerObject::kElementBytes;
    return CreateOnObject(player, name_payload, argument, runtime);
}

std::uint32_t ScriptEffectCallbacks::CreateOnNpc(
    std::uint32_t name_payload, std::uint32_t argument,
    std::uint32_t npc_name_payload, IScriptEffectRuntime& runtime) {
    const auto npc_name = ResolveName(npc_name_payload, runtime);
    const auto npc = runtime.FindNpc(runtime.NpcManager(), npc_name);
    return CreateOnObject(npc, name_payload, argument, runtime);
}

}  // namespace pal4
