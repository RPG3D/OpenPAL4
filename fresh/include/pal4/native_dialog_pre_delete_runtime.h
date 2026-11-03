#pragma once
#include "pal4/native_dialog_reset_runtime.h"

namespace pal4 {
class NativeEventDocumentRuntime;
class NativeEffectTextManager;
class NativeDialogObject;

class INativeDialogDeleteEffects : public INativeDialogResetEffects {
public:
    virtual std::uint32_t EffectFrame(std::uint32_t effect)=0;
    virtual void DestroyEffect(std::uint32_t effect)=0;
};

// Required concrete effect dispatcher is borrowed, as are allocator, document,
// task and template owners. Prepare releases active runtime state before the
// Dialog owner runs ordinary destruction. It preserves configuration capacities.
class NativeDialogPreDeleteRuntime final : public IDialogObjectPreDeleteRuntime {
public:
    NativeDialogPreDeleteRuntime(NativeBaseObjectMemory&,NativeScriptObjectRuntime&,
        NativeEventDocumentRuntime&,NativeEffectTextManager&,INativeDialogDeleteEffects&);
    // Raw image path for legacy tree fixtures. Native owners use the overload
    // below so their modern facet storage is retired with playback/config.
    void Prepare(DialogObjectState&,std::uint32_t facet_nil);
    void Prepare(NativeDialogObject&);
    void ResetProperties(DialogObjectState&) override;
    void DestroyXmlNode(std::uint32_t) override;
    void Free(std::uint32_t) override;
    void ReleaseEffectReferences(std::uint32_t) override;
    void ReleaseTextRecord(std::uint32_t) override;
    void DestroyFrame(std::uint32_t) override;
    void CleanupXmlDocument(DialogObjectState&) override;
    void ClearTextElements(std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t,std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t,std::uint32_t) override;
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t) override;
    std::uint32_t EffectFrame(std::uint32_t) override;
    std::uint32_t FrameParent(std::uint32_t) override;
    void RemoveFrameChild(std::uint32_t) override;
    void ProcessEffectEvent(std::uint32_t,std::uint32_t) override;
    void DestroyEffect(std::uint32_t) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeEventDocumentRuntime& documents_;
    NativeEffectTextManager& templates_;
    INativeDialogDeleteEffects& effects_;
    NativeDialogResetRuntime reset_;
};
} // namespace pal4
