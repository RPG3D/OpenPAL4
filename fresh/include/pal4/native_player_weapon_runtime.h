#pragma once

#include "pal4/native_player_weapon_cleanup.h"

namespace pal4 {
class NativePlayerTransformRuntime;

// Creation/playback stays with the effect-system owner, including empty names.
class INativePlayerWeaponSetupHost : public INativePlayerWeaponCleanupHost {
public:
    virtual std::uint32_t EffectSingleton() = 0;
    virtual bool EffectAvailable(std::uint32_t name) = 0;
    virtual BaseObjectEffectCreation CreateEffect(std::uint32_t system,
        std::uint32_t name, std::uint32_t position, std::uint32_t first,
        std::uint32_t second) = 0;
    virtual void ResetTemporary(BaseObjectEffectHandle&) = 0;
    virtual void SetEffectProperties(BaseObjectState&, std::uint32_t offset,
                                      std::uint32_t frame) = 0;
    virtual void SetEffectVisibility(BaseObjectState&, std::uint32_t offset,
                                      std::uint32_t frame) = 0;
    virtual void RefreshEffect(BaseObjectState&, std::uint32_t offset) = 0;
};

// SDK placement and shared ordinary-weapon setup; borrows all services.
// Named resource loading/XML setup remain a separate required composition.
class NativePlayerWeaponRuntime : public NativePlayerWeaponCleanup {
public:
    NativePlayerWeaponRuntime(RpWorld& world, std::uint32_t game,
        INativePlayerWeaponSetupHost& effects, NativePlayerTransformRuntime& transforms)
        : NativePlayerWeaponCleanup(world, game, effects), effects_(effects), transforms_(transforms) {}
    std::uint32_t GetUserData(std::uint32_t clump, std::string_view hook) override;
    void InvertTransform(std::uint32_t destination, std::uint32_t source) override;
    void ScaleFrame(std::uint32_t frame, std::uint32_t scale, std::uint32_t mode) override;
    void UpdateFrame(std::uint32_t frame) override;
    void RotateMatrix(std::uint32_t matrix, std::uint32_t axis, float degrees,
                       std::uint32_t mode) override;
    void ProcessUserData(std::uint32_t clump, std::string_view hook, std::uint32_t frame) override;
    void AddClump(std::uint32_t arena, std::uint32_t clump) override;
    std::uint32_t EffectSingleton() override;
    bool EffectAvailable(std::uint32_t name) override;
    BaseObjectEffectCreation CreateEffect(std::uint32_t system, std::uint32_t name,
        std::uint32_t position, std::uint32_t first, std::uint32_t second) override;
    void ResetTemporary(BaseObjectEffectHandle&) override;
    void SetEffectProperties(BaseObjectState&, std::uint32_t offset, std::uint32_t frame) override;
    void SetEffectVisibility(BaseObjectState&, std::uint32_t offset, std::uint32_t frame) override;
    void RefreshEffect(BaseObjectState&, std::uint32_t offset) override;
private:
    INativePlayerWeaponSetupHost& effects_;
    NativePlayerTransformRuntime& transforms_;
};
}  // namespace pal4
