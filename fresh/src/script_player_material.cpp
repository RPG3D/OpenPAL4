#include "pal4/script_player_material.h"

namespace pal4 {

std::uint32_t ScriptPlayerMaterial::ResolveName(
    std::uint32_t name_object, IScriptPlayerMaterialRuntime& runtime) {
    const auto name = runtime.ReadWord(name_object + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

std::uint32_t ScriptPlayerMaterial::CurrentPlayer(
    IScriptPlayerMaterialRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    return manager + runtime.ReadWord(manager + 0x2c50u) * 0xb14u;
}

std::uint32_t ScriptPlayerMaterial::Set(
    std::int32_t player_index, std::uint32_t name_object,
    IScriptPlayerMaterialRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return player == 0u ? 0u :
        runtime.SetMaterial(player, ResolveName(name_object, runtime));
}

std::uint32_t ScriptPlayerMaterial::Apply(
    std::int32_t player_index, IScriptPlayerMaterialRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return player == 0u ? 0u : runtime.ApplyMaterial(player);
}

std::uint32_t ScriptPlayerMaterial::SetCurrent(
    std::uint32_t name_object, IScriptPlayerMaterialRuntime& runtime) {
    const auto player = CurrentPlayer(runtime);
    return player == 0u ? 0u :
        runtime.SetMaterial(player, ResolveName(name_object, runtime));
}

std::uint32_t ScriptPlayerMaterial::ApplyCurrent(
    IScriptPlayerMaterialRuntime& runtime) {
    const auto player = CurrentPlayer(runtime);
    return player == 0u ? 0u : runtime.ApplyMaterial(player);
}

std::uint32_t ScriptPlayerMaterial::SetLingshaLegs(
    std::uint32_t name_object, IScriptPlayerMaterialRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), 1);
    return player == 0u ? 0u :
        runtime.SetLegMaterial(player, ResolveName(name_object, runtime));
}

std::uint32_t ScriptPlayerMaterial::ApplyLingshaLegs(
    IScriptPlayerMaterialRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), 1);
    return player == 0u ? 0u : runtime.ApplyLegMaterial(player);
}

}  // namespace pal4
