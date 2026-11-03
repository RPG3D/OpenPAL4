#include "pal4/portable/original_actor.h"
#include "pal4/portable/original_animation.h"
#include "pal4/portable/effect_texture.h"
#include "pal4/portable/image_texture.h"
#include "pal4/portable/package.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value,const char* error){if(!value)throw std::runtime_error(error);}
int main(int argc,char** argv){try{
    if(argc!=4 && argc!=5 && argc!=6){bool rejected=false;try{(void)ReadOriginalActor({},{});}catch(const std::exception&){rejected=true;}Check(rejected,"empty actor DFF accepted");return 0;}
    std::ifstream stream(argv[1],std::ios::binary),cache(argv[3],std::ios::binary);PackageReader package(stream);const auto model=std::string(argv[2]);const auto dff=package.Read(model+"\\"+model+".dff");const auto expected=ReadActor(cache);
    for(const auto size:{std::size_t{1},std::size_t{11},dff.size()/2,dff.size()-1}){bool rejected=false;try{(void)ReadOriginalActor(std::vector<std::uint8_t>(dff.begin(),dff.begin()+size),{});}catch(const std::exception&){rejected=true;}Check(rejected,"truncated actor DFF accepted");}
    EffectTextures images([&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{const auto member=model+"\\"+std::string(name.substr(name.find_last_of('\\')+1));if(!package.Find(member))return {};return package.Read(member);});
    const std::string variant=argc==6?argv[5]:"";auto actual=ReadOriginalActor(dff,[&](std::string_view name){return images.Load("gamedata\\effect\\tex\\"+std::string(name))->image;},variant,[&]{return DecodeOriginalTexture(package.Read(model+"\\"+variant+".png"),false);});
    Check(actual.frames.size()==expected.frames.size() && actual.palettes.size()==expected.palettes.size() && actual.mesh.vertices.size()==expected.mesh.vertices.size() && actual.mesh.draws.size()==expected.mesh.draws.size() && actual.mesh.textures.size()==expected.mesh.textures.size(),"actor counts differ");
    for(std::size_t i=0;i<actual.frames.size();++i)Check(actual.frames[i].parent==expected.frames[i].parent && actual.frames[i].local==expected.frames[i].local,"actor frames differ");
    for(std::size_t i=0;i<actual.palettes.size();++i)Check(actual.palettes[i].frame==expected.palettes[i].frame && actual.palettes[i].inverse_bind==expected.palettes[i].inverse_bind,"actor palettes differ");
    for(std::size_t i=0;i<actual.influences.size();++i)Check(actual.influences[i].bones==expected.influences[i].bones && actual.influences[i].weights==expected.influences[i].weights,"actor skin differs");
    for(std::size_t i=0;i<actual.mesh.vertices.size();++i){const auto& x=actual.mesh.vertices[i];const auto& y=expected.mesh.vertices[i];for(unsigned j=0;j<3;++j)Check(x.position[j]==y.position[j],"actor vertex differs");for(unsigned j=0;j<2;++j)Check(x.uv0[j]==y.uv0[j] && x.uv1[j]==y.uv1[j],"actor UV differs");for(unsigned j=0;j<4;++j)Check(std::abs(x.color[j]-y.color[j])<0.00001f,"actor color differs");}
    for(std::size_t i=0;i<actual.mesh.draws.size();++i){const auto& x=actual.mesh.draws[i];const auto& y=expected.mesh.draws[i];Check(x.first==y.first && x.count==y.count && x.diffuse==y.diffuse && x.lightmap==y.lightmap,"actor draw differs");for(unsigned j=0;j<4;++j)Check(std::abs(x.color[j]-y.color[j])<0.00001f,"actor material color differs");}
    if(!expected.atomic_spheres.empty()){Check(actual.atomic_spheres.size()==expected.atomic_spheres.size(),"actor sphere count differs");for(std::size_t i=0;i<actual.atomic_spheres.size();++i)Check(actual.atomic_spheres[i].frame==expected.atomic_spheres[i].frame && actual.atomic_spheres[i].sphere==expected.atomic_spheres[i].sphere,"actor sphere differs");}
    if(expected.render_bindings){Check(actual.render_bindings && actual.render_bindings->materials==expected.render_bindings->materials && actual.render_bindings->geometry_materials==expected.render_bindings->geometry_materials,"actor material identity differs");
        Check(actual.render_bindings->atomics.size()==expected.render_bindings->atomics.size() && actual.render_bindings->draws.size()==expected.render_bindings->draws.size(),"actor binding counts differ");
        for(std::size_t i=0;i<actual.render_bindings->atomics.size();++i)Check(actual.render_bindings->atomics[i].geometry==expected.render_bindings->atomics[i].geometry && actual.render_bindings->atomics[i].flags==expected.render_bindings->atomics[i].flags,"actor atomic binding differs");
        for(std::size_t i=0;i<actual.render_bindings->draws.size();++i)Check(actual.render_bindings->draws[i].atomic==expected.render_bindings->draws[i].atomic && actual.render_bindings->draws[i].material_slot==expected.render_bindings->draws[i].material_slot,"actor draw binding differs");}
    if(expected.bind_vertices)Check(actual.bind_vertices==expected.bind_vertices,"actor bounds vertices differ");
    for(std::size_t i=0;i<actual.mesh.textures.size();++i){const auto& x=actual.mesh.textures[i];const auto& y=expected.mesh.textures[i];Check(x.width==y.width && x.height==y.height && x.rgba.size()==y.rgba.size(),"actor texture dimensions differ");for(std::size_t j=0;j<x.rgba.size();++j)Check(std::abs(int(x.rgba[j])-int(y.rgba[j]))<=(j%4==3?0:1),"actor texture differs");}
    const auto hierarchy=ReadDffAnimationHierarchy(dff);for(const auto& clip:expected.clips)actual.clips.push_back(ReadOriginalAnimation(package.Read(model+"\\"+clip.name+".anm"),clip.name,hierarchy));
    if(argc==5){std::set<std::string> names;for(auto name:package.ListFiles()){if(name.size()<=4)continue;std::string extension=name.substr(name.size()-4);for(auto& c:extension)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');if(extension!=".anm" || !package.Find(model+"\\"+name))continue;name.resize(name.size()-4);names.insert(name);}std::uint64_t keys=0;for(const auto& name:names){const auto clip=ReadOriginalAnimation(package.Read(model+"\\"+name+".anm"),name,hierarchy);for(const auto& track:clip.tracks)keys+=track.keys.size();}std::cout<<"discovered="<<names.size()<<" keys="<<keys<<'\n';}
    std::vector<Vertex> a,b;for(std::size_t i=0;i<actual.clips.size();++i)for(float fraction:{0.f,.5f,1.f}){SampleActorVertices(actual,static_cast<int>(i),actual.clips[i].duration*fraction,a,AnimationTimeMode::clamp);SampleActorVertices(expected,static_cast<int>(i),expected.clips[i].duration*fraction,b,AnimationTimeMode::clamp);Check(a.size()==b.size(),"sample count differs");for(std::size_t j=0;j<a.size();++j)for(unsigned k=0;k<3;++k)Check(std::abs(a[j].position[k]-b[j].position[k])<.001f,"sampled animation differs");}
    std::cout<<"model="<<model<<" vertices="<<actual.mesh.vertices.size()<<" palettes="<<actual.palettes.size()<<" clips="<<actual.clips.size()<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
