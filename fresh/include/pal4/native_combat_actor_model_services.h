#pragma once
#include <cstdint>
#include <stdexcept>
#include <string_view>
namespace pal4 {
class NativeCombatActorHealthRuntime;
class INativeSkillSystemServices;
// Host cursor services for the actual Monster manager's stage list. Tokens
// are host handles, not pointers to emulated VC6 vectors/iterator objects.
class INativeCombatMonsterRoster {
public:
    virtual ~INativeCombatMonsterRoster()=default;
    virtual std::uint32_t CreateStageIterator()=0;
    virtual bool StageIteratorAtEnd(std::uint32_t)=0;
    virtual std::uint32_t StageIteratorActor(std::uint32_t)=0;
    virtual void NextStageIterator(std::uint32_t)=0;
    virtual void DeleteStageIterator(std::uint32_t)=0;
    virtual void UpdateMonsterMotion(std::uint32_t actor,std::uint32_t flag)=0;
    virtual std::string_view MonsterAnimationName(std::uint32_t child_animation)=0;
};
// Borrowed model/position service for an already constructed native actor.
// The owning renderer and model resources outlive the synchronous call.
class INativeCombatActorModelServices {
public:
    virtual ~INativeCombatActorModelServices() = default;
    [[nodiscard]] virtual std::uint32_t Address() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t RefreshResource() = 0;
    virtual void BeginMotion(NativeCombatActorHealthRuntime&,INativeSkillSystemServices&,INativeCombatMonsterRoster*) {
        throw std::logic_error("Combat model BeginMotion capability is not bound");
    }
    virtual void UpdateMotion(std::uint32_t flag,NativeCombatActorHealthRuntime&,
                              INativeSkillSystemServices&,INativeCombatMonsterRoster* = nullptr) = 0;
};
}
