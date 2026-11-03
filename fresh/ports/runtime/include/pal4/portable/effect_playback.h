#pragma once
#include "pal4/portable/effect_dialog.h"
#include "pal4/effect_system_object_f_queue.h"
#include <functional>
namespace pal4::portable {
// 每个场景/特效组的所有者；句柄值不含宿主指针。
class EffectPlayback {
public:
    using PlanLoader=std::function<EffectPlan(std::string_view)>;
    using Handle=pal4::EffectSystemHandleWords;
    EffectPlayback(PlanLoader,EffectTextures&,ParticleRandom&,ParticleCounts&,EffectObjectCounts&);
    ~EffectPlayback();
    EffectPlayback(const EffectPlayback&)=delete;
    EffectPlayback& operator=(const EffectPlayback&)=delete;
    Handle Create(std::string_view name,std::int32_t repeats,bool distance_clip);
    // Empty means the referenced retail object/task classes are implemented.
    // Malformed/missing CPK resources still throw rather than being hidden.
    std::string SupportIssue(std::string_view name) const;
    bool Contains(Handle) const;
    void Attach(Handle,const EffectMatrix& parent);
    void SetTransform(Handle,Point3 position,Point3 rotation,Point3 scale);
    bool BeginWaiting(Handle);
    void Stop(Handle&);
    void Release(Handle&,bool force=true);
    void Advance(float elapsed);
    void Clear();
    const EffectDialog* Dialog(Handle) const;
    void Visit(const std::function<void(std::uint32_t,const EffectDialog&)>&) const;
    std::size_t ActiveCount() const;
    std::size_t PendingCount() const;
    std::size_t CachedCount() const;
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
