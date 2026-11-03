#include "pal4/behavior_helper_physics.h"
#if defined(PAL4_TEST_NATIVE_HELPER_PHYSICS)
#include "pal4/native_behavior_helper_physics.h"
#endif
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace pal4;
void Check(bool ok,const char* message) {
    if(!ok) throw std::runtime_error(message);
}
float Get(const GridManagerState& state,std::size_t word) {
    return std::bit_cast<float>(state.base_words[word]);
}
void Set(GridManagerState& state,std::size_t word,float value) {
    state.base_words[word]=std::bit_cast<std::uint32_t>(value);
}
void Near(float actual,float expected,float tolerance=1e-5f) {
    Check(std::isfinite(actual) && std::abs(actual-expected)<=tolerance,"numeric contract");
}
GridManagerState Image(std::uint32_t type) {
    GridManagerState s{};
    s.base_words[0]=type;
    Set(s,1,1.f);Set(s,5,1.f);Set(s,9,1.f); // right/up/forward
    Set(s,23,2.f);Set(s,26,4.f);Set(s,27,1.f); // mass/force/speed caps
    Set(s,31,1.f); // previous forward
    s.nested_grid=0xfedcba98u;s.owned_memory=0xdeadbeefu;
    s.reserved_a=0x13579bdfu;s.reserved_b=0x2468ace0u;
    s.update_enabled=0xa5u;s.reset_pending=0x5au;
    return s;
}
bool Equal(const GridManagerState& a,const GridManagerState& b) {
    return std::memcmp(&a,&b,sizeof(a))==0;
}
void OpaqueUnchanged(const GridManagerState& s,const GridManagerState& old) {
    Check(s.base_words[0]==old.base_words[0] && s.nested_grid==old.nested_grid &&
          s.owned_memory==old.owned_memory && s.reserved_a==old.reserved_a &&
          s.reserved_b==old.reserved_b && s.update_enabled==old.update_enabled &&
          s.reset_pending==old.reset_pending,"opaque IDs/flags changed");
    for(const auto w:{23u,24u,26u,27u}) Check(s.base_words[w]==old.base_words[w],"parameters changed");
}
void InvalidFrames() {
    for(float dt:{-0.1f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        auto s=Image(0x842898u);const auto old=s;
        bool rejected=false;
        try { BehaviorHelperPhysics::Integrate(s,{0.f,0.f,1.f},dt); }
        catch(const std::invalid_argument&) {rejected=true;}
        Check(rejected && Equal(s,old),"invalid integration frame wrote state");
        rejected=false;
        try { (void)BehaviorHelperPhysics::PredictPosition(s,dt); }
        catch(const std::invalid_argument&) {rejected=true;}
        Check(rejected && Equal(s,old),"invalid prediction frame wrote state");
    }
    for(float bad:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        for(std::size_t axis=0;axis<3;++axis) {
            auto s=Image(0x842898u);const auto old=s;BehaviorVector3 v{};v[axis]=bad;
            bool rejected=false;
            try {BehaviorHelperPhysics::Integrate(s,v,1.f/60.f);}
            catch(const std::invalid_argument&) {rejected=true;}
            Check(rejected && Equal(s,old),"invalid requested vector wrote state");
        }
    }
    auto unsupported=Image(0x842777u);const auto old=unsupported;
    bool rejected=false;
    try {BehaviorHelperPhysics::Integrate(unsupported,{},0.f);}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected && Equal(unsupported,old),"unsupported helper accepted/written");
    rejected=false;
    try {(void)BehaviorHelperPhysics::PredictPosition(unsupported,0.f);}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"unsupported prediction helper accepted");
}
void Prediction() {
    auto s=Image(0x842ae8u);Set(s,10,2.f);Set(s,11,-3.f);Set(s,12,5.f);Set(s,25,0.75f);
    const auto old=s;const auto predicted=BehaviorHelperPhysics::PredictPosition(s,2.f);
    Near(predicted[0],2.f);Near(predicted[1],-3.f);Near(predicted[2],6.5f);
    Check(Equal(s,old),"prediction mutated image");
    const auto zero=BehaviorHelperPhysics::PredictPosition(s,0.f);
    Near(zero[0],2.f);Near(zero[1],-3.f);Near(zero[2],5.f);
}
void Integration(std::uint32_t type) {
    struct Guarded {std::array<std::uint8_t,17> prefix;GridManagerState image;std::array<std::uint8_t,75> tail;};
    Guarded g{};g.prefix.fill(0xa7);g.tail.fill(0x39);g.image=Image(type);const auto original=g.image;
    // Exact first step: capped force4 / mass2, blend .15, v=.005, z=v/60.
    BehaviorHelperPhysics::Integrate(g.image,{0.f,0.f,100.f},1.f/60.f);
    Near(Get(g.image,39),0.f);Near(Get(g.image,40),0.f);Near(Get(g.image,41),0.3f);
    Near(Get(g.image,25),0.005f);Near(Get(g.image,12),0.005f/60.f);
    auto relocated=g.image;
    for(int frame=1;frame<120;++frame) {
        const auto before=Get(g.image,12);
        const BehaviorVector3 first{0.f,0.f,100.f};
        const BehaviorVector3 second=first;
        BehaviorHelperPhysics::Integrate(g.image,first,1.f/60.f);
        BehaviorHelperPhysics::Integrate(relocated,second,1.f/60.f);
        Check(Equal(g.image,relocated),"state/vector address affected integration");
        Check(Get(g.image,12)>=before,"forward movement reversed");
        Check(Get(g.image,25)<=1.f+1e-6f,"speed cap exceeded");
        OpaqueUnchanged(g.image,original);
    }
    Check(Get(g.image,12)>1.f && Get(g.image,12)<2.f,"two-second travel outside force/speed bounds");
    Near(Get(g.image,1),1.f);Near(Get(g.image,5),1.f);Near(Get(g.image,9),1.f);
    for(auto b:g.prefix) Check(b==0xa7,"prefix overwritten");
    for(auto b:g.tail) Check(b==0x39,"derived endpoint/random tail overwritten");
    const auto old=g.image;
    BehaviorHelperPhysics::Integrate(g.image,{},0.f);
    Check(Equal(old,g.image),"zero frame changed steady image");
    std::cout<<"type="<<type<<" speed="<<Get(g.image,25)<<" z="<<Get(g.image,12)
             <<" smoothed_z="<<Get(g.image,37)<<'\n';
}
#if defined(PAL4_TEST_NATIVE_HELPER_PHYSICS)
void NativeWrapper() {
    static_assert(sizeof(void*)==4u);
    struct Guarded {std::array<std::uint8_t,13> prefix;GridManagerState image;std::array<std::uint8_t,78> tail;};
    for(auto type:{0x842898u,0x842ae8u}) {
        Guarded g{};g.prefix.fill(0xc3);g.tail.fill(0x7e);g.image=Image(type);
        auto typed=g.image;
        const auto address=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&g.image));
        for(int frame=0;frame<120;++frame) {
            const auto a=BehaviorHelperPhysics::PredictPosition(typed,1.f/60.f);
            const auto b=NativeBehaviorHelperPhysics::PredictPosition(address,1.f/60.f);
            Check(a==b,"native/core prediction mismatch");
            BehaviorHelperPhysics::Integrate(typed,{0.f,0.f,100.f},1.f/60.f);
            NativeBehaviorHelperPhysics::Integrate(address,{0.f,0.f,100.f},1.f/60.f);
            Check(Equal(g.image,typed),"native/core integration mismatch");
        }
        for(auto b:g.prefix) Check(b==0xc3,"native prefix overwritten");
        for(auto b:g.tail) Check(b==0x7e,"native derived tail overwritten");
    }
    bool rejected=false;
    try {NativeBehaviorHelperPhysics::Integrate(0u,{},0.f);}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"null native helper accepted");
    rejected=false;
    try {NativeBehaviorHelperPhysics::Integrate(1u,{},-1.f);}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"invalid native frame dereferenced helper");
}
#endif
void TurnAndConstrain() {
    auto s=Image(0x842898u);Set(s,25,0.5f); // above low-speed threshold .2
    BehaviorHelperPhysics::Integrate(s,{4.f,0.f,0.f},0.1f);
    // blend .4: acceleration(.8,0,0), velocity(.08,0,.5)
    Near(Get(s,10),0.008f);Near(Get(s,12),0.05f);
    const float speed=std::sqrt(0.08f*0.08f+0.5f*0.5f);
    Near(Get(s,25),speed);Near(Get(s,7),0.08f/speed);Near(Get(s,9),0.5f/speed);
    Near(Get(s,32),Get(s,10));Near(Get(s,34),Get(s,12));
    Check(std::isfinite(Get(s,28)) && std::isfinite(Get(s,38)),"curvature not finite");
    auto low=Image(0x842898u);
    BehaviorHelperPhysics::Integrate(low,{4.f,0.f,0.f},0.1f);
    Near(Get(low,10),0.f);Near(Get(low,12),0.008f); // original threshold=1 redirects along forward
}
}
int main() try {
    std::cout<<std::setprecision(9);
#if defined(PAL4_TEST_NATIVE_HELPER_PHYSICS)
    NativeWrapper();
#endif
    InvalidFrames();Prediction();Integration(0x842898u);Integration(0x842ae8u);TurnAndConstrain();
    std::cout<<"shared helper physics contracts passed; steering/world/gameplay not executed\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
