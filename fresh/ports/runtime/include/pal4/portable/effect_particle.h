#pragma once
#include "pal4/portable/effect_texture.h"
#include "pal4/portable/effect_tasks.h"
namespace pal4::portable {
class ParticleRandom {
public:
    virtual ~ParticleRandom()=default;
    virtual unsigned Time()=0;
    virtual void Seed(unsigned)=0;
    virtual int Next15()=0;
};
struct ParticleVertexData {
    std::array<float,3> position{};
    std::array<std::uint8_t,4> color{};
    std::array<float,2> size{};
    std::array<float,4> uv{};
    float rotation{};
};
struct ParticleCounts {unsigned instances{},created{};};
// 原PTank粒子配置/发射/模拟/流输出；平台绘制器消费公开快照。
class EffectParticle final:public EffectVisualTarget {
public:
    EffectParticle(const BinaryXmlNode&,EffectTextures&,ParticleRandom&,ParticleCounts&);
    ~EffectParticle() override;
    EffectParticle(const EffectParticle&)=delete;
    EffectParticle& operator=(const EffectParticle&)=delete;
    void Advance(double elapsed,bool skip_emission=false);
    void SetEmission(bool enabled);
    void Show(bool visible) override;
    bool Visible() const;
    void Reset();
    void PreRender();
    void SetTimedMode(bool enabled);
    std::array<float,3> Scale() const override;
    void SetScale(std::array<float,3>,bool additive=false) override;
    std::array<std::uint8_t,4> Color() const override;
    void SetColor(std::array<std::uint8_t,4>) override;
    void SetColorMode(std::uint8_t mode) override;
    bool HasEmissionFacet() const override{return true;}
    void StopEmission() override;
    void SetEmissionTime(float seconds,bool increase) override;
    std::vector<ParticleVertexData> Snapshot() const;
    const EffectTexture& Texture() const;
    const std::array<float,16>& Transform() const;
    unsigned SourceBlend() const;
    unsigned DestinationBlend() const;
    unsigned Capacity() const;
    double Duration() const;
    void SetParentTransform(const std::array<float,16>&);
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
Scene CreateParticleScene(const EffectParticle&);
void UpdateParticleScene(Scene&,const EffectParticle&,const std::array<float,16>& view_projection);
}
