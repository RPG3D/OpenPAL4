#pragma once
#include "pal4/portable/effect_cylinder.h"
#include "pal4/portable/effect_particle.h"
namespace pal4::portable {
// 原ObjectList引用及全部任务的所有者；不创建未引用模板。
class EffectSequence {
public:
    EffectSequence(const EffectDocument&,EffectTextures&,ParticleRandom&,ParticleCounts&,EffectObjectCounts&,bool start_now=true);
    EffectSequence(EffectPlan,EffectTextures&,ParticleRandom&,ParticleCounts&,EffectObjectCounts&,bool start_now=true);
    ~EffectSequence();
    EffectSequence(const EffectSequence&)=delete;
    EffectSequence& operator=(const EffectSequence&)=delete;
    void Advance(float elapsed);
    void Restart();
    void Stop();
    // Dialog分阶段接口：创建容器/准备对象与开始等待分别由原处理器调度。
    void BeginTasks(bool begin_waiting);
    void BeginWaiting();
    void AdvanceTasks(float elapsed);
    void AdvanceObjects(double elapsed);
    void AdvanceObject(std::size_t index,double elapsed);
    void EnsureObject(std::size_t index);
    void ReleaseObject(std::size_t index);
    bool HasObject(std::size_t index) const;
    bool IsDummy(std::size_t index) const;
    void ClearTasks();
    void DestroyTasks();
    bool HasTasks() const;
    void ResetObject(std::size_t index);
    void PreRenderObject(std::size_t index);
    void SetObjectMode(std::size_t index,bool timed);
    void HideObject(std::size_t index);
    double ObjectDuration(std::size_t index) const;
    void SetObjectParentTransform(std::size_t index,const std::array<float,16>&);
    bool TasksFinished() const;
    std::size_t ObjectCount() const;
    std::string_view ObjectName(std::size_t index) const;
    const EffectParticle* Particle(std::size_t index) const;
    const EffectCylinder* Cylinder(std::size_t index) const;
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
