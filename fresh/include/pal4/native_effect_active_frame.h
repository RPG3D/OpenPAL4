#pragma once
#include "pal4/effect_system_active_frame.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;
class NativeEffectDialogLoader;
class NativeEffectObjectPool;
class NativeEffectTreeStorage;
class NativeParticleEffectFactory;
class NativeDialogParticlePlayback;
class NativeDialogTaskRuntime;
class INativeDialogParticleQueues;
class INativeContextEffectObjects;
class INativeEffectPlaybackEvents {
public:
    virtual ~INativeEffectPlaybackEvents()=default;
    virtual void Started(std::uint32_t group,const EffectSystemHandle&,std::int32_t repeats)=0;
    virtual void Finished(std::uint32_t group,const EffectSystemHandle&,std::int32_t repeats)=0;
    virtual void Removed(std::uint32_t group,const EffectSystemHandle&,std::uint32_t repeats)=0;
};

// Bound to a group with initialized active(+12) and inline(+212) trees.
// Borrows all services. Native group object update/draw remains a separate
// original frame phase; this adapter drives Dialog tasks and repeat/pool state.
class NativeEffectActiveFrame final : public IEffectSystemActiveFrameRuntime,
    public IEffectSystemCleanupEffectsRuntime {
public:
    NativeEffectActiveFrame(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeScriptObjectRuntime&,std::uint32_t group,NativeEffectTreeStorage&,
        NativeEffectDialogLoader&,NativeEffectObjectPool&,NativeParticleEffectFactory&,
        INativeDialogParticleQueues&,NativeDialogParticlePlayback&,
        NativeDialogTaskRuntime&,INativeEffectPlaybackEvents&);
    ~NativeEffectActiveFrame();
    void Update(double seconds);
    void Cleanup();
    void BindContextEffects(INativeContextEffectObjects* objects);
    [[nodiscard]] bool StartDialog(std::uint32_t dialog);
    [[nodiscard]] bool EnableDialogTasks(std::uint32_t dialog);
    void BeforeDestroyEffect(std::uint32_t) override;
    void DestroyEffect(std::uint32_t,std::uint32_t) override;
    void NotifyRemoved(std::uint32_t,const EffectSystemHandle&,std::uint32_t) override;
    void ClearTree(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void DestroyTreeBranch(std::uint32_t) override;
    void FreeNode(std::uint32_t) override;
    void EraseNode(std::uint32_t,std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    std::uint32_t SharedNil(std::uint32_t tree) override;
    std::uint32_t RemoveInactiveInline(std::uint32_t tree,std::uint32_t node) override;
    void UpdatePlayback(std::uint32_t,double) override;
    void StartPlayback(std::uint32_t) override;
    void PausePlayback(std::uint32_t) override;
    void ResetDialog(std::uint32_t) override;
    void ReturnEffect(std::uint32_t,std::uint32_t) override;
    EffectSystemHandle CopyHandle(std::uint32_t,std::uint32_t) override;
    void ResetTemporaryHandle(EffectSystemHandle&) override;
    void NotifyStarted(std::uint32_t,const EffectSystemHandle&,std::int32_t) override;
    void NotifyFinished(std::uint32_t,const EffectSystemHandle&,std::int32_t) override;
    std::uint32_t FindXmlAttribute(std::uint32_t,std::string_view) override;
    void SetXmlInt(std::uint32_t,std::uint32_t) override;
    void AddXmlInt(std::uint32_t,std::string_view,std::uint32_t) override;
    std::uint32_t EraseActiveNode(std::uint32_t,std::uint32_t) override;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
