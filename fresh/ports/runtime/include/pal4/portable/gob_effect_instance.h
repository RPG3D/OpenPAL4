#pragma once
#include "pal4/portable/effect_playback.h"
namespace pal4::portable {
// GOB配置/显隐桥接；生命周期由所借用的场景EffectPlayback持有。
class GobEffectInstance {
public:
    GobEffectInstance(GobEffectParameters,EffectMatrix parent,EffectPlayback&);
    ~GobEffectInstance();
    GobEffectInstance(const GobEffectInstance&)=delete;
    GobEffectInstance& operator=(const GobEffectInstance&)=delete;
    void SetVisible(bool);
    bool Visible() const;
    EffectPlayback::Handle Handle() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
