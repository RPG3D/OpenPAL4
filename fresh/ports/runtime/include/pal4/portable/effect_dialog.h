#pragma once
#include "pal4/portable/effect_sequence.h"
#include "pal4/portable/effect_transform.h"
#include "pal4/portable/world.h"
namespace pal4::portable {
// 原Dialog生命周期；帧更新、primitive更新分别由上层EffectSystem调度。
// 本类不拥有GOB句柄、active/pending计时器或GPU对象。
class EffectDialog {
public:
    EffectDialog(EffectPlan,EffectTextures&,ParticleRandom&,ParticleCounts&,EffectObjectCounts&);
    ~EffectDialog();
    EffectDialog(const EffectDialog&)=delete;
    EffectDialog& operator=(const EffectDialog&)=delete;
    bool Prepare();
    bool BeginWaiting();
    void AdvanceTasks(float elapsed);
    void AdvanceObjects(double elapsed);
    void Reset();
    void ReturnToCache();
    double Stop(bool detach_frame);
    void SetParentTransform(const EffectMatrix&);
    void RestoreDetachedTransform(const EffectMatrix&);
    void SetTransform(Point3 position,Point3 rotation,Point3 scale);
    bool HasTasks() const;
    bool Prepared() const;
    bool Waiting() const;
    std::size_t QueuedObjects() const;
    const EffectMatrix& WorldTransform() const;
    const EffectSequence& Sequence() const;
    // Original retail validator uses a 1200-unit default camera radius.
    bool RenderAllowed(std::size_t index,Point3 camera,float radius=1200.f) const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
