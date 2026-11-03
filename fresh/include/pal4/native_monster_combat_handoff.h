#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace pal4 {
class NativePalGameIVTextureStorage;
class NativePalGameIVStateList;
class NativeScriptCombatStart;
class NativeMonsterObjectOwner;

// Scene encounter bridge for434E30. Borrows the actual game/state registry and
// the same native parameter-document writer used by giStartCombat. The caller
// supplies the floor/arena lookup; no guessed combat area is substituted.
class NativeMonsterCombatHandoff final {
public:
    using ResolveArea = std::function<std::string(std::string_view)>;
    NativeMonsterCombatHandoff(NativePalGameIVTextureStorage& game,
        NativePalGameIVStateList& states, NativeScriptCombatStart& request,
        ResolveArea resolve_area);
    ~NativeMonsterCombatHandoff();
    NativeMonsterCombatHandoff(const NativeMonsterCombatHandoff&) = delete;
    NativeMonsterCombatHandoff& operator=(const NativeMonsterCombatHandoff&) = delete;
    std::int32_t Process(NativeMonsterObjectOwner& monster,
        std::int32_t requested_mode, std::string_view arena_name);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
