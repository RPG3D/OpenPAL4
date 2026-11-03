#include "pal4/portable/effect_particle.h"
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
Scene CreateParticleScene(const EffectParticle& particle){
    Scene scene;scene.saturation={1,1,1};scene.textures.push_back(particle.Texture().image);
    scene.vertices.resize(particle.Capacity()*6);scene.draws.push_back({0,0,0,-1,{1,1,1,1}});return scene;
}
void UpdateParticleScene(Scene& scene,const EffectParticle& particle,const std::array<float,16>& vp){
    if(scene.vertices.size()!=particle.Capacity()*6 || scene.draws.size()!=1)throw std::logic_error("particle mesh capacity mismatch");
    const auto axis=[](std::array<float,3> v){float n=0;for(float c:v)n+=c*c;n=std::sqrt(n);if(!std::isfinite(n)||n<1e-8f)throw std::logic_error("invalid particle camera basis");for(float& c:v)c/=n;return v;};
    const auto right=axis({vp[0],vp[4],vp[8]}),up=axis({vp[1],vp[5],vp[9]});
    const auto particles=particle.Snapshot();if(particles.size()>particle.Capacity())throw std::logic_error("particle snapshot capacity exceeded");
    scene.draws[0].count=particle.Visible()?static_cast<unsigned>(particles.size())*6:0;
    constexpr unsigned corners[]{0,1,2,2,1,3};
    for(std::size_t i=0;i<particles.size();++i){const auto& p=particles[i];const auto c=std::cos(p.rotation),s=std::sin(p.rotation);
        for(unsigned j=0;j<6;++j){const auto k=corners[j];const float x=((k&1)?0.5f:-0.5f)*p.size[0],y=((k&2)?0.5f:-0.5f)*p.size[1];const float rx=c*x-s*y,ry=s*x+c*y;
            auto& v=scene.vertices[i*6+j];for(unsigned a=0;a<3;++a)v.position[a]=p.position[a]+right[a]*rx+up[a]*ry;
            v.uv0[0]=p.uv[(k&1)?2:0];v.uv0[1]=p.uv[(k&2)?3:1];v.uv1[0]=v.uv1[1]=0;
            for(unsigned a=0;a<4;++a)v.color[a]=float(p.color[a])/255.f;
        }
    }
}
}
