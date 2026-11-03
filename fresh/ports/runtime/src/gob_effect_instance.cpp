#include "pal4/portable/gob_effect_instance.h"
#include "pal4/gob_effect.h"
#include "pal4/effect_handle.h"
#include <bit>
#include <stdexcept>
namespace pal4::portable {
namespace {void Require(bool v,const char* message){if(!v)throw std::logic_error(message);}}
struct GobEffectInstance::Impl final:IGobEffectVisibilityRuntime {
    static constexpr unsigned object=0x1000,clump=0x2000,frame=0x3000,handle=object+GobEffect::kEffectHandleOffset;
    GobEffectParameters parameters;EffectMatrix parent;EffectPlayback& playback;
    EffectPlayback::Handle owned{0xffffffffu,0},temporary{0xffffffffu,0};unsigned visible{};
    Impl(GobEffectParameters p,EffectMatrix matrix,EffectPlayback& effects):parameters(std::move(p)),parent(matrix),playback(effects){ValidateEffectMatrix(parent);}
    void CheckHandle(unsigned h) const{Require(h==handle,"invalid GOB effect handle");}
    void StoreWord(unsigned a,unsigned v) override{if(a==object+0x184){visible=v;return;}if(a==handle){owned.id=v;return;}Require(a==handle+4,"invalid GOB effect write");owned.system=v;}
    unsigned ReadWord(unsigned a) override{if(a==object+0x1f0)return parameters.times;if(a==object+0x1f8)return parameters.distance_clip;if(a==object+0x14)return clump;Require(a==clump+4,"invalid GOB effect read");return frame;}
    bool EffectNeedsUpdate(unsigned h) override{CheckHandle(h);return playback.Contains(owned);}
    void HandleEffectEvent(unsigned h,unsigned context,unsigned mode) override{CheckHandle(h);Require(context==0 && mode==1,"unsupported GOB effect event");playback.Release(owned,true);}
    std::string EffectName(unsigned a) override{Require(a==object+GobEffect::kStringOffset,"invalid GOB effect name");return parameters.name;}
    std::pair<unsigned,unsigned> CreateEffect(std::string_view name,unsigned times,bool clip) override{temporary=playback.Create(name,std::bit_cast<std::int32_t>(times),clip);return {temporary.id,temporary.system};}
    void ResetTemporaryEffect() override{EffectHandle::Reset(temporary);}
    void UpdateEffectProperties(unsigned h,unsigned f) override{CheckHandle(h);Require(f==frame,"invalid GOB effect parent frame");playback.Attach(owned,parent);}
    std::uint8_t UpdateEffectState(unsigned h) override{CheckHandle(h);return playback.BeginWaiting(owned);}
    std::uint8_t UpdateDialogContent(unsigned h) override{CheckHandle(h);playback.Stop(owned);return 1;}
};
GobEffectInstance::GobEffectInstance(GobEffectParameters p,EffectMatrix parent,EffectPlayback& playback):impl_(std::make_unique<Impl>(std::move(p),parent,playback)){}
GobEffectInstance::~GobEffectInstance()=default;
void GobEffectInstance::SetVisible(bool visible){(void)GobEffect::SetVisibleAndCreateEffect(Impl::object,0,visible?1:0,*impl_);}
bool GobEffectInstance::Visible() const{return impl_->visible!=0;}
EffectPlayback::Handle GobEffectInstance::Handle() const{return impl_->owned;}
}
