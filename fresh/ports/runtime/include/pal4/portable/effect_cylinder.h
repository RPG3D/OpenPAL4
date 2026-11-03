#pragma once
#include "pal4/portable/effect_geometry.h"
#include "pal4/portable/effect_texture.h"
#include "pal4/portable/effect_tasks.h"
namespace pal4::portable {
struct EffectObjectCounts {std::uint32_t cylinders{},created{};};
class EffectCylinder final:public EffectVisualTarget {
public:
    // 借用计数拥有者；纹理及文档使用拥有所有权的数据，不依赖原节点寿命。
    EffectCylinder(const BinaryXmlNode& document,EffectTextures& textures,EffectObjectCounts& counts);
    ~EffectCylinder() override;
    EffectCylinder(const EffectCylinder&)=delete;
    EffectCylinder& operator=(const EffectCylinder&)=delete;
    const Scene& Mesh() const;
    const std::array<float,16>& Transform() const;
    std::vector<DrawAppearance> Appearance() const;
    bool Additive() const;
    bool Visible() const;
    double Duration() const;
    void SetParentTransform(const std::array<float,16>&);
    void Show(bool visible) override;
    void Reset();
    std::array<float,3> Scale() const override;
    void SetScale(std::array<float,3> value,bool additive=false) override;
    void SetRotation(std::array<float,3> value,bool additive=false);
    void SetPosition(std::array<float,3> value,bool additive=false);
    void SetColor(std::array<std::uint8_t,4> value) override;
    std::array<std::uint8_t,4> Color() const override;
    void SetColorMode(std::uint8_t mode) override;
    void SetTimedMode(bool enabled);
    void Advance(double elapsed);
    void WriteReport(std::ostream& out) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
