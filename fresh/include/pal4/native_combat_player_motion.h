#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatPlayerModelCache;
class NativeCombatActorSkillRuntime;
class INativeSkillSystemServices;
class NativeScriptEventRouter;
struct NativeCombatPlayerMotionBinding {
    NativeCombatPlayerModelCache& model;
    NativeCombatActorSkillRuntime& skills;
};
// Actual5395E0, reusing the shared scene-transition algorithm. Borrows the
// real Player stage roster and resolver; all returned services outlive calls.
// Resolver covers every modeled stage member, including synchronous refresh.
class NativeCombatPlayerMotion final {
public:
    using Resolve=std::function<NativeCombatPlayerMotionBinding(std::uint32_t)>;
    NativeCombatPlayerMotion(NativeCombatPlayerManager&,INativeSkillSystemServices&,
                            NativeScriptEventRouter&,Resolve);
    ~NativeCombatPlayerMotion();
    NativeCombatPlayerMotion(const NativeCombatPlayerMotion&)=delete;
    NativeCombatPlayerMotion& operator=(const NativeCombatPlayerMotion&)=delete;
    void Update(std::uint32_t actor,std::uint32_t flags);
    void Begin(std::uint32_t actor);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
