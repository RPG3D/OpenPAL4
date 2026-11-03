#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/base_object_weapon.h"

namespace pal4 {

class IBaseObjectNamedWeaponRuntime : public IBaseObjectWeaponRuntime {
public:
    virtual void CleanupNamed(BaseObjectState&, BaseObjectHand);
    virtual void AddAndProcessScripts() {}
    virtual void CleanupScripts() {}
    [[nodiscard]] virtual std::string CreatePath(std::uint32_t) { return {}; }
    virtual void CleanupPath(std::string&) {}
    [[nodiscard]] virtual std::uint32_t GameInstance() { return 0u; }
    [[nodiscard]] virtual bool LoadClump(
        BaseObjectState&, std::uint32_t, std::string_view,
        std::string_view, std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t) { return false; }
    [[nodiscard]] virtual std::uint32_t TextureDictionary() { return 0u; }
    [[nodiscard]] virtual std::uint32_t ResolveTexturePath(
        std::uint32_t, std::string_view) { return 0u; }
    virtual void LoadTexture(BaseObjectState&, std::uint32_t,
                             std::string_view, std::uint32_t) {}
    virtual void HideHookAtomics(std::uint32_t) {}
    [[nodiscard]] virtual std::int32_t CompareRoleToLinsha(
        std::uint32_t) { return 1; }
    [[nodiscard]] virtual bool EffectReady(BaseObjectState&,
                                            std::uint32_t) { return false; }
    virtual void DisplayAndCleanupEffect(BaseObjectState&,
                                         std::uint32_t) {}
    [[nodiscard]] virtual BaseObjectEffectHandle LoadXmlEffect(
        std::uint32_t, std::uint32_t, std::uint32_t) { return {}; }
    virtual void CleanupClump(BaseObjectState&, std::uint32_t,
                              std::uint32_t) {}
};

class BaseObjectNamedWeapon final {
public:
    // IDA 0x443050.
    [[nodiscard]] static std::uint32_t SetupRight(
        BaseObjectState& state, std::string_view model,
        std::string_view texture, std::uint32_t directory,
        std::uint32_t effect0, std::uint32_t effect1,
        IBaseObjectNamedWeaponRuntime& runtime);
    // IDA 0x4432f0.
    [[nodiscard]] static std::uint32_t SetupLeft(
        BaseObjectState& state, std::string_view model,
        std::string_view texture, std::uint32_t directory,
        std::uint32_t effect0, std::uint32_t effect1,
        IBaseObjectNamedWeaponRuntime& runtime);
    // IDA 0x4435a0 and 0x443640.
    [[nodiscard]] static std::uint32_t CleanupRight(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupLeft(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
};

}  // namespace pal4
