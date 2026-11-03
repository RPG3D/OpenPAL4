#pragma once
#include "pal4/portable/actor.h"
#include <memory>
#include <span>

namespace pal4::portable {
// 引擎开关跨场景保持；实例的材质与插件状态跨GPU重建保持。
class DistanceOptimization {
public:
    DistanceOptimization();
    ~DistanceOptimization();
    DistanceOptimization(const DistanceOptimization&)=delete;
    DistanceOptimization& operator=(const DistanceOptimization&)=delete;
    void SetEnabled(bool enabled);
    void SetScaleOption(std::uint32_t option);
    bool Enabled() const;
    float Threshold() const;
    void WriteReport(std::ostream& output) const;
private:
    friend class AtomicDistanceInstance;
    struct Impl;std::unique_ptr<Impl> impl_;
};
class AtomicDistanceInstance {
public:
    // 借用Actor，须比本实例长寿；旧缓存缺少映射时明确拒绝。
    AtomicDistanceInstance(const Actor& actor,DistanceOptimization& engine);
    ~AtomicDistanceInstance();
    AtomicDistanceInstance(const AtomicDistanceInstance&)=delete;
    AtomicDistanceInstance& operator=(const AtomicDistanceInstance&)=delete;
    std::span<const DrawAppearance> Evaluate(std::span<const Matrix> frames,const Matrix& model,
                                           std::array<float,3> camera);
    bool CollisionAllowed(std::size_t atomic) const;
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
