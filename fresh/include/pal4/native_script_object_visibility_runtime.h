#pragma once

#include "pal4/script_object_visibility.h"
#include "pal4/script_gob_reset.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class NativeSceneGobAssets;
class NativeSceneMovementAsset;
class NativeMinimapManagerOwner;
class NativeCameraManagerOwner;

// Binds giSetObjectVisible to one loaded arena and its GOB owner.
class NativeScriptObjectVisibilityRuntime final
    : public IScriptObjectVisibilityRuntime, public IScriptGobResetRuntime {
public:
    NativeScriptObjectVisibilityRuntime(
        NativeSceneGobAssets& objects, NativeSceneMovementAsset& movement,
        NativeMinimapManagerOwner& minimap, std::uint32_t arena,
        NativeCameraManagerOwner* cameras = nullptr)
        : objects_(objects), movement_(movement), minimap_(minimap),
          arena_(arena), cameras_(cameras) {}

    [[nodiscard]] std::uint32_t ObjectManager() override;
    [[nodiscard]] std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] std::uint32_t FindObject(std::uint32_t manager, std::string_view name) override {
        return FindByName(manager,name);
    }
    [[nodiscard]] std::uint32_t InvokeReset(std::uint32_t object) override;
    [[nodiscard]] std::uint32_t ArenaManager() override;
    void RemoveAnimatedObject(std::uint32_t arena,
                              std::uint32_t object) override;
    void SetObjectState(std::uint32_t object,
                        std::uint32_t state) override;
    void AddAnimatedObject(std::uint32_t arena,
                           std::uint32_t object) override;
    [[nodiscard]] std::uint32_t MinimapManager() override;
    void RefreshDialog(std::uint32_t minimap) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t LogVisibility(
        std::uint32_t source_line, std::string_view name,
        std::uint32_t arena_word8, std::uint32_t arena_word_e0,
        std::uint8_t visible) override;

private:
    NativeSceneGobAssets& objects_;
    NativeSceneMovementAsset& movement_;
    NativeMinimapManagerOwner& minimap_;
    std::uint32_t arena_;
    NativeCameraManagerOwner* cameras_;
};

}  // namespace pal4
