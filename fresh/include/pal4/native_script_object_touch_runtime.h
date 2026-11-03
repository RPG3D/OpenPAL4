#pragma once

#include "pal4/script_object_touch.h"

#include <functional>

namespace pal4 {
class NativePlayerManagerSession;
class NativeSceneGobAssets;
class NativeBaseObjectMemory;

// giGOMTouch borrows the same object dispatch as actual ActorControl interaction.
// Script calls bypass proximity/selection checks, retaining the object's +0x50 behavior.
class NativeScriptObjectTouchRuntime final : public IScriptObjectTouchRuntime {
public:
    using Dispatch = std::function<void(std::uint32_t, std::uint32_t)>;
    NativeScriptObjectTouchRuntime(NativePlayerManagerSession& players,
        NativeSceneGobAssets& objects, NativeBaseObjectMemory& memory,
        Dispatch dispatch);
    std::uint32_t PlayerManager() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ObjectManager() override;
    std::uint32_t FindByName(std::uint32_t manager, std::string_view name) override;
    std::uint32_t Touch(std::uint32_t object, std::uint32_t direction) override;
private:
    NativePlayerManagerSession& players_;
    NativeSceneGobAssets& objects_;
    NativeBaseObjectMemory& memory_;
    Dispatch dispatch_;
};
}
