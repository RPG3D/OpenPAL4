#pragma once
#include "pal4/base_object_attached_effect.h"
#include "pal4/effect_handle.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectCreation;
// Borrows the creating scene's effect services; actor fields retain ownership
// of the two handles, and must retire them before that scene is destroyed.
class NativeBaseObjectAttachedEffectRuntime final : public IBaseObjectAttachedEffectRuntime {
public:
    NativeBaseObjectAttachedEffectRuntime(NativeBaseObjectMemory&, NativeEffectCreation&,
        IEffectHandleContentRuntime&, IEffectHandleMutationRuntime&);
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t GetObjectTransform(std::uint32_t) override;
    bool EffectNeedsUpdate(BaseObjectState&, std::uint32_t) override;
    void UpdateEffect(BaseObjectState&, std::uint32_t) override;
    std::uint32_t EffectSingleton() override;
    BaseObjectEffectCreation CreateEffectObject(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t) override;
    void ResetTemporary(BaseObjectEffectHandle&) override;
    void SetVisibility(BaseObjectState&, std::uint32_t, std::uint32_t) override;
    void SetRotation(BaseObjectState&, std::uint32_t, float, float, float, std::uint8_t) override;
    void BindFrame(BaseObjectState&, std::uint32_t, std::uint32_t) override;
    void RefreshEffect(BaseObjectState&, std::uint32_t) override;
private:
    EffectSystemHandleWords Handle(BaseObjectState&, std::uint32_t) const;
    NativeBaseObjectMemory& memory_;
    NativeEffectCreation& creation_;
    IEffectHandleContentRuntime& content_;
    IEffectHandleMutationRuntime& mutation_;
};
}
