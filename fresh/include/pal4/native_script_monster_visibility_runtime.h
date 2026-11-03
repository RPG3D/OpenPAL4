#pragma once
#include "pal4/script_monster_visibility.h"
namespace pal4 {
class NativeMonsterManagerOwner;
class NativeArenaNpcSceneEntry;
class IGameResourceManagerRuntime;
// Scene-scoped borrower; original manager owns models/behavior, scene entry owns bindings.
class NativeScriptMonsterVisibilityRuntime final : public IScriptMonsterVisibilityRuntime {
public:
    NativeScriptMonsterVisibilityRuntime(NativeMonsterManagerOwner& monsters,
        NativeArenaNpcSceneEntry& scene, std::uint32_t arena,
        IGameResourceManagerRuntime& transforms)
        : monsters_(monsters), scene_(scene), arena_(arena), transforms_(transforms) {}
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint32_t MonsterManager() override;
    std::uint32_t FindMonster(std::uint32_t manager, std::uint32_t name) override;
    std::uint32_t FindMonsterInfo(std::uint32_t manager, std::uint32_t name) override;
    std::string_view AnimationName(std::uint32_t animation) override;
    std::int32_t CompareIgnoreCase(std::string_view lhs, std::string_view rhs) override;
    std::uint32_t PlayAnimation(std::uint32_t animation, std::string_view name) override;
    void SetStateFlag(std::uint32_t monster, std::uint32_t state) override;
    void ReplaceComponent(std::uint32_t monster, std::uint32_t component, std::uint32_t enabled) override;
    void CleanupBuffers(std::uint32_t monster, std::uint32_t mode) override;
    std::uint32_t ArenaManager() override;
    std::uint32_t EnterCharacter(std::uint32_t arena, std::uint32_t monster) override;
    std::uint32_t LeaveCharacter(std::uint32_t arena, std::uint32_t monster) override;
private:
    NativeMonsterManagerOwner& monsters_;
    NativeArenaNpcSceneEntry& scene_;
    std::uint32_t arena_;
    IGameResourceManagerRuntime& transforms_;
};
}  // namespace pal4
