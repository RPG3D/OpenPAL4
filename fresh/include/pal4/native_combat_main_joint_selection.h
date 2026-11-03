#pragma once
#include <cstdint>
#include <memory>
#include <span>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatAttackActions;
class NativeCombatTaskQueue;
class NativeScriptEventRouter;
class ICombatConsoleResultRuntime;
struct NativeCombatSystemImage;
//54CCB0/54CD50/54CE40 over real Main, actor/skill, CheckProtect and queue owners.
// Does not subscribe separately: the enclosing Main frame owns all19 events.
// Borrowers outlive this consumer; drain the queue before closing its domains.
class NativeCombatMainJointSelection final {
public:
    NativeCombatMainJointSelection(NativeCombatMainWindow&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const>,
        NativeCombatSystemImage&,NativeCombatAttackActions&,NativeCombatTaskQueue&,NativeScriptEventRouter&);
    ~NativeCombatMainJointSelection();
    void Handle(std::uint32_t payload);
    bool Click(std::uint32_t event);
    std::uint8_t ConfirmVisible();
    ICombatConsoleResultRuntime& ResultRuntime();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
