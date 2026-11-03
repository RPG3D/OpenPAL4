#pragma once
#include "pal4/portable/effect_document.h"
#include <functional>
#include <stdexcept>
namespace pal4::portable {
// 借用实际对象的视觉接口；调度器不拥有对象、网格或GPU资源。
class EffectVisualTarget {
public:
    virtual ~EffectVisualTarget()=default;
    virtual void Show(bool visible)=0;
    virtual std::array<float,3> Scale() const=0;
    virtual void SetScale(std::array<float,3> scale,bool additive=false)=0;
    virtual std::array<std::uint8_t,4> Color() const=0;
    virtual void SetColor(std::array<std::uint8_t,4> color)=0;
    virtual void SetColorMode(std::uint8_t mode)=0;
    virtual bool HasEmissionFacet() const{return false;}
    virtual void StopEmission(){throw std::logic_error("effect emission facet is not bound");}
    virtual void SetEmissionTime(float,bool){throw std::logic_error("effect emission facet is not bound");}
};
struct EffectTaskBinding {BinaryXmlNode configuration;EffectVisualTarget* target{};bool dummy{};};
class EffectTasks {
public:
    // 复制配置；target和随机服务须覆盖调度器寿命。未提供随机服务时，随机任务明确失败。
    explicit EffectTasks(std::vector<EffectTaskBinding> bindings,std::function<int()> random15={},bool begin_waiting=true);
    ~EffectTasks();
    EffectTasks(const EffectTasks&)=delete;
    EffectTasks& operator=(const EffectTasks&)=delete;
    void Advance(float elapsed);
    void Stop();
    void BeginWaiting();
    bool Finished() const;
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
