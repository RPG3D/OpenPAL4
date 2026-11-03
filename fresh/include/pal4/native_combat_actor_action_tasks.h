#pragma once
#include "pal4/combat_action_task_operations.h"
namespace pal4 {
class NativeCombatActorModelOwner;
class INativeSkillSystemServices;
class NativeCombatPlayerModelCache;
class NativeScriptEventRouter;
class NativeActorInstanceOwner;
// Borrowed ChangeAction / actor564 binding: Monster52B9D0 or Player539820.
// ProcessAction and completion queries reuse the same real playback owners.
class NativeCombatActorActionTasks final : public ICombatActionTaskRuntime {
public:
    NativeCombatActorActionTasks(NativeCombatActorModelOwner&,INativeSkillSystemServices&);
    NativeCombatActorActionTasks(NativeCombatPlayerModelCache&,INativeSkillSystemServices&,NativeScriptEventRouter&);
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    std::string ReadCString(std::uint32_t) override;
    std::uint32_t SkillSystem() override;
    std::uint32_t GlobalSystem() override;
    void PrepareActionServices() override;
    void SetActorTarget(std::uint32_t actor,std::uint32_t text,std::uint8_t mode) override;
    void StartActorAction(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void RestoreRequestedModel(std::uint32_t actor);
    std::string GetAnimationName(std::uint32_t) override;
    std::int32_t GetAnimationFlag(std::uint32_t) override;
    int CompareInsensitive(std::string_view,std::string_view) override;
private:
    class Query;
    std::uint32_t Owner() const;
    void RequireActor(std::uint32_t) const;
    const NativeActorInstanceOwner& Model() const;
    NativeCombatActorModelOwner* model_{};
    NativeCombatPlayerModelCache* player_{};
    NativeScriptEventRouter* router_{};
    INativeSkillSystemServices& system_;
};
}
