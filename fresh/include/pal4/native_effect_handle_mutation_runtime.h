#pragma once
#include "pal4/effect_handle.h"
#include "pal4/native_dialog_transform_runtime.h"

namespace pal4 {
class NativeEffectDialogLoader;
class NativeEventDocumentRuntime;
class NativeEffectActiveFrame;

// Borrows the existing registry/property boundary and actual Dialog owners.
// Frame transforms use public SDK APIs through NativeDialogTransformRuntime.
class NativeEffectHandleMutationRuntime final : public IEffectHandleMutationRuntime {
public:
    NativeEffectHandleMutationRuntime(IEffectHandlePropertyRuntime&,
        NativeEffectDialogLoader&,NativeEventDocumentRuntime&,NativeEffectActiveFrame&);
    std::uint32_t InitializeSystem() override;
    bool Validate(std::uint32_t,std::uint32_t) override;
    std::uint32_t FindElement(std::uint32_t,const EffectSystemHandleWords&) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t RegisterSystem() override;
    bool ContainsOwner(std::uint32_t,std::uint32_t) override;
    std::string ReadNameAttribute(std::uint32_t) override;
    std::int32_t ReadConfigInt(std::uint32_t,std::string_view) override;
    void WriteConfigInt(std::uint32_t,std::string_view,std::int32_t) override;
    std::uint8_t ApplyVector(EffectHandleVectorProperty,std::uint32_t,
        const std::array<float,3>&,bool) override;
    std::uint8_t ApplyParentFrame(std::uint32_t,std::uint32_t) override;
    std::uint8_t ApplyVisibility(std::uint32_t,std::uint32_t) override;
    // Native name is a borrowed NUL-terminated C string, never a VC6 string image.
    std::uint8_t ApplyFacet(std::uint32_t dialog,std::uint32_t name,std::uint32_t facet) override;
    std::uint8_t PausePlayback(std::uint32_t) override;
private:
    IEffectHandlePropertyRuntime& properties_;
    NativeEffectDialogLoader& loader_;
    NativeEventDocumentRuntime& documents_;
    NativeEffectActiveFrame& active_;
    NativeDialogTransformRuntime transforms_;
};
}
