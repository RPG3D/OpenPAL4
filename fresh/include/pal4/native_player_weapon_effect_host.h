#pragma once
#include "pal4/native_player_named_weapon_runtime.h"

namespace pal4 {
class NativeEffectCreation;
class NativeEffectHandleContentRuntime;
class NativeEffectHandleMutationRuntime;
class NativeEffectHandleContextRuntime;
class NativeEffectXmlObjects;

// Resource cleanup is delegated to the equipped-model owner. Texture ownership
// lives directly in NativePlayerNamedWeaponRuntime.
class INativePlayerWeaponResources {
public:
    virtual ~INativePlayerWeaponResources()=default;
    virtual void CleanupNamedResource(BaseObjectState&,std::uint32_t,std::uint32_t)=0;
};

// Borrows a stable scene effect session and the equipped-resource owner. Destroy
// this adapter and retire player handles before either dependency is destroyed.
class NativePlayerWeaponEffectHost final : public INativePlayerNamedWeaponHost {
public:
    NativePlayerWeaponEffectHost(NativeEffectCreation&,NativeEffectHandleContentRuntime&,
        NativeEffectHandleMutationRuntime&,NativeEffectHandleContextRuntime&,
        NativeEffectXmlObjects&,INativePlayerWeaponResources&);
    bool EffectNeedsUpdate(BaseObjectState&,std::uint32_t) override;
    void UpdateEffect(BaseObjectState&,std::uint32_t) override;
    bool EffectReady(BaseObjectState&,std::uint32_t) override;
    void DisplayAndCleanupEffect(BaseObjectState&,std::uint32_t) override;
    void CleanupNamedResource(BaseObjectState&,std::uint32_t,std::uint32_t) override;
    std::uint32_t EffectSingleton() override;
    bool EffectAvailable(std::uint32_t) override;
    BaseObjectEffectCreation CreateEffect(std::uint32_t,std::uint32_t,std::uint32_t,
        std::uint32_t,std::uint32_t) override;
    void ResetTemporary(BaseObjectEffectHandle&) override;
    void SetEffectProperties(BaseObjectState&,std::uint32_t,std::uint32_t) override;
    void SetEffectVisibility(BaseObjectState&,std::uint32_t,std::uint32_t) override;
    void RefreshEffect(BaseObjectState&,std::uint32_t) override;
    BaseObjectEffectHandle LoadXmlEffect(std::uint32_t,std::uint32_t,std::uint32_t) override;
private:
    NativeEffectCreation& creation_;
    NativeEffectHandleContentRuntime& content_;
    NativeEffectHandleMutationRuntime& mutation_;
    NativeEffectHandleContextRuntime& context_;
    NativeEffectXmlObjects& xml_;
    INativePlayerWeaponResources& resources_;
};
}
