#pragma once

#include "pal4/arena_effect_manager.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectTreeStorage;
class NativeEffectOwnerRegistry;
class NativeEffectGroupLifecycle;
class INativeEffectGroupLifecycleServices;
class IGameManagerResetRuntime;
class NativeScreenEffectSession;

// Owns the base group and trailing script-tree allocations in a borrowed,
// zeroed 304-byte arena manager image. Shared state and all services must
// outlive this object. Destroy while Dialog and script handlers remain alive.
class NativeArenaEffectManagerOwner final {
public:
    NativeArenaEffectManagerOwner(NativeBaseObjectMemory&, std::uint32_t image,
        ArenaEffectSharedState&, NativeEffectTreeStorage&, std::uint32_t singleton,
        NativeEffectOwnerRegistry&, INativeEffectGroupLifecycleServices&,
        IGameManagerResetRuntime&);
    ~NativeArenaEffectManagerOwner();
    NativeArenaEffectManagerOwner(const NativeArenaEffectManagerOwner&) = delete;
    NativeArenaEffectManagerOwner& operator=(const NativeArenaEffectManagerOwner&) = delete;
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] NativeEffectGroupLifecycle& Group();
    // IDA522A00: stop CG callback, drain playback and unregister the base group.
    void ShutdownEffects();
    void BindScreenEffects(NativeScreenEffectSession*);
    std::uint32_t PlayScreenEffect(std::int32_t key);
    std::uint32_t StopScreenEffect();
    void Destroy();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
