#include "pal4/portable/dff_collision.h"
#include "pal4/portable/effect_texture.h"
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
    if(argc!=4){bool rejected=false;try{(void)ReadDffEnvironment({},{});}catch(const std::exception&){rejected=true;}Check(rejected,"empty DFF accepted");return 0;}
    const std::filesystem::path directory=argv[3];EffectTextures images([&](std::string_view name)->std::optional<std::vector<std::uint8_t>>{const auto filename=Lower(std::string(name.substr(name.find_last_of('\\')+1)));for(const auto& entry:std::filesystem::directory_iterator(directory))if(entry.is_regular_file() && Lower(entry.path().filename().string())==filename)return Read(entry.path());return {};});
    const auto bytes=Read(argv[1]);const auto actual=ReadDffEnvironment(bytes,[&](std::string_view name){return images.Load("gamedata\\effect\\tex\\"+std::string(name))->image;});std::ifstream input(argv[2],std::ios::binary);const auto expected=ReadScene(input);
    for(std::size_t n=0;n<bytes.size();n+=bytes.size()/32+1){bool rejected=false;try{(void)ReadDffEnvironment(std::span(bytes).first(n),{});}catch(const std::exception&){rejected=true;}Check(rejected,"truncated DFF accepted");}
    Check(actual.vertices.size()==expected.vertices.size() && actual.draws.size()==expected.draws.size() && actual.textures.size()==expected.textures.size(),"DFF scene counts differ");
    for(std::size_t i=0;i<actual.draws.size();++i){const auto& a=actual.draws[i];const auto& b=expected.draws[i];Check(a.first==b.first && a.count==b.count && a.diffuse==b.diffuse && a.lightmap==b.lightmap,"DFF draw differs");for(unsigned j=0;j<4;++j)Check(std::abs(a.color[j]-b.color[j])<0.00001f,"DFF material color differs");}
    for(std::size_t i=0;i<actual.vertices.size();++i){const auto& a=actual.vertices[i];const auto& b=expected.vertices[i];for(unsigned j=0;j<3;++j)Check(std::abs(a.position[j]-b.position[j])<0.001f,"DFF position differs");for(unsigned j=0;j<2;++j)Check(std::abs(a.uv0[j]-b.uv0[j])<0.00001f && std::abs(a.uv1[j]-b.uv1[j])<0.00001f,"DFF UV differs");for(unsigned j=0;j<4;++j)Check(std::abs(a.color[j]-b.color[j])<0.00001f,"DFF vertex color differs");}
    for(std::size_t i=0;i<actual.textures.size();++i){const auto& a=actual.textures[i];const auto& b=expected.textures[i];Check(a.width==b.width && a.height==b.height && a.rgba.size()==b.rgba.size(),"DFF texture dimensions differ");unsigned maximum=0;std::size_t differences=0,alpha_differences=0;for(std::size_t j=0;j<a.rgba.size();++j){const auto delta=std::abs(int(a.rgba[j])-int(b.rgba[j]));maximum=std::max(maximum,unsigned(delta));differences+=delta!=0;alpha_differences+=(j%4==3 && delta!=0);}if(differences)std::cout<<"texture "<<i<<" differing_channels="<<differences<<" alpha="<<alpha_differences<<" max="<<maximum<<'\n';Check(maximum<=1 && alpha_differences==0,"DFF texture differs beyond decoder rounding");}
    std::cout<<"environment vertices="<<actual.vertices.size()<<" draws="<<actual.draws.size()<<" textures="<<actual.textures.size()<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
