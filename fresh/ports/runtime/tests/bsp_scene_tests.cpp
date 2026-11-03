#include "pal4/portable/bsp_scene.h"
#include "pal4/portable/effect_texture.h"
#include "pal4/portable/package.h"
#include "pal4/portable/ui_world.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value,const char* error){if(!value)throw std::runtime_error(error);}
std::vector<std::uint8_t> Read(const std::filesystem::path& path){std::ifstream in(path,std::ios::binary);Check(bool(in),"missing test file");return {std::istreambuf_iterator<char>(in),{}};}
std::string Lower(std::string s){for(auto& c:s)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');return s;}
int main(int argc,char** argv){try{
    if(argc==2){
        std::ifstream stream(argv[1],std::ios::binary);Check(bool(stream),"missing UI package");PackageReader package(stream);
        unsigned uva_reads=0;
        auto read=[&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{if(name.ends_with(".uva"))++uva_reads;if(!package.Find(name))return {};return package.Read(name);};
        const auto ui=ReadUiWorld("ZJM",read);Check(bool(ui.transparent),"missing UI transparent scene");
        Check(uva_reads==0,"UI world loading must not parse an unverified UV animation dictionary");
        const auto animated=ReadUiWorld("ZJM",read,true);
        Check(uva_reads==2&&!animated.world_animations&&bool(animated.transparent_animations),"explicit UI animation loading did not preserve dictionary ownership");
        Check(animated.transparent->draws.at(0).uv_animations[0]=="19 - Default","original transparent material did not bind its named animation");
        const auto appearance=animated.transparent_animations->Evaluate(*animated.transparent,25);
        Check(appearance.size()==6&&animated.transparent->atomic_centers.size()==6,"original cloud atomics were merged before transparent sorting");
        for(const auto& item:appearance)Check(std::abs(item.uv[0][4]+.25f)<2e-6f&&item.uv[1]==kIdentityUv,"original CPK material UV differs from SDK sample");
        Check(ui.world.vertices.size()==17640 && ui.world.draws.size()==58 && ui.world.textures.size()==7,"unexpected UI world geometry");
        Check(ui.transparent->vertices.size()==36 && ui.transparent->draws.size()==6 && ui.transparent->textures.size()==1,"unexpected UI transparent geometry");
        bool rejected=false;try{(void)ReadUiWorld("../ZJM",read);}catch(const std::invalid_argument&){rejected=true;}Check(rejected,"invalid UI world name accepted");
        rejected=false;try{(void)ReadUiWorld("missing",read);}catch(const std::runtime_error&){rejected=true;}Check(rejected,"missing UI world accepted");
        std::cout<<"ZJM world vertices="<<ui.world.vertices.size()<<" draws="<<ui.world.draws.size()<<" textures="<<ui.world.textures.size()<<" transparent vertices="<<ui.transparent->vertices.size()<<" draws="<<ui.transparent->draws.size()<<" textures="<<ui.transparent->textures.size()<<'\n';
        return 0;
    }
    if(argc!=5){bool rejected=false;try{(void)ReadBspScene({},{},{});}catch(const std::exception&){rejected=true;}Check(rejected,"empty BSP accepted");return 0;}
    const std::filesystem::path directory=argv[4];EffectTextures images([&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{const auto filename=Lower(std::string(name.substr(name.find_last_of('\\')+1)));for(const auto& entry:std::filesystem::directory_iterator(directory))if(entry.is_regular_file() && Lower(entry.path().filename().string())==filename)return Read(entry.path());return {};});
    const auto bytes=Read(argv[1]),config=Read(argv[2]);const auto actual=ReadBspScene(bytes,config,[&](std::string_view name){return images.Load("gamedata\\effect\\tex\\"+std::string(name))->image;});std::ifstream input(argv[3],std::ios::binary);const auto expected=ReadScene(input);
    for(std::size_t n=0;n<bytes.size();n+=bytes.size()/32+1){bool rejected=false;try{(void)ReadBspScene(std::span(bytes).first(n),config,{});}catch(const std::exception&){rejected=true;}Check(rejected,"truncated BSP accepted");}
    Check(actual.vertices.size()==expected.vertices.size() && actual.draws.size()==expected.draws.size() && actual.textures.size()==expected.textures.size(),"BSP scene counts differ");
    Check(actual.saturation==expected.saturation && actual.brightness==expected.brightness,"BSP light settings differ");
    for(std::size_t i=0;i<actual.draws.size();++i){const auto& a=actual.draws[i];const auto& b=expected.draws[i];Check(a.first==b.first && a.count==b.count && a.diffuse==b.diffuse && a.lightmap==b.lightmap,"BSP draw differs");for(unsigned j=0;j<4;++j)Check(std::abs(a.color[j]-b.color[j])<0.00001f,"BSP material differs");}
    for(std::size_t i=0;i<actual.vertices.size();++i){const auto& a=actual.vertices[i];const auto& b=expected.vertices[i];for(unsigned j=0;j<3;++j)Check(a.position[j]==b.position[j],"BSP position differs");for(unsigned j=0;j<2;++j)Check(a.uv0[j]==b.uv0[j] && a.uv1[j]==b.uv1[j],"BSP UV differs");for(unsigned j=0;j<4;++j)Check(std::abs(a.color[j]-b.color[j])<0.00001f,"BSP color differs");}
    for(std::size_t i=0;i<actual.textures.size();++i){const auto& a=actual.textures[i];const auto& b=expected.textures[i];Check(a.width==b.width && a.height==b.height && a.rgba.size()==b.rgba.size(),"BSP texture dimensions differ");for(std::size_t j=0;j<a.rgba.size();++j)Check(std::abs(int(a.rgba[j])-int(b.rgba[j]))<=(j%4==3?0:1),"BSP texture pixel differs");}
    std::cout<<"BSP vertices="<<actual.vertices.size()<<" draws="<<actual.draws.size()<<" textures="<<actual.textures.size()<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
