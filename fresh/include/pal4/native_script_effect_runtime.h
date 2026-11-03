#pragma once

#include "pal4/script_effect_callbacks.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectCreation;
class IScriptPlayerPositionRuntime;
class NativeArenaEffectManagerOwner;

// Borrows one initialized arena effect group and its creation service. The
// optional actor service is required only by player/NPC script entrypoints.
// Script text/position pointers are consumed synchronously, never retained.
class NativeScriptEffectRuntime final : public IScriptEffectRuntime {
public:
    NativeScriptEffectRuntime(NativeBaseObjectMemory&, NativeEffectCreation&,
        IScriptPlayerPositionRuntime* actors = nullptr);
    void BindActors(IScriptPlayerPositionRuntime* actors) noexcept { actors_=actors; }
    void BindScreenEffects(NativeArenaEffectManagerOwner* manager) noexcept { screen_effects_=manager; }
    std::uint32_t EffectManager() override;
    std::uint32_t HandleScriptCallback(std::uint32_t, std::int32_t) override;
    std::uint32_t CleanupScriptCallback(std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override;
    std::uint32_t NpcManager() override;
    std::uint32_t FindNpc(std::uint32_t, std::uint32_t name) override;
    ScriptEffectHandle CreateAtCoordinates(std::uint32_t, std::uint32_t name,
        const std::array<float, 3>&, std::uint32_t repeats, std::uint32_t flag) override;
    ScriptEffectHandle CreateAtAddress(std::uint32_t, std::uint32_t name,
        std::uint32_t position, std::uint32_t repeats, std::uint32_t flag) override;
    std::uint32_t ResetHandle(ScriptEffectHandle&) override;
private:
    IScriptPlayerPositionRuntime& Actors();
    NativeBaseObjectMemory& memory_;
    std::uint32_t group_;
    NativeEffectCreation& creation_;
    IScriptPlayerPositionRuntime* actors_;
    NativeArenaEffectManagerOwner* screen_effects_{};
};
}  // namespace pal4
