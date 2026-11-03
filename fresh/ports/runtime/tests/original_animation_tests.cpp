#include "pal4/portable/original_animation.h"
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/package.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value,const char* error){if(!value)throw std::runtime_error(error);}
int main(int argc,char** argv){try{
    if(argc!=4){bool failed=false;try{(void)ReadDffAnimationHierarchy({});}catch(const std::exception&){failed=true;}Check(failed,"empty DFF accepted");
        failed=false;try{OriginalAnimationStore bad({0},{"../invalid"},[](auto){return std::vector<std::uint8_t>{};});}catch(const std::exception&){failed=true;}Check(failed,"invalid animation name accepted");return 0;}
    std::ifstream stream(argv[1],std::ios::binary),cache(argv[3],std::ios::binary);PackageReader package(stream);const auto model=std::string(argv[2]);const auto dff=package.Read(model+"\\"+model+".dff");const auto hierarchy=ReadDffAnimationHierarchy(dff);auto actor=ReadActor(cache);Check(!actor.clips.empty(),"reference actor has no clips");
    {
        std::vector<std::string> names;for(const auto& clip:actor.clips)names.push_back(clip.name);
        unsigned reads=0;bool fail=true;
        OriginalAnimationStore store(hierarchy,names,[&](std::string_view name){++reads;if(fail)return std::vector<std::uint8_t>{};return package.Read(model+"\\"+std::string(name)+".anm");});
        bool rejected=false;try{(void)store.Get("missing");}catch(const std::exception&){rejected=true;}Check(rejected && reads==0,"unknown animation reached reader");
        rejected=false;try{(void)store.Get(names[0]);}catch(const std::exception&){rejected=true;}Check(rejected && reads==1,"invalid animation was accepted");
        fail=false;auto first=store.Get(names[0]);auto shared=store.Get(names[0]);Check(first==shared && reads==2,"animation was not shared");
        std::weak_ptr<const AnimationClip> weak=first;first.reset();shared.reset();Check(weak.expired(),"animation stayed alive after release");
        first=store.Get(names[0]);Check(reads==3,"released animation was not reread");
        std::vector<Vertex> reference,actual;SampleActorVertices(actor,0,0,reference,AnimationTimeMode::clamp);SampleActorClipVertices(actor,first.get(),0,actual,AnimationTimeMode::clamp);
        Check(reference.size()==actual.size(),"lazy animation vertex count mismatch");for(std::size_t i=0;i<actual.size();++i)for(unsigned j=0;j<3;++j)Check(std::abs(actual[i].position[j]-reference[i].position[j])<.001f,"lazy animation pose mismatch");
        first.reset();
        ActorPlayback playback(actor,model,[](std::string_view)->std::optional<std::vector<std::uint8_t>>{return {};},{},false,[&](std::string_view name){return store.Get(name);});
        Check(playback.Select(names[0]),"lazy playback selection failed");playback.Sample(actual);Check(actual.size()==reference.size(),"lazy playback pose missing");
    }
    std::size_t keys=0;for(const auto& expected:actor.clips){const auto bytes=package.Read(model+"\\"+expected.name+".anm");const auto actual=ReadOriginalAnimation(bytes,expected.name,hierarchy);Check(actual.duration==expected.duration && actual.tracks.size()==expected.tracks.size(),"animation header mismatch");
        for(std::size_t i=0;i<actual.tracks.size();++i){const auto& a=actual.tracks[i];const auto& b=expected.tracks[i];Check(a.frame==b.frame && a.keys.size()==b.keys.size(),"animation track mismatch");for(std::size_t j=0;j<a.keys.size();++j){const auto& x=a.keys[j];const auto& y=b.keys[j];Check(x.time==y.time && x.rotation==y.rotation && x.translation==y.translation,"animation key mismatch");++keys;}}
        bool rejected=false;try{(void)ReadOriginalAnimation(std::span(bytes).first(bytes.size()-1),expected.name,hierarchy);}catch(const std::exception&){rejected=true;}Check(rejected,"truncated ANM accepted");}
    const auto original_count=actor.clips.size();bool failed=false;try{LoadOriginalAnimations(actor,dff,[](std::string_view){return std::vector<std::uint8_t>{};});}catch(const std::exception&){failed=true;}Check(failed && actor.clips.size()==original_count,"failed ANM load changed actor");
    if(original_count>1){const auto first_name=actor.clips.front().name;const auto first_key=actor.clips.front().tracks.front().keys.front();unsigned calls=0;failed=false;try{LoadOriginalAnimations(actor,dff,[&](std::string_view name){if(calls++==0)return package.Read(model+"\\"+std::string(name)+".anm");return std::vector<std::uint8_t>{};});}catch(const std::exception&){failed=true;}Check(failed && calls==2 && actor.clips.size()==original_count && actor.clips.front().name==first_name && actor.clips.front().tracks.front().keys.front().rotation==first_key.rotation,"partial ANM load changed actor");}
    LoadOriginalAnimations(actor,dff,[&](std::string_view name){return package.Read(model+"\\"+std::string(name)+".anm");});Check(actor.clips.size()==original_count,"ANM replacement lost clips");
    std::cout<<"model="<<model<<" nodes="<<hierarchy.size()<<" clips="<<actor.clips.size()<<" keys="<<keys<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
