#include "pal4/portable/arena_source.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
    if(argc==3 && std::string_view(argv[2])=="--texture-filter"){
        ArenaSource source(argv[1],{});
        for(bool lazy:{false,true}){
            const auto actor=source.ReadOriginalActorResource("gamedata\\PALObject\\OJ29\\","OJ29",{},lazy);
            Check(actor.mesh.vertices.size()==1188 && actor.mesh.textures.size()==1,"OJ29 fixture geometry changed");
            Check(actor.mesh.textures[0].filter==TextureFilter::linear,"original object did not receive resource filter override");
            Check(actor.mesh.textures[0].rgba.size()==128*128*4,"OJ29 atlas changed");
        }
        std::cout<<"original OJ29 eager/lazy resource texture filter passed\n";return 0;
    }
    if(argc!=4)return 2;
    ArenaSource source(argv[1],{});const std::string model=argv[2];const auto actor=source.ReadOriginalActorResource("gamedata\\PALActor\\"+model+"\\",model);std::ifstream input(argv[3],std::ios::binary);const auto expected=ReadActor(input);
    Check(actor.mesh.vertices.size()==expected.mesh.vertices.size() && actor.frames.size()==expected.frames.size() && actor.palettes.size()==expected.palettes.size(),"actor geometry mismatch");
    for(const auto& previous:expected.clips){const auto it=std::find_if(actor.clips.begin(),actor.clips.end(),[&](const auto& clip){return clip.name==previous.name;});Check(it!=actor.clips.end(),"original clip name missing");Check(it->duration==previous.duration && it->tracks.size()==previous.tracks.size(),"original clip mismatch");for(std::size_t i=0;i<it->tracks.size();++i){const auto& a=it->tracks[i];const auto& b=previous.tracks[i];Check(a.frame==b.frame && a.keys.size()==b.keys.size(),"original track mismatch");for(std::size_t j=0;j<a.keys.size();++j)Check(a.keys[j].time==b.keys[j].time && a.keys[j].rotation==b.keys[j].rotation && a.keys[j].translation==b.keys[j].translation,"original key mismatch");}}
    std::cout<<"model="<<model<<" original_clips="<<actor.clips.size()<<" reference_clips="<<expected.clips.size()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
