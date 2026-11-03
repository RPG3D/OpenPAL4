#include "pal4/portable/uv_animation.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace pal4::portable;
namespace {
using Bytes=std::vector<std::uint8_t>;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Word(Bytes& b,std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(v>>(i*8)));}
void Float(Bytes& b,float v){Word(b,std::bit_cast<std::uint32_t>(v));}
Bytes Chunk(unsigned id,const Bytes& data){Bytes out;Word(out,id);Word(out,static_cast<unsigned>(data.size()));Word(out,0x1803ffff);out.insert(out.end(),data.begin(),data.end());return out;}
void Append(Bytes& a,const Bytes& b){a.insert(a.end(),b.begin(),b.end());}
Bytes Fixture(unsigned channel=0){
    Bytes a;Word(a,0x100);Word(a,0x1c1);Word(a,2);Word(a,0);Float(a,100);Word(a,0);
    a.resize(56);a[24]='T';for(unsigned i=0;i<8;++i)Word(a,channel);
    for(unsigned i=0;i<2;++i){Float(a,i?100.f:0.f);for(float v:{0.f,1.f,1.f,0.f,i?-1.f:0.f,0.f})Float(a,v);Word(a,i?0:0xff90a204);}
    Bytes count;Word(count,1);auto body=Chunk(1,count);Append(body,Chunk(0x1b,a));return Chunk(0x2b,body);
}
Scene Bound(std::string name){Scene scene;Draw draw{};draw.color[0]=.2f;draw.color[1]=.3f;draw.color[2]=.4f;draw.color[3]=.5f;draw.uv_animations[0]=std::move(name);scene.draws.push_back(draw);return scene;}
void Near(float actual,float expected){Check(std::abs(actual-expected)<2e-6f,"UV result differs from SDK matrix");}
template<class F>void Reject(F action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}Check(rejected,"invalid UV input was accepted");}
}
int main(int argc,char** argv){try{
    if(argc==3){
        std::ifstream stream(argv[1],std::ios::binary);Check(bool(stream),"missing original UVA");
        Bytes bytes((std::istreambuf_iterator<char>(stream)),{});UvAnimationDictionary dictionary(bytes);const auto scene=Bound(argv[2]);
        std::printf("{\"samples\":[");unsigned i=0;
        for(float t:{0.f,1.f,25.f,50.f,99.f,100.f,101.f}){const auto a=dictionary.Evaluate(scene,t);std::printf("%s{\"requested_time\":%.9g,\"uv\":[",i++?",":"",t);for(unsigned c=0;c<6;++c)std::printf("%s%.9g",c?",":"",a[0].uv[0][c]);std::printf("]}");}
        std::printf("]}\n");return 0;
    }
    Check(argc==1,"usage: uv-tests [original.uva animation-name]");
    auto bytes=Fixture();UvAnimationDictionary dictionary(bytes);const auto scene=Bound("T");
    for(float time:{0.f,1.f,25.f,50.f,99.f,100.f,101.f,200.f,201.f}){
        const auto a=dictionary.Evaluate(scene,time);Check(a.size()==1&&a[0].visible,"appearance ownership");
        const float fraction=time<=100?time/100:(time==200?1.f:std::fmod(time,100.f)/100);
        Near(a[0].uv[0][0],1);Near(a[0].uv[0][3],1);Near(a[0].uv[0][4],-fraction);
        Check(a[0].uv[1]==kIdentityUv,"animation leaked into unbound channel");Near(a[0].color[0],.2f);Near(a[0].color[3],.5f);
    }
    {UvAnimationDictionary second(Fixture(1));auto a=second.Evaluate(scene,25);Check(a[0].uv[0]==kIdentityUv,"second channel animated first channel");Near(a[0].uv[1][4],-.25f);}
    Reject([&]{dictionary.Evaluate(Bound("absent"),0);});
    Reject([&]{dictionary.Evaluate(scene,-1);});
    for(std::size_t n=0;n<bytes.size();++n)Reject([&]{UvAnimationDictionary invalid{std::span(bytes).first(n)};});
    auto invalid=bytes;invalid[188]=1;Reject([&]{UvAnimationDictionary bad(invalid);});
    invalid=bytes;for(unsigned i=0;i<4;++i)invalid[156+i]=0;Reject([&]{UvAnimationDictionary bad(invalid);});
    invalid=bytes;invalid[0x60]=3;Reject([&]{UvAnimationDictionary bad(invalid);});
    // The dictionary has one active node. Unused channel-map entries are
    // serialized storage, not eight active nodes (original BJ exporter tail).
    {auto tail=bytes;for(unsigned i=0;i<4;++i)tail[0x64+i]=0xff;
        UvAnimationDictionary ignored_tail(tail);const auto a=ignored_tail.Evaluate(scene,25);
        Near(a[0].uv[0][4],-.25f);Check(a[0].uv[1]==kIdentityUv,"inactive map slot affects channel matrices");}
    const auto again=dictionary.Evaluate(scene,0);Check(again[0].uv[0]==kIdentityUv,"absolute evaluation retained previous time");
    std::puts("UV SDK samples, wrap, channels, colors, independent ownership and malformed streams passed");return 0;
}catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}}
