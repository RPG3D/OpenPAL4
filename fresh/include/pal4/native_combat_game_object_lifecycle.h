#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectGroupLifecycle;
class NativeScreenEffectSession;
// ActorManager's game-object layer260..303. Borrows an already constructed
// EffectSystem base and a native host handler catalog. Retire event handlers
// and all Dialog borrowers before Destroy; dependencies outlive this owner.
class NativeCombatGameObjectLifecycle final {
public:
    NativeCombatGameObjectLifecycle(NativeBaseObjectMemory&,std::uint32_t manager,std::uint32_t combat,
        NativeEffectGroupLifecycle&,NativeScreenEffectSession&);
    ~NativeCombatGameObjectLifecycle();
    bool Initialize(std::uint32_t game,std::uint32_t camera);
    void Deinitialize();
    void Destroy();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
