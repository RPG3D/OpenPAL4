#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

enum class BaseObjectHand { Right, Left };

struct BaseObjectEffectHandle {
    std::uint32_t first{};
    std::uint32_t second{};
};

struct BaseObjectEffectCreation {
    BaseObjectEffectHandle persistent{};
    BaseObjectEffectHandle temporary{};
};

class IBaseObjectWeaponRuntime {
public:
    virtual ~IBaseObjectWeaponRuntime() = default;
    virtual void CleanupWeapon(BaseObjectState&, BaseObjectHand);
    [[nodiscard]] virtual std::uint32_t GetUserData(
        std::uint32_t, std::string_view) { return 0u; }
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t) { return 0u; }
    // IDA442BC2/4431A9: destination receives the inverse of the hook matrix.
    virtual void InvertTransform(std::uint32_t, std::uint32_t) {}
    virtual void ScaleFrame(std::uint32_t, std::uint32_t,
                            std::uint32_t) {}
    virtual void UpdateFrame(std::uint32_t) {}
    virtual void RotateMatrix(std::uint32_t, std::uint32_t,
                              float, std::uint32_t) {}
    virtual void ProcessUserData(std::uint32_t, std::string_view,
                                 std::uint32_t) {}
    [[nodiscard]] virtual bool EffectNeedsUpdate(BaseObjectState&,
                                                  std::uint32_t) { return false; }
    virtual void UpdateEffect(BaseObjectState&, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t EffectSingleton() { return 0u; }
    [[nodiscard]] virtual BaseObjectEffectCreation CreateEffect(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) { return {}; }
    virtual void ResetTemporary(BaseObjectEffectHandle&) {}
    virtual void SetEffectProperties(BaseObjectState&,
                                     std::uint32_t, std::uint32_t) {}
    virtual void SetEffectVisibility(BaseObjectState&,
                                     std::uint32_t, std::uint32_t) {}
    virtual void RefreshEffect(BaseObjectState&, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t ArenaManager() { return 0u; }
    virtual void AddClump(std::uint32_t, std::uint32_t) {}
    virtual void RemoveClump(std::uint32_t, std::uint32_t) {}
    virtual void RemoveFrameChild(std::uint32_t, std::string_view,
                                  std::uint32_t) {}
    [[nodiscard]] virtual bool EffectAvailable(std::uint32_t) {
        return false;
    }
};

class BaseObjectWeapon final {
public:
    // IDA 0x442b60.
    [[nodiscard]] static std::uint32_t SetupRight(
        BaseObjectState& state, std::uint32_t weapon,
        std::uint32_t scale, std::uint32_t effect_name,
        IBaseObjectWeaponRuntime& runtime);
    // IDA 0x442d60.
    [[nodiscard]] static std::uint32_t SetupLeft(
        BaseObjectState& state, std::uint32_t weapon,
        std::uint32_t scale, std::uint32_t effect_name,
        IBaseObjectWeaponRuntime& runtime);
    // IDA 0x442f50.
    [[nodiscard]] static std::uint32_t CleanupRight(
        BaseObjectState& state, IBaseObjectWeaponRuntime& runtime);
    // IDA 0x442fd0.
    [[nodiscard]] static std::uint32_t CleanupLeft(
        BaseObjectState& state, IBaseObjectWeaponRuntime& runtime);
};

}  // namespace pal4
