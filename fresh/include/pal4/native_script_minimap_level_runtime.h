#pragma once
#include "pal4/script_minimap_level.h"

namespace pal4 {
class NativeMinimapManagerOwner;
class NativePlayerManagerSession;

class NativeScriptMinimapLevelRuntime final : public IScriptMinimapLevelRuntime {
public:
    void Bind(NativeMinimapManagerOwner& minimap, NativePlayerManagerSession& players) noexcept {
        minimap_ = &minimap; players_ = &players;
    }
    std::uint32_t MinimapManager() override;
    void SetTexture(std::uint32_t minimap, std::uint32_t level) override;
    std::uint32_t PlayerManager() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void UpdateGrid(std::uint32_t minimap, std::uint32_t position, std::uint32_t rotation) override;
private:
    NativeMinimapManagerOwner* minimap_{};
    NativePlayerManagerSession* players_{};
};
}  // namespace pal4
