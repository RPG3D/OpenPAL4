#include "pal4/portable/atomic_distance.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
constexpr Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
Actor Fixture(bool shared=false) {
    Actor actor;actor.frames={{-1,identity},{0,identity}};
    actor.atomic_spheres={{0,{0,0,0,1}},{1,{0,0,0,1}}};
    actor.mesh.draws.resize(2);auto& b=actor.render_bindings.emplace();
    b.materials={{10,20,30,40},{80,70,60,50}};b.geometry_materials={{0},{1}};
    b.atomics={{0,4},{shared?0u:1u,4}};b.draws={{0,0},{1,0}};return actor;
}
}
int main() {
    try {
        DistanceOptimization engine;Check(engine.Enabled() && engine.Threshold()==1200,"original default threshold");
        auto actor=Fixture();AtomicDistanceInstance instance(actor,engine);
        std::vector<Matrix> frames{identity,identity};frames[1][12]=2000;
        auto result=instance.Evaluate(frames,identity,{0,0,0});
        Check(result[0].visible && result[0].color==std::array<float,4>{1,1,1,1} && !result[1].visible,"per Atomic visibility");
        Check(instance.CollisionAllowed(0) && !instance.CollisionAllowed(1),"collision reads the same original 8450 state");
        frames[0][12]=1175;result=instance.Evaluate(frames,identity,{0,0,0});
        Check(result[0].visible && result[0].color[3]>.48f && result[0].color[3]<.52f,"50 unit fade band");
        const auto faded=result[0].color;Check(faded[0]==faded[3],"fade must modify RGB and alpha");
        engine.SetEnabled(false);frames[0][12]=2500;result=instance.Evaluate(frames,identity,{0,0,0});
        Check(result[0].visible && result[1].visible && result[0].color==faded,"disabled preserves previous material");
        engine.SetEnabled(true);result=instance.Evaluate(frames,identity,{0,0,0});Check(!result[0].visible,"reenable clips");
        frames[0][12]=0;frames[1][12]=0;result=instance.Evaluate(frames,identity,{0,0,0});
        Check(result[0].visible && result[0].color[3]==1,"return inside restores white");
        auto model=identity;model[12]=3000;result=instance.Evaluate(frames,model,{0,0,0});Check(!result[0].visible,"instance transform used");
        result=instance.Evaluate(frames,model,{3000,0,0});Check(result[0].visible,"actual camera position used");
        engine.SetScaleOption(0);Check(engine.Threshold()==1800,"original long distance option");
        frames[0][12]=1500;Check(instance.Evaluate(frames,identity,{0,0,0})[0].visible,"long distance rendered");
        engine.SetScaleOption(1);Check(!instance.Evaluate(frames,identity,{0,0,0})[0].visible,"short distance culled");
        auto shared=Fixture(true);AtomicDistanceInstance shared_instance(shared,engine);
        frames[0][12]=1175;frames[1][12]=0;result=shared_instance.Evaluate(frames,identity,{0,0,0});
        Check(result[0].visible && result[1].visible && result[0].color==result[1].color && result[1].color[3]<1,"shared geometry retains actual material writes");
        frames[0][12]=0;result=shared_instance.Evaluate(frames,identity,{0,0,0});Check(result[1].color[3]==1,"shared geometry restoration");
        // Same shared material through a distinct geometry must also observe mutations.
        auto alias=Fixture();alias.render_bindings->geometry_materials[1][0]=0;AtomicDistanceInstance alias_instance(alias,engine);
        frames[0][12]=1175;result=alias_instance.Evaluate(frames,identity,{0,0,0});Check(result[0].color==result[1].color,"shared material alias");
        auto legacy=Fixture();legacy.render_bindings.reset();bool rejected=false;try{AtomicDistanceInstance old(legacy,engine);}catch(const std::logic_error&){rejected=true;}Check(rejected,"legacy caches must fail explicitly");
        rejected=false;try{(void)instance.Evaluate({},identity,{0,0,0});}catch(const std::logic_error&){rejected=true;}Check(rejected,"missing pose matrices must fail");
        std::cout<<"original Atomic clipping, fade, shared materials, camera/model pose and disable persistence passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
