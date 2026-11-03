#include "pal4/script_monster_visibility.h"

namespace pal4 {
namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptMonsterVisibilityRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

void SelectIdleAnimation(std::uint32_t monster,
                         IScriptMonsterVisibilityRuntime& runtime) {
    const auto animation = monster + 0x170u;
    if (runtime.CompareIgnoreCase(runtime.AnimationName(animation), "Z01") != 0 &&
        runtime.CompareIgnoreCase(runtime.AnimationName(animation), "C01") != 0 &&
        runtime.PlayAnimation(animation, "C01") == 0u) {
        (void)runtime.PlayAnimation(animation, "Z01");
    }
}

}  // namespace

std::uint32_t ScriptMonsterVisibility::SetHide(
    std::uint32_t name_payload, std::uint8_t hidden,
    IScriptMonsterVisibilityRuntime& runtime) {
    const auto name = ResolveName(name_payload, runtime);
    const auto monster = runtime.FindMonster(runtime.MonsterManager(), name);
    if (monster == 0u) return 0u;

    if (hidden == 0u) {
        runtime.SetStateFlag(monster, 0u);
        runtime.CleanupBuffers(monster, 1u);
        return runtime.LeaveCharacter(runtime.ArenaManager(), monster);
    }

    SelectIdleAnimation(monster, runtime);
    runtime.SetStateFlag(monster, 1u);
    const auto component = runtime.ReadWord(monster + 0x654u);
    if (component != 0u) runtime.ReplaceComponent(monster, component, 1u);
    return runtime.EnterCharacter(runtime.ArenaManager(), monster);
}

std::uint32_t ScriptMonsterVisibility::SetVisible(
    std::uint32_t name_payload, std::uint8_t visible,
    IScriptMonsterVisibilityRuntime& runtime) {
    const auto name = ResolveName(name_payload, runtime);
    const auto monster = runtime.FindMonster(runtime.MonsterManager(), name);
    if (monster == 0u) return 0u;
    const auto repeated_name = ResolveName(name_payload, runtime);
    const auto info = runtime.FindMonsterInfo(runtime.MonsterManager(),
                                              repeated_name);
    if (visible == 0u)
        return runtime.LeaveCharacter(runtime.ArenaManager(), monster);

    (void)runtime.LeaveCharacter(runtime.ArenaManager(), monster);
    SelectIdleAnimation(monster, runtime);
    runtime.SetStateFlag(monster, 1u);
    if (info != 0u) runtime.ReplaceComponent(monster, info, 1u);
    return runtime.EnterCharacter(runtime.ArenaManager(), monster);
}

}  // namespace pal4
